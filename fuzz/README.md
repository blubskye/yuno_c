# Fuzzing & Sanitizers

## Building with Sanitizers

The main `CMakeLists.txt` supports three sanitizer options:

```bash
# ASan + UBSan (recommended for development)
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DSANITIZE_ADDRESS=ON -DSANITIZE_UNDEFINED=ON
cmake --build build-asan

# TSan (for data race detection in pthread code)
cmake -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DSANITIZE_THREAD=ON
cmake --build build-tsan
```

Run the bot normally — sanitizers report errors at runtime with stack traces.

## Building Fuzz Targets

### libFuzzer (Clang)

```bash
cmake -B build-fuzz -S fuzz -DCMAKE_C_COMPILER=clang
cmake --build build-fuzz
```

### AFL++

```bash
cmake -B build-afl -S fuzz -DCMAKE_C_COMPILER=afl-clang-fast -DAFL_MODE=ON
cmake --build build-afl
```

## Running Fuzzers

### libFuzzer

```bash
# Config parsing (runs indefinitely, Ctrl+C to stop)
./build-fuzz/fuzz_config fuzz/corpus/config/

# JSON API response parsing
./build-fuzz/fuzz_json_response fuzz/corpus/json_response/

# Ban file import parsing
./build-fuzz/fuzz_ban_import fuzz/corpus/ban_import/

# Command argument parsing
./build-fuzz/fuzz_command_parse fuzz/corpus/command/

# Quick smoke test (10 seconds each)
./build-fuzz/fuzz_config -max_total_time=10 fuzz/corpus/config/
./build-fuzz/fuzz_json_response -max_total_time=10 fuzz/corpus/json_response/
./build-fuzz/fuzz_ban_import -max_total_time=10 fuzz/corpus/ban_import/
./build-fuzz/fuzz_command_parse -max_total_time=10 fuzz/corpus/command/
```

### AFL++

```bash
# Config parsing
afl-fuzz -i fuzz/corpus/config/ -o findings-config/ -- ./build-afl/fuzz_config @@

# JSON response parsing
afl-fuzz -i fuzz/corpus/json_response/ -o findings-json/ -- ./build-afl/fuzz_json_response @@

# Ban import parsing
afl-fuzz -i fuzz/corpus/ban_import/ -o findings-bans/ -- ./build-afl/fuzz_ban_import @@

# Command parsing
afl-fuzz -i fuzz/corpus/command/ -o findings-cmd/ -- ./build-afl/fuzz_command_parse @@
```

## Fuzz Targets

| Target | What it tests | Source code targeted |
|--------|--------------|---------------------|
| `fuzz_config` | Config JSON loading | `src/config.c` — `config_load()` |
| `fuzz_json_response` | API response parsing | `src/commands/fun.c` — anime, manga, urban, neko, hentai patterns |
| `fuzz_ban_import` | Ban file import | `src/modules/terminal.c` — `terminal_cmd_timportbans()` |
| `fuzz_command_parse` | Prefix command dispatch | `src/bot.c` — `on_message_create()`, `parse_user_mention()` |

## Interpreting Crashes

libFuzzer saves crashing inputs to `crash-*` and `timeout-*` files in the current directory. Reproduce with:

```bash
./build-fuzz/fuzz_config crash-abc123
```

AFL++ saves crashes to `findings-*/crashes/`. Reproduce with:

```bash
./build-afl/fuzz_config findings-config/crashes/id:000000,...
```

Both will print ASan/UBSan stack traces showing the exact location of the bug.

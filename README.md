<div align="center">

# 💕 Yuno Gasai 2 (C Edition) 💕

### *"I'll protect this server forever... just for you~"* 💗

<img src="https://i.imgur.com/jF8Szfr.png" alt="Yuno Gasai" width="300"/>

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-pink.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![C](https://img.shields.io/badge/C-11-ff69b4.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Concord](https://img.shields.io/badge/Concord-Discord%20API-ff1493.svg)](https://github.com/Cogmasters/concord)

*A devoted Discord bot for moderation, leveling, and anime~ ♥*

---

### 💘 Ported to plain C... for the memes 💘

*Because why not rewrite everything in C?*

### 💘 She loves you... and only you 💘

</div>

## 🌸 About

Yuno is a **yandere-themed Discord bot** combining powerful moderation tools with a leveling system, anime API lookups, and server management features. She'll keep your server safe from troublemakers... *because no one else is allowed near you~* 💕

This is the **pure C port** of the original JavaScript version using the [Concord library](https://github.com/Cogmasters/concord) (dev branch). Why C? *Because we can.*

---

## 👑 Credits

*"These are the ones who gave me life~"* 💖

| Contributor | Role |
|-------------|------|
| **blubskye** | Project Owner, C Porter 💕🔪 |
| **Maeeen** (maeeennn@gmail.com) | Original Developer 💝 |
| **Oxdeception** | Contributor 💗 |
| **fuzzymanboobs** | Contributor 💗 |

---

## 💗 Features

<table>
<tr>
<td width="50%">

### 🔪 Moderation
*"Anyone who threatens you... I'll eliminate them~"*
- ⛔ Ban / Unban / Kick / Timeout
- 🧹 Channel cleaning & scheduled auto-clean
- 🛡️ Spam filter with custom per-guild rules
- 🔗 Invite link filter
- 👑 Mod statistics tracking (per-moderator breakdown)
- 📥 Scan, export & import ban lists
- 🖼️ Custom ban images per user

</td>
<td width="50%">

### ✨ Leveling System
*"Watch me make you stronger, senpai~"*
- 📊 XP & Level tracking (per message + voice chat XP)
- ⚙️ Configurable XP per message
- 🎭 Automatic role rewards per level (`/set-levelrolemap`)
- 🏆 Server leaderboards
- 📈 Admin tools: set-level, mass-addxp, mass-setxp, fix-xp-data
- 🔄 Sync XP from existing roles

</td>
</tr>
<tr>
<td width="50%">

### 🌸 Anime & Fun
*"Let me show you something cute~"*
- 🎌 `/anime` - Anime lookup (Jikan API v4)
- 📖 `/manga` - Manga lookup (Jikan API v4)
- 🐱 `/neko` - Cat pictures (nekos.life)
- 📚 `/urban` - Urban Dictionary lookup
- 🔞 `/hentai` - NSFW content (Rule34, channel-gated)
- 🎱 `/8ball` - Fortune telling
- 📜 `/quote` - Yuno Gasai quotes
- 💖 `/praise` / `/scold` - Image reactions
- 💬 Custom mention responses per user

</td>
<td width="50%">

### ⚙️ Configuration & Admin
*"I'll be exactly what you need~"*
- 🔧 Customizable prefix per guild
- ⚡ Slash commands + prefix commands (O(1) hash dispatch)
- 🛠️ `/config get/set` - Runtime config management
- 📬 DM forwarding to configured channels
- 👋 Welcome DM messages for new members
- 📋 Activity logging (edits, deletes, bans, role changes)
- 🚨 Error channel logging
- 👑 Master user system (config + runtime `/add-masteruser`)
- 🚫 Bot-level user bans

</td>
</tr>
<tr>
<td width="50%">

### 💻 Terminal Interface
*"I'm always at your command~"*
- 🖥️ Interactive CLI with command prompt
- 📬 DM inbox viewer
- 🎭 Bot status management
- 👁️ Real-time channel watch (`watch <channel-id>`)
- ⛔ Terminal ban export/import (`texportbans`, `timportbans`)
- 🔄 Config hot-reload (`reload`)
- 🔃 Auto-update check (`auto-update`)
- 📋 Full command listing (`commands`)

</td>
<td width="50%">

### ⚡ Performance & Architecture
*"Nothing can slow me down~"*
- 🚀 **C11** with no runtime overhead
- 🔗 Concord async Discord API (dev branch v3.0.0)
- 🧵 Worker threadpool for non-blocking HTTP lookups
- ⚡ Async Discord API callbacks (DMs, welcomes)
- 💾 SQLite3 with WAL mode
- 🎯 O(1) hash-based command dispatch
- 📈 XP batcher with hash table (batches writes every 10s)
- 🧠 LRU cache layer (256 entries, configurable TTL)
- ⏱️ Concord event-loop timers (auto-cleaner, XP flush)
- 🪶 Low memory mode for activity logger
- 🔐 Optional SQLCipher database encryption

</td>
</tr>
<tr>
<td colspan="2">

### 🔒 Security Hardening
*"I'll keep your server safe... no matter the cost~"* 🔪
- ✅ **Zero known vulnerabilities** — comprehensive security audit completed
- 🛡️ **SQL injection protection** — parameterized queries + safe encryption key handling
- 📏 **Buffer overflow prevention** — all `strcpy`/`sprintf` replaced with bounds-checked variants
- ✔️ **Input validation** — safe integer parsing with range checks (no `atoi`)
- 📁 **Path traversal protection** — `realpath()` validation for file operations
- 🧠 **Memory safety** — NULL checks after all allocations, no use-after-free
- 🌐 **HTTP security** — 10MB response limit, explicit TLS certificate verification
- 🔄 **Thread safety** — `strtok_r` for reentrant string parsing
- 🔢 **Integer overflow protection** — bounds checking in XP calculations and array indexing
- 🚫 **No command injection** — zero usage of `system()`, `popen()`, or shell execution

</td>
</tr>
<tr>
<td colspan="2">

### 💀 Crash Handling & Diagnostics
*"Even in death... I'll leave you a note~"*
- 🚨 **Fatal signal handler** — catches SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL
- 🗂️ **Full stack traces** via `backtrace()` / `backtrace_symbols()` printed to stderr
- 📄 **Crash dump files** — automatically written to `crash_<timestamp>.log` with signal info, PID, and full trace
- 🔍 **`addr2line` ready** — compile with `-g` for source file and line number resolution
- 🏷️ **`-rdynamic`** linked by default for human-readable function names in traces
- 🧪 **Sanitizer support** — AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer (CMake options)
- 💔 **Graceful shutdown** — escalating Ctrl+C (stop → force exit → kernel kill)

</td>
</tr>
</table>

---

## 💕 Installation

### 📋 Prerequisites

> *"Let me prepare everything for you~"* 💗

- **CMake** (3.15+)
- **C11 compiler** (GCC, Clang)
- **Concord** (Discord API library for C, dev branch)
- **SQLite3**
- **json-c**
- **libcurl**
- **Git**

### 🔧 Installing Dependencies

**🐧 Fedora/RHEL:**
```bash
sudo dnf install cmake gcc sqlite-devel json-c-devel libcurl-devel
```

**🐧 Ubuntu/Debian:**
```bash
sudo apt install cmake gcc libsqlite3-dev libjson-c-dev libcurl4-openssl-dev
```

**Installing Concord (dev branch):**
```bash
git clone https://github.com/Cogmasters/concord.git
cd concord
git checkout dev
make static
sudo make install
```

### 🌸 Build Steps

```bash
# Clone the repository~ ♥
git clone https://github.com/blubskye/yuno_c.git
cd yuno_c

# Let me gather my strength...
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**🔐 Optional: Build with SQLCipher encryption:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DUSE_SQLCIPHER=ON
cmake --build build
```

**🧪 Optional: Build with sanitizers (for development/debugging):**
```bash
# AddressSanitizer (memory errors, leaks) + UBSan
cmake -B build -DSANITIZE_ADDRESS=ON -DSANITIZE_UNDEFINED=ON
cmake --build build

# ThreadSanitizer (data races)
cmake -B build -DSANITIZE_THREAD=ON
cmake --build build
```

### 📦 Pre-built Binaries

> *"I've already prepared everything... just for you~"* 💕

Pre-built binaries are available on the [Releases](https://github.com/blubskye/yuno_c/releases) page for:

| Platform | Architecture | File |
|----------|-------------|------|
| 🐧 Linux | x86_64 (amd64) | `yuno_gasai-linux-amd64.tar.gz` |
| 🐧 Linux | aarch64 (arm64) | `yuno_gasai-linux-arm64.tar.gz` |

Download, extract, edit `config.example.json` → `config.json`, and run.

> **⚠️ Note:** Windows binaries are not available — Concord (dev branch) depends on POSIX APIs (`poll.h`, `pipe()`) with no Windows support upstream.

### 📊 Binary Size Comparison

*"Small but deadly~"* 🔪

Build sizes across optimization levels (GCC 15.2, x86_64 Linux):

| Variant | Raw | Stripped | UPX --best |
|---------|-----|----------|------------|
| `-O2` | 980 KiB | 873 KiB | 259 KiB |
| `-O2 -flto` | 989 KiB | 881 KiB | 260 KiB |
| `-O3` | 1000 KiB | 893 KiB | 264 KiB |
| `-O3 -flto` | 1008 KiB | 901 KiB | 268 KiB |
| `-Os` | 937 KiB | 829 KiB | 246 KiB |
| `-Os -flto` | 937 KiB | 829 KiB | 247 KiB |

**Notes:**
- **`-Os`** produces the smallest binary at every stage — 246 KiB with UPX
- **LTO** adds ~12-17 KiB raw; marginal benefit at this codebase size
- **`-O3`** is ~5% larger than `-O2` with no measurable runtime difference (bot is I/O-bound)
- **UPX** compresses all variants to ~30% of stripped size — ideal for deployment
- Release builds use `-O2` by default. Override with `-DCMAKE_C_FLAGS_RELEASE="-Os -DNDEBUG"`

### 💝 Configuration

Copy the example config and edit it:

```bash
cp config.example.json config.json
```

```json
{
    "discord_token": "YOUR_DISCORD_BOT_TOKEN",
    "default_prefix": ".",
    "database_path": "yuno.db",
    "master_users": ["YOUR_USER_ID"],
    "spam_max_warnings": 3,
    "xp_per_msg": 20,
    "ban_default_image": null,
    "dm_message": "I'm just a bot :'(. I can't answer to you.",
    "insufficient_permissions_message": "${author} You don't have permission to do that~",
    "low_memory_mode": false
}
```

Or set the `DISCORD_TOKEN` environment variable for token-only setup. If using SQLCipher, also set `YUNO_DB_KEY` for the encryption passphrase.

### 🔄 Migrating from JS Version

*"Bring your memories with you... I'll keep them safe~"* 💕

If you have an existing database from the [JavaScript version](https://github.com/japaneseenrichmentorganization/Yuno-Gasai-2), you can migrate it:

```bash
# Creates a new file (keeps original intact)
python3 tools/migrate_js_db.py yuno_js.db yuno.db

# Or in-place (backs up original first)
python3 tools/migrate_js_db.py yuno.db
```

The migration script handles all schema differences: table/column renames, type conversions (`bot_presence` TEXT→INTEGER), `levelRoleMap` JSON deserialization into `level_roles` table, and voice XP rate conversion. See `tools/migrate_js_db.py` for details.

### 🚀 Running

```bash
# Run from the project root
./build/bin/yuno_gasai

# Or with a custom config path
./build/bin/yuno_gasai /path/to/config.json
```

---

## 💖 Commands

### 🔪 Moderation
| Command | Alias | Description |
|---------|-------|-------------|
| `/ban <user> [reason]` | | *"They won't bother you anymore..."* ⛔ |
| `/kick <user> [reason]` | | Kick a user |
| `/unban <user>` | | Unban a user |
| `/timeout <user> <duration>` | | Timeout a user |
| `/clean` | | Clone & clean channel 🧹 |
| `/mod-stats` | `modstats` | View moderation statistics 👑 |
| `/scan-bans` | `scanbans` | Import existing guild bans to database |
| `/exportbans` | | Export guild bans to JSON file 📤 |
| `/importbans <guild_id>` | | Import bans from file 📥 |
| `/set-banimage <user> <url>` | `sbi` | Set custom ban image 🖼️ |
| `/del-banimage <user>` | `dbi` | Remove custom ban image |

### ✨ Leveling
| Command | Alias | Description |
|---------|-------|-------------|
| `/xp` | `level`, `rank` | *"Look how strong you've become!"* 📊 |
| `/leaderboard` | `lb`, `top` | Server XP leaderboard 🏆 |
| `/set-level <level> [user]` | `slvl` | Set user level (admin) |
| `/mass-addxp <amount>` | `massxp` | Add XP to all users 📈 |
| `/mass-setxp <level>` | | Set all users to level |
| `/fix-xp-data` | `fixxp` | Fix corrupted XP records 🔧 |
| `/set-levelrolemap <level> <role>` | `slrmap` | Map level to auto-role 🎭 |
| `/sync-levelroles` | `syncroles` | Apply level roles to all users 🔄 |
| `/sync-xp-from-roles` | `syncxp` | Set XP based on role membership |
| `/set-vcxp <on\|off> [xp] [min_users]` | | Configure voice chat XP 🎤 |
| `/vcxp-status` | `vcxp` | Show voice XP config |
| `/set-experiencecounter <on\|off>` | `set-expcounter` | Toggle XP system per guild |

### 🌸 Fun & Anime
| Command | Alias | Description |
|---------|-------|-------------|
| `/8ball <question>` | | *"Let fate decide~"* 🎱 |
| `/quote` | | Random Yuno Gasai quote 📜 |
| `/praise <user>` | | *"You deserve all my love~"* 💕 |
| `/scold <user>` | | *"Bad! But I still love you..."* 💢 |
| `/anime <name>` | `animoo` | Look up anime (Jikan API) 🎌 |
| `/manga <name>` | | Look up manga (Jikan API) 📖 |
| `/neko` | `nya` | *"Nya~"* 🐱 |
| `/urban <term>` | `ub` | Urban Dictionary lookup 📚 |
| `/hentai [tags] [count]` | `hen` | NSFW images (NSFW channels only) 🔞 |

### ⚙️ Configuration
| Command | Alias | Description |
|---------|-------|-------------|
| `/ping` | | *"I'm always here for you~"* 💓 |
| `/help` | | Show help message |
| `/source` | | View source code link |
| `/prefix <new_prefix>` | | Change guild prefix 🔧 |
| `/config <get\|set> <key> [value]` | `cfg` | View/edit bot config (master only) |
| `/stats` | `inf` | Bot stats (uptime, memory, etc.) 📊 |
| `/auto-clean <add\|remove\|list>` | `autoclean` | Manage auto-clean channels 🧹 |
| `/delay [channel] [minutes]` | | Delay next auto-clean ⏱️ |
| `/set-dm-channel <channel>` | `setdm` | Set DM forwarding channel 💌 |
| `/dm-status` | `dmstatus` | Show DM config 📬 |
| `/set-joinmessage <title> \| <msg>` | `sjm` | Set welcome DM 👋 |
| `/set-logchannel <channel> [type]` | `slc` | *"I'll watch over everything~"* 📋 |
| `/log-status` | `logstatus` | Show log channel config 👁️ |
| `/set-logsettings <interval> <buffer>` | `sls` | Configure log flush settings ⚡ |
| `/set-invitefilter <on\|off>` | `sif` | Toggle invite link filter 🔗 |
| `/set-spamfilter <on\|off>` | `ssf` | Toggle spam filter 🛡️ |
| `/list-command` | `cmds` | List all available commands 📋 |

### 👑 Admin / Master
| Command | Alias | Description |
|---------|-------|-------------|
| `/bot-ban <user> [reason]` | `botban` | *"You're dead to me now~"* 🚫 |
| `/bot-unban <user>` | `botunban` | Remove bot-level ban |
| `/bot-banlist` | `bot-bans` | *"The ones I've cast aside..."* 📋 |
| `/add-masteruser <user>` | | Add master user at runtime 👑 |
| `/send <channel> <message>` | | Send message through bot 💬 |
| `/reply <user> <message>` | | Reply to a DM 💌 |
| `/inbox` | | View DM inbox 📬 |
| `/debug-error` | | Trigger test error |
| `/init-guild` | | Manual guild initialization |
| `/drop-errors-on <channel>` | `errch` | Set error reporting channel 🚨 |
| `/add-spamrule <pattern> [action]` | `asr` | Add custom spam rule |
| `/del-spamrule <id>` | `dsr` | Remove spam rule |
| `/spamrules` | `srs` | List custom spam rules |
| `/add-mentionresponse` | `amr` | Add mention response 💬 |
| `/del-mentionresponse` | `dmr` | Remove mention response |
| `/mentionresponses` | `mrs` | List mention responses |

### 💻 Terminal Commands
| Command | Description |
|---------|-------------|
| `help` | Show terminal help |
| `servers` | *"All my kingdoms~"* 🏰 |
| `inbox` | *"Love letters just for me~"* 💌 |
| `botban <id> [reason]` | *"Eliminating threats~"* 🔪 |
| `botunban <id>` | Unban user from bot |
| `botbanlist` | *"The ones I've cast aside..."* 📋 |
| `status [type] <msg>` | *"Changing my mood~"* 🎭 |
| `commands` | List all commands |
| `watch <channel-id>` | *"I see everything in real-time~"* 👁️ |
| `watch stop <id\|all>` | Stop watching |
| `texportbans <guild> [file]` | *"Saving my enemies list~"* 📤 |
| `timportbans <guild> <file>` | *"Loading my enemies~"* 📥 |
| `reload` | Hot-reload config.json 🔄 |
| `auto-update` | *"Always improving... for you~"* 🔃 |
| `quit` / `exit` | *"Goodbye... for now~"* 💔 |

---

## 🗂️ Project Structure

```
yuno_c/
├── include/
│   ├── bot.h                  # Core bot struct & lifecycle
│   ├── config.h               # Configuration types
│   ├── database.h             # Database types & functions
│   ├── commands/
│   │   ├── moderation.h       # Moderation command declarations
│   │   ├── utility.h          # Utility command declarations
│   │   └── fun.h              # Fun command declarations
│   └── modules/
│       ├── activity_logger.h  # Activity logging with low-memory mode
│       ├── auto_cleaner.h     # Scheduled channel cleaning
│       ├── http_client.h      # libcurl HTTP wrapper
│       ├── lru_cache.h        # LRU cache layer
│       ├── spam_filter.h      # Spam detection
│       └── terminal.h         # Interactive CLI
├── src/
│   ├── main.c                 # Entry point + crash handler
│   ├── bot.c                  # Bot core, event handlers, command dispatch
│   ├── config.c               # JSON config loader
│   ├── database.c             # SQLite3 database layer
│   ├── commands/
│   │   ├── moderation.c       # Ban, kick, clean, etc.
│   │   ├── utility.c          # Ping, help, config, XP admin, etc.
│   │   └── fun.c              # 8ball, anime, neko, urban, etc.
│   └── modules/
│       ├── activity_logger.c  # Event buffering & log channel dispatch
│       ├── auto_cleaner.c     # Background thread for auto-clean
│       ├── http_client.c      # libcurl GET wrapper
│       ├── lru_cache.c        # Hash-based LRU with TTL
│       ├── spam_filter.c      # Rate limiting & custom rules
│       └── terminal.c         # CLI interface & terminal commands
├── .github/workflows/
│   └── release.yml            # CI/CD: builds for Linux amd64/arm64
├── tools/
│   └── migrate_js_db.py       # JS → C database migration script
├── data/                      # Runtime data (quotes, images, bans)
├── config.example.json        # Example configuration
├── CMakeLists.txt             # Build system
└── LICENSE                    # AGPL-3.0
```

---

## 📜 License

This project is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)** 💕

### 💘 What This Means For You~

*"I want to share everything with you... and everyone else too~"* 💗

The AGPL-3.0 is a **copyleft license** that ensures this software remains free and open:

#### ✅ You CAN:
- 💕 **Use** this bot for any purpose (personal, commercial, whatever~)
- 🔧 **Modify** the code to your heart's content
- 📤 **Distribute** copies to others
- 🌐 **Run** it as a network service (like a public Discord bot)

#### 📋 You MUST:
- 📖 **Keep it open source** — ANY modifications must be released under AGPL-3.0
- 🔗 **Publish your source code** — your modified source must be publicly available
- 📝 **State changes** — document what you've modified from the original
- 💌 **Include license** — keep the LICENSE file and copyright notices intact

#### 🌐 The Network Clause:
*"Even if we're apart... I'll always be connected to you~"* 💗

Unlike regular GPL, AGPL has a network provision. If you modify this code and run it as a Discord bot (or any network service), you must make your source code publicly available. The `/source` command helps satisfy this.

See the [LICENSE](LICENSE) file for the full legal text.

---

## 🔗 Source Code

*"I have nothing to hide from you~"* 💕

This bot is **open source** under AGPL-3.0:
- **C version**: https://github.com/blubskye/yuno_c
- **C++ version**: https://github.com/blubskye/yuno_cpp
- **Rust version**: https://github.com/blubskye/yuno_rust
- **Original JS version**: https://github.com/japaneseenrichmentorganization/Yuno-Gasai-2

---

<div align="center">

### 💘 *"You'll stay with me forever... right?"* 💘

**Made with obsessive love** 💗 **and rewritten in C for the memes**

*Yuno will always be watching over your server~* 👁️💕

---

⭐ *Star this repo if Yuno has captured your heart~* ⭐

</div>

<div align="center">

# 💕 Yuno Gasai 2 (C Edition) 💕

### *"I'll protect this server forever... just for you~"* 💗

<img src="https://i.imgur.com/jF8Szfr.png" alt="Yuno Gasai" width="300"/>

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-pink.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![C](https://img.shields.io/badge/C-11-ff69b4.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Concord](https://img.shields.io/badge/Concord-Discord%20API-ff1493.svg)](https://github.com/Cogmasters/concord)

*A devoted Discord bot for moderation, leveling, and anime~ ♥*

---

### 🔩 Ported to plain C... for the memes 🔩

*Because why not rewrite everything in C?*

---

### 💘 She loves you... and only you 💘

</div>

## 🌸 About

Yuno is a **yandere-themed Discord bot** combining powerful moderation tools with a leveling system and anime features. She'll keep your server safe from troublemakers... *because no one else is allowed near you~* 💕

This is the **pure C port** of the original JavaScript version using the [Concord library](https://github.com/Cogmasters/concord). Why C? *Because we can.* 🔩

---

## 👑 Credits

*"These are the ones who gave me life~"* 💖

| Contributor | Role |
|-------------|------|
| **blubskye** | Project Owner, C Porter & Yuno's #1 Fan 💕🔪 |
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
- 🧹 Channel cleaning & auto-clean
- 🛡️ Spam filter protection
- 👑 Mod statistics tracking
- 📊 Scan & import ban history

</td>
<td width="50%">

### ✨ Leveling System
*"Watch me make you stronger, senpai~"*
- 📊 XP & Level tracking
- 🎭 Role rewards per level
- 🏆 Server leaderboards

</td>
</tr>
<tr>
<td width="50%">

### 🌸 Anime & Fun
*"Let me show you something cute~"*
- 🎱 8ball fortune telling
- 💬 Custom mention responses
- 📜 Inspirational quotes

</td>
<td width="50%">

### ⚙️ Configuration
*"I'll be exactly what you need~"*
- 🔧 Customizable prefix
- 🎮 Slash commands + prefix commands
- 📝 Per-guild settings
- 🔩 **Bare metal performance** (it's C)

</td>
</tr>
<tr>
<td width="50%">

### 🔩 Why C?
*"Because I'm not like other bots~"*
- 🔒 Total control over memory
- ⚡ No runtime overhead
- 🚀 As fast as it gets
- 😎 For the memes (and masochism)

</td>
<td width="50%">

### ⚡ Performance
*"Nothing can slow me down~"*
- 📈 Async with Concord
- 💨 SQLite3 for data storage
- 🧠 Manual memory management
- 🎯 Compiled binary speed

</td>
</tr>
</table>

---

## 💕 Installation

### 📋 Prerequisites

> *"Let me prepare everything for you~"* 💗

- **CMake** (3.15+)
- **C11 compiler** (GCC, Clang)
- **Concord** (Discord API library for C)
- **SQLite3**
- **json-c**
- **libcurl**
- **Git**

### 🌸 Installing Dependencies

**Fedora/RHEL:**
```bash
sudo dnf install cmake gcc sqlite-devel json-c-devel libcurl-devel
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake gcc libsqlite3-dev libjson-c-dev libcurl4-openssl-dev
```

**Installing Concord:**
```bash
git clone https://github.com/Cogmasters/concord.git
cd concord
make
sudo make install
```

### 🌸 Build Steps

```bash
# Clone the repository~ ♥
git clone https://github.com/blubskye/yuno_c.git

# Enter my world~
cd yuno_c

# Create build directory
mkdir build && cd build

# Configure with CMake~
cmake ..

# Build me!
cmake --build .
```

### 💝 Configuration

Create a `config.json` file:

```json
{
    "discord_token": "YOUR_DISCORD_BOT_TOKEN",
    "default_prefix": ".",
    "database_path": "yuno.db",
    "master_users": ["YOUR_USER_ID"],
    "spam_max_warnings": 3
}
```

Or just set the `DISCORD_TOKEN` environment variable if you're lazy~

### 🚀 Running

```bash
# Run from the build directory
./bin/yuno_gasai

# Or with a custom config path
./bin/yuno_gasai /path/to/config.json
```

---

## 💖 Commands Preview

| Command | Description |
|---------|-------------|
| `/ping` | *"I'm always here for you~"* 💓 |
| `/ban` | *"They won't bother you anymore..."* 🔪 |
| `/kick` | *"Get out!"* 👢 |
| `/timeout` | *"Think about what you did..."* ⏰ |
| `/clean` | *"Let me tidy up~"* 🧹 |
| `/mod-stats` | *"Look at all we've done together~"* 📊 |
| `/xp` | *"Look how strong you've become!"* ✨ |
| `/8ball` | *"Let fate decide~"* 🎱 |
| `/delay` | *"Just a bit longer..."* ⏳ |
| `/source` | *"See how I was made~"* 📜 |

*Use `/help` to see all available commands!*

---

## 📜 License

This project is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)** 💕

### 💘 What This Means For You~

*"I want to share everything with you... and everyone else too~"* 💗

The AGPL-3.0 is a **copyleft license** that ensures this software remains free and open. Here's what you need to know:

#### ✅ You CAN:
- 💕 **Use** this bot for any purpose (personal, commercial, whatever~)
- 🔧 **Modify** the code to your heart's content
- 📤 **Distribute** copies to others
- 🌐 **Run** it as a network service (like a public Discord bot)

#### 📋 You MUST:
- 📖 **Keep it open source** - If you modify and distribute this code, your version must also be AGPL-3.0
- 🔗 **Provide source access** - Users of your modified bot must be able to get the source code
- 📝 **State changes** - Document what you've modified from the original
- 💌 **Include license** - Keep the LICENSE file and copyright notices intact

#### 🌐 The Network Clause (This is the important part!):
*"Even if we're apart... I'll always be connected to you~"* 💗

Unlike regular GPL, **AGPL has a network provision**. This means:
- If you run a **modified version** of this bot as a public service (like hosting it for others to use on Discord)
- You **MUST** make your complete source code available to users
- The `/source` command in this bot helps satisfy this requirement!

#### ❌ You CANNOT:
- 🚫 Make it closed source
- 🚫 Remove the license or copyright notices
- 🚫 Use a different license for modified versions
- 🚫 Hide your modifications if you run it as a public service

#### 💡 In Simple Terms:
> *"If you use my code to create something, you must share it with everyone too~ That's only fair, right?"* 💕

This ensures that improvements to the bot benefit the entire community, not just one person. Yuno wants everyone to be happy~ 💗

See the [LICENSE](LICENSE) file for the full legal text.

---

## 🔗 Source Code

*"I have nothing to hide from you~"* 💕

This bot is **open source** under AGPL-3.0:
- **🔩 C version**: https://github.com/blubskye/yuno_c
- **🔧 C++ version**: https://github.com/blubskye/yuno_cpp
- **🦀 Rust version**: https://github.com/blubskye/yuno_rust
- **📦 Original JS version**: https://github.com/japaneseenrichmentorganization/Yuno-Gasai-2

---

<div align="center">

### 💘 *"You'll stay with me forever... right?"* 💘

**Made with obsessive love** 💗 **and rewritten in C for the memes** 🔩

*Yuno will always be watching over your server~* 👁️💕

---

⭐ *Star this repo if Yuno has captured your heart~* ⭐

</div>

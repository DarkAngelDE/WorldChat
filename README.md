# 🌐 WorldChat — Global Cross-Zone Chat for WoW Private Servers

> A lightweight, plug-and-play C++ script that adds a **server-wide chat channel** to your World of Warcraft private server. Players send messages with `.w <message>` and everyone online sees them — across all zones, realms, and instances.

---

## ✨ Features

- **Global broadcast** — Messages are delivered to all online players regardless of zone or map
- **Faction icons** — Alliance 🔵 / Horde 🔴 icons displayed automatically next to each name
- **Class-colored names** — Each player's name is colored by their class (e.g. Paladin = pink, Mage = blue)
- **GM badge** — Players with `SEC_MODERATOR` or above get a Blizzard icon next to their name
- **Link blocker** — Automatically mutes players who attempt to post URLs or item/spell hyperlinks
- **Played time gate** — New characters must accumulate playtime before they can chat (configurable)
- **Anti-spam cooldown** — Short mute applied after each message to prevent flooding
- **Per-player opt-out** — Players can disable WorldChat so they don't see messages (`.wc off`)
- **No restarts needed** for SQL table creation — just run the SQL and drop the script in

---

## 📋 Commands

| Command | Description |
|---|---|
| `.w <message>` | Send a message to all WorldChat participants |
| `.wc on` | Enable WorldChat (receive and send messages) — **default** |
| `.wc off` | Disable WorldChat (you won't see any messages) |
| `.wc status` | Show whether your WorldChat is currently on or off |

---

## ⚙️ Configuration

Open `WorldChat.cpp` and adjust the defines at the top:

```cpp
#define WC_PLAYED_TIME  1500   // Minimum played time in seconds before chatting is allowed (1500 = 25 minutes)
#define WC_MUTE_SPAM    5      // Anti-spam cooldown in seconds after each message
#define WC_MUTE_CENSOR  300    // Mute duration in seconds when a forbidden link is detected (300 = 5 minutes)
```

---

## 🗄️ SQL Setup

Run the following on your **character database**:

```sql
CREATE TABLE IF NOT EXISTS `worldchat_settings` (
  `guid`    INT(11) UNSIGNED NOT NULL,
  `enabled` TINYINT(1) NOT NULL DEFAULT 1,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

The full file is also available at [`sql/worldchat_setup.sql`](sql/worldchat_setup.sql).

> ℹ️ No manual data insertion needed — rows are created automatically when players first use `.wc on` or `.wc off`.

---

## 🔧 Installation

### Requirements

- A WoW private server running on a **Legion 7.x / MoPCore** compatible emulator  
  (tested with [AzerothCore](https://www.azerothcore.org/), MaNGOS, and MoPCore forks)
- C++ build environment for your emulator (CMake + your compiler of choice)

### Steps

1. **Copy** `WorldChat.cpp` into your server's script directory, e.g.:
   ```
   src/server/scripts/Custom/WorldChat.cpp
   ```

2. **Register** the script in your `custom_script_loader.cpp` (or equivalent):
   ```cpp
   void AddSC_WorldChat();
   // ...
   AddSC_WorldChat();
   ```

3. **Run** the SQL file against your character database:
   ```bash
   mysql -u root -p characters < sql/worldchat_setup.sql
   ```

4. **Rebuild** your server:
   ```bash
   cmake --build . --config Release
   ```

5. **Restart** your worldserver — WorldChat is ready!

---

## 🔒 Security Notes

WorldChat automatically **blocks** the following:

- HTTP/HTTPS/FTP URLs
- Common TLDs (`.com`, `.net`, `.de`, `.ru`, `.gg`, etc.)
- WoW hyperlink sequences (`|Hquest:`, `|Htrade:`, `|Hitem:`, etc.)
- Interface texture injections (`|TInterface...`)
- Newline characters

Any player caught posting forbidden content is **automatically muted** for `WC_MUTE_CENSOR` seconds via the login database mute system.

---

## 🖥️ Preview

```
[WorldChat] 🔵 [Warrior]Arthas: Hey everyone, anyone up for a raid tonight?
[WorldChat] 🔴 [Mage]Jaina: Count me in!
[WorldChat] 🔵 [Paladin]Uther: I'll tank, need a healer though.
```

*(Colors are rendered in-game via WoW's color code system)*

---

## 🌍 Community & Resources

> 💡 **Looking for more WoW emulator scripts, tools, and resources?**

### 🛒 [WoWEmu.de](https://wowemu.de) — The German WoW Emulator Marketplace
The go-to community marketplace for WoW private server scripts, configs, and tools.  
Buy and sell emulator scripts, get help from experienced developers, and find everything you need to run your own WoW private server.

- 📦 Scripts & modules for sale
- 💬 Community support
- 🏆 Server toplist
- 💼 Freelancer & jobs board
- 🪙 EmuCoins system for easy transactions

### 🔧 [Emervoid.de](https://emervoid.de) — WoW Emulator Development
Additional emulator tools, resources, and development support for WoW private server operators.

---

## 📄 License

This script is released free of charge for the WoW private server community.  

---

## 🐛 Issues & Contributions

Found a bug or want to contribute an improvement? Open an [Issue](../../issues) or submit a [Pull Request](../../pulls).  
For community discussion, head over to [WoWEmu.de](https://wowemu.de).

# GitDash 🕐
### A Time Machine for Geometry Dash Creators

> Never lose your level progress again. GitDash silently snapshots your level every time you save, and lets you roll back to any previous state with one tap.

---

## ✨ Features

- 🕐 **Automatic Snapshots** — captures your level every time you hit Save
- 📦 **Compressed Storage** — each snapshot is zlib-compressed, so 50 snapshots take minimal space
- 🔄 **One-tap Restore** — browse your timeline and roll back instantly from the editor pause menu
- 🗑️ **Smart Pruning** — configurable max snapshots per level, oldest auto-deleted when limit is reached
- ⚙️ **Settings** — control max snapshot count, autosave behaviour, and timestamp display from the Geode menu

---

## 📱 Platforms

| Platform | Status |
|----------|--------|
| Android (32-bit) | ✅ Supported |
| Android (64-bit) | ✅ Supported |
| Windows | ✅ Supported |
| macOS | ❌ Not supported |
| iOS | ❌ Not supported |

---

## 🚀 How to Use

1. Open any level in the **GD editor**
2. Build something and hit **Save** — GitDash captures a snapshot silently
3. Keep building. Save often.
4. If something goes wrong, open the **Editor Pause Menu** and tap the **GitDash (clock) button**
5. Use **← →** arrows to browse your snapshots
6. Tap **Restore** to roll back to that state

---

## ⚙️ Settings

| Setting | Default | Description |
|---------|---------|-------------|
| Max Snapshots Per Level | 50 | How many snapshots to keep. Oldest deleted when exceeded. |
| Snapshot on Autosave | Off | Also snapshot when GD's autosave triggers (not just manual saves) |
| Show Timestamps | On | Shows HH:MM:SS timestamp next to relative time in the Timeline |

---

## 🏗️ Building from Source

This project is built entirely via **GitHub Actions** — no local build environment needed.

### Requirements
- A GitHub account
- That's it — Actions handles everything else

### Steps
1. Fork or clone this repo
2. Push any change to `main` or `dev`
3. Go to **Actions tab** → wait for the workflow to complete
4. Download `GitDash.geode` from the **Artifacts** section

### Project Structure
```
GitDash/
├── .github/
│   └── workflows/
│       └── build.yml        # GitHub Actions CI — builds Win/Android
├── src/
│   ├── main.cpp             # Hooks: LevelEditorLayer + EditorPauseLayer
│   ├── SnapshotManager.hpp  # Snapshot data structure + manager class
│   ├── SnapshotManager.cpp  # Disk I/O, zlib compression, index management
│   ├── TimelinePopup.hpp    # Timeline UI declaration
│   └── TimelinePopup.cpp    # Timeline UI implementation
├── CMakeLists.txt           # Build configuration
├── mod.json                 # Geode mod manifest + settings
├── about.md                 # Mod store description
└── changelog.md             # Release notes
```

---

## 🔧 Tech Stack

| Component | Technology |
|-----------|-----------|
| Mod Framework | [Geode SDK](https://geode-sdk.org) v5.x |
| Language | C++20 |
| Compression | zlib |
| Build System | CMake + GitHub Actions |
| Target Game | Geometry Dash 2.2081 |

---

## 📋 Changelog

## v0.1.4
- Added iOS support
- Fixed crash when tapping objects after restoring a snapshot (color channel null pointer)
- Fixed crash caused by BetterEdit compatibility issue during snapshot comparison
- Optimized change detection — compares size before content to avoid unnecessary file reads
- Fixed snapshots being created even when level data didn't change

## v0.1.3
- Fixed crash when tapping on objects after restoring a snapshot
- Fixed freeze when exiting the editor after restoring
- Fixed snapshots being created even when nothing changed in the level
- Improved scene transition safety (0.3s deferred restore)

## v0.1.2
- Fixed crash when restoring a snapshot while a touch event was still being processed

### v0.1.1
- Snapshot on manual save
- Optional snapshot on autosave
- Timeline popup in editor pause menu
- Restore, Delete, Delete All actions
- Configurable max snapshot count
- zlib compression on all snapshot data
- Auto-pruning of old snapshots

---

## 👤 Author

**YallCode**
- GitHub: [@yallcode](https://github.com/yallcode)
- YouTube: [@YallaYCode](https://youtube.com/@YallaYCode)
- Discord: [discord.gg/yUe8kE5fRF](https://discord.gg/yUe8kE5fRF)
- X/Twitter: [@YallCode](https://twitter.com/YallCode)

---

## 📄 License

This project is open source. Feel free to learn from it, but please don't re-upload it as your own mod.

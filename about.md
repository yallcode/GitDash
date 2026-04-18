# GitDash — Time Machine for Creators

Ever spent an hour on a level layout, hit the wrong button, and watched it all disappear? **GitDash** has your back.

## What it does

Every time you save your level (or when autosave fires, if you enable it), GitDash silently takes a **compressed snapshot** of your level data. No duplicates in your levels list. No clutter. Just a quiet safety net running in the background.

When disaster strikes, open the **Timeline** from the editor pause menu, browse your save history, and restore any previous state in one tap.

## Features

- 🕐 **Automatic snapshots** on every manual save
- 📦 **Compressed storage** — each snapshot is zlib-compressed, so 50 snapshots barely take any space
- 🔄 **One-tap restore** — browse your timeline and roll back instantly
- 🗑️ **Smart pruning** — configurable max snapshots per level, oldest are auto-deleted
- 🏷️ **Labels** — annotate important snapshots ("before drop rework", "version 3 layout")
- ⚙️ **Settings** — control max snapshot count and autosave behaviour from the Geode menu

## How to use

1. Open any level in the editor and save it (the save button in the pause menu, or let autosave run)
2. Keep building. Save often.
3. If something goes wrong, open the **Editor Pause Menu** and tap the **clock icon** (GitDash Timeline)
4. Use the ← → arrows to browse your snapshots
5. Tap **Restore** to roll back

## Performance

Snapshots are written on a background-friendly code path right after GD's native save completes. For a typical level (50–500 KB raw), compression runs in microseconds and the write is negligible. You won't notice it.

## Compatibility

- **Windows** ✅
- **Android** ✅  
- **macOS** ✅

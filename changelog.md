# Changelog

## v0.1.2
- Fixed crash when restoring a snapshot while a touch event was still being processed (increased deferred scene transition delay to 0.5s)

## v0.1.1
- Fixed mod index submission (version tagging)

## v0.1.0 (initial release)
- Snapshot on manual save via `EditorPauseLayer::onSave`, `onSaveAndPlay`, `onSaveAndExit`
- Timeline popup in the editor pause menu (top-right Timeline button)
- Navigate snapshots with ← → arrows
- Restore, Delete, and Delete All actions
- Configurable max snapshot count per level (default: 50, range: 5–200)
- Optional snapshot on autosave (off by default)
- Timestamp display toggle in settings
- Automatic pruning of oldest snapshots when limit is reached
- Deferred scene transition to prevent touch-handler crashes
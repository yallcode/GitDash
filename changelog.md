# Changelog

## v0.1.1 (initial release)
- Snapshot on manual save (`LevelEditorLayer::saveLevel`)
- Optional snapshot on autosave (off by default)
- Timeline popup in EditorPauseLayer with ← → navigation
- Restore, Delete, and Delete All actions
- Configurable max snapshot count (default: 50)
- zlib compression on all snapshot data
- Automatic pruning of oldest snapshots when limit is reached
- Timestamp display toggle in settings

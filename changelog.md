# Changelog

## v0.1.3
- Fixed crash when tapping on objects after restoring a snapshot
- Fixed freeze when exiting the editor after restoring
- Fixed snapshots being created even when nothing changed in the level
- Improved scene transition safety (0.3s deferred restore)

## v0.1.2
- Fixed crash when restoring a snapshot while a touch event was still being processed

## v0.1.1
- Fixed mod index submission

## v0.1.0 (initial release)
- Snapshot on manual save (Save, Save and Play, Save and Exit)
- Timeline popup in the editor pause menu
- Navigate snapshots with ← → arrows
- Restore, Delete, Delete All actions
- Configurable max snapshot count (default 50, range 5–200)
- Optional snapshot on autosave (off by default)
- Timestamp display toggle in settings
- Automatic pruning of oldest snapshots when limit is reached
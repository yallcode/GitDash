#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <ctime>
#include <filesystem>

using namespace geode::prelude;
namespace fs = std::filesystem;

// ─── Data Structures ─────────────────────────────────────────────────────────

/**
 * Represents a single saved snapshot of a level.
 * The actual level data lives in a .snap file on disk;
 * this struct is the lightweight index entry kept in memory.
 */
struct Snapshot {
    std::time_t timestamp;   // Unix time when snapshot was taken
    std::string filename;    // e.g. "1700000000.snap"
    std::string label;       // Optional user label, empty by default
    size_t      dataSize;    // Compressed byte size (for display)

    // Returns a human-readable relative time string like "2 min ago"
    std::string relativeTimeString() const;

    // Returns "HH:MM:SS" formatted time string
    std::string timeString() const;
};

// ─── SnapshotManager ─────────────────────────────────────────────────────────

/**
 * Handles all disk I/O and in-memory state for GitDash snapshots.
 *
 * Usage:
 *   auto& mgr = SnapshotManager::get();
 *   mgr.takeSnapshot(levelID, levelString);
 *   auto snaps = mgr.getSnapshots(levelID);
 *   std::string restored = mgr.loadSnapshot(levelID, snaps[0]);
 */
class SnapshotManager {
public:
    // Singleton accessor
    static SnapshotManager& get();

    // Delete copy/move — singleton
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;

    /**
     * Creates a new snapshot for the given level.
     * Compresses levelString with zlib and writes it to disk.
     * Automatically prunes old snapshots if over the max limit.
     * @return true on success, false on I/O error
     */
    bool takeSnapshot(int levelID, const std::string& levelString);

    /**
     * Returns all snapshots for a level, sorted newest-first.
     * Loads the index from disk if not already cached.
     */
    std::vector<Snapshot> getSnapshots(int levelID);

    /**
     * Loads and decompresses the level string from a snapshot file.
     * @return The raw GD level string, or empty string on failure
     */
    std::string loadSnapshot(int levelID, const Snapshot& snap);

    /**
     * Deletes a single snapshot file + removes it from the index.
     */
    bool deleteSnapshot(int levelID, const Snapshot& snap);

    /**
     * Deletes ALL snapshots for a level. Used for cleanup.
     */
    void deleteAllSnapshots(int levelID);

    /**
     * Returns how many snapshots exist for a level.
     */
    size_t snapshotCount(int levelID);

private:
    SnapshotManager() = default;

    // Returns the directory for a specific level's snapshots.
    // Creates the directory if it doesn't exist.
    // Path: <Geode save dir>/gitdash/<levelID>/
    fs::path getLevelDir(int levelID);

    // Returns path to the index JSON file for a level.
    fs::path getIndexPath(int levelID);

    // Saves the in-memory snapshot list to the index JSON file.
    bool saveIndex(int levelID, const std::vector<Snapshot>& snapshots);

    // Loads the snapshot index from disk into memory.
    std::vector<Snapshot> loadIndex(int levelID);

    // Prunes snapshots over the user-configured limit (oldest deleted first).
    void pruneIfNeeded(int levelID);

    // Simple zlib compress/decompress wrappers
    static std::string compressString(const std::string& data);
    static std::string decompressString(const std::string& compressed);

    // Cache: levelID -> sorted snapshot list
    std::unordered_map<int, std::vector<Snapshot>> m_cache;
};

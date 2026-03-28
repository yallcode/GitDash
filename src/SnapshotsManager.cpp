#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>
#include <zlib.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>   // Bundled with Geode SDK

using namespace geode::prelude;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ─── Snapshot helpers ────────────────────────────────────────────────────────

std::string Snapshot::relativeTimeString() const {
    auto now = std::time(nullptr);
    auto diff = static_cast<long long>(now - timestamp);

    if (diff < 60)          return std::to_string(diff) + "s ago";
    if (diff < 3600)        return std::to_string(diff / 60) + " min ago";
    if (diff < 86400)       return std::to_string(diff / 3600) + "h ago";
    return std::to_string(diff / 86400) + "d ago";
}

std::string Snapshot::timeString() const {
    std::tm* t = std::localtime(&timestamp);
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(2) << t->tm_hour << ":"
       << std::setw(2) << t->tm_min  << ":"
       << std::setw(2) << t->tm_sec;
    return ss.str();
}

// ─── Singleton ───────────────────────────────────────────────────────────────

SnapshotManager& SnapshotManager::get() {
    static SnapshotManager instance;
    return instance;
}

// ─── Path helpers ─────────────────────────────────────────────────────────────

fs::path SnapshotManager::getLevelDir(int levelID) {
    auto dir = Mod::get()->getSaveDir() / "snapshots" / std::to_string(levelID);
    if (!fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            log::error("[GitDash] Failed to create snapshot dir: {}", ec.message());
        }
    }
    return dir;
}

fs::path SnapshotManager::getIndexPath(int levelID) {
    return getLevelDir(levelID) / "index.json";
}

// ─── Snapshot index I/O ───────────────────────────────────────────────────────

std::vector<Snapshot> SnapshotManager::loadIndex(int levelID) {
    auto path = getIndexPath(levelID);
    std::vector<Snapshot> result;

    if (!fs::exists(path)) return result;

    std::ifstream f(path);
    if (!f.is_open()) {
        log::error("[GitDash] Could not open index: {}", path.string());
        return result;
    }

    try {
        json j;
        f >> j;
        for (auto& entry : j) {
            Snapshot s;
            s.timestamp = entry.at("timestamp").get<std::time_t>();
            s.filename  = entry.at("filename").get<std::string>();
            s.label     = entry.value("label", "");
            s.dataSize  = entry.value("dataSize", 0u);
            result.push_back(s);
        }
    } catch (const std::exception& e) {
        log::error("[GitDash] Failed to parse index JSON: {}", e.what());
    }

    // Sort newest first
    std::sort(result.begin(), result.end(), [](const Snapshot& a, const Snapshot& b) {
        return a.timestamp > b.timestamp;
    });

    return result;
}

bool SnapshotManager::saveIndex(int levelID, const std::vector<Snapshot>& snapshots) {
    auto path = getIndexPath(levelID);
    json j = json::array();

    for (const auto& s : snapshots) {
        j.push_back({
            {"timestamp", s.timestamp},
            {"filename",  s.filename},
            {"label",     s.label},
            {"dataSize",  s.dataSize}
        });
    }

    std::ofstream f(path);
    if (!f.is_open()) {
        log::error("[GitDash] Could not write index: {}", path.string());
        return false;
    }

    f << j.dump(2);
    return true;
}

// ─── zlib helpers ─────────────────────────────────────────────────────────────

std::string SnapshotManager::compressString(const std::string& data) {
    uLongf compressedSize = compressBound(data.size());
    std::string compressed(compressedSize, '\0');

    int result = compress2(
        reinterpret_cast<Bytef*>(compressed.data()),
        &compressedSize,
        reinterpret_cast<const Bytef*>(data.data()),
        data.size(),
        Z_BEST_COMPRESSION
    );

    if (result != Z_OK) {
        log::error("[GitDash] zlib compress failed with code {}", result);
        return "";
    }

    compressed.resize(compressedSize);
    return compressed;
}

std::string SnapshotManager::decompressString(const std::string& compressed) {
    // We don't know the original size, so decompress in growing chunks
    std::string decompressed;
    decompressed.resize(compressed.size() * 4);

    z_stream stream{};
    inflateInit(&stream);
    stream.next_in   = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    stream.avail_in  = static_cast<uInt>(compressed.size());

    int ret = Z_OK;
    while (ret != Z_STREAM_END) {
        stream.next_out  = reinterpret_cast<Bytef*>(decompressed.data() + stream.total_out);
        stream.avail_out = static_cast<uInt>(decompressed.size() - stream.total_out);

        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_BUF_ERROR || stream.avail_out == 0) {
            decompressed.resize(decompressed.size() * 2);
            continue;
        }
        if (ret != Z_OK && ret != Z_STREAM_END) {
            log::error("[GitDash] zlib decompress failed with code {}", ret);
            inflateEnd(&stream);
            return "";
        }
    }

    decompressed.resize(stream.total_out);
    inflateEnd(&stream);
    return decompressed;
}

// ─── Core API ─────────────────────────────────────────────────────────────────

bool SnapshotManager::takeSnapshot(int levelID, const std::string& levelString) {
    if (levelString.empty()) {
        log::warn("[GitDash] takeSnapshot called with empty level string — skipping.");
        return false;
    }

    auto dir       = getLevelDir(levelID);
    auto timestamp = std::time(nullptr);
    auto filename  = std::to_string(timestamp) + ".snap";
    auto filepath  = dir / filename;

    // Compress
    std::string compressed = compressString(levelString);
    if (compressed.empty()) return false;

    // Write to disk
    std::ofstream f(filepath, std::ios::binary);
    if (!f.is_open()) {
        log::error("[GitDash] Failed to open snap file for writing: {}", filepath.string());
        return false;
    }
    f.write(compressed.data(), compressed.size());
    f.close();

    // Update in-memory cache + index
    Snapshot snap;
    snap.timestamp = timestamp;
    snap.filename  = filename;
    snap.label     = "";
    snap.dataSize  = compressed.size();

    auto& cached = m_cache[levelID];
    // Reload from disk to ensure accuracy if not yet cached
    if (cached.empty()) cached = loadIndex(levelID);
    cached.insert(cached.begin(), snap);  // Insert at front (newest first)

    saveIndex(levelID, cached);
    pruneIfNeeded(levelID);

    log::info("[GitDash] Snapshot saved: {} ({} bytes compressed, {} bytes raw)",
        filename, compressed.size(), levelString.size());

    return true;
}

std::vector<Snapshot> SnapshotManager::getSnapshots(int levelID) {
    auto it = m_cache.find(levelID);
    if (it != m_cache.end()) return it->second;

    auto loaded = loadIndex(levelID);
    m_cache[levelID] = loaded;
    return loaded;
}

std::string SnapshotManager::loadSnapshot(int levelID, const Snapshot& snap) {
    auto filepath = getLevelDir(levelID) / snap.filename;

    std::ifstream f(filepath, std::ios::binary);
    if (!f.is_open()) {
        log::error("[GitDash] Could not open snap file: {}", filepath.string());
        return "";
    }

    std::string compressed((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    f.close();

    std::string decompressed = decompressString(compressed);
    if (decompressed.empty()) {
        log::error("[GitDash] Decompression failed for: {}", snap.filename);
        return "";
    }

    log::info("[GitDash] Loaded snapshot: {} ({} bytes)", snap.filename, decompressed.size());
    return decompressed;
}

bool SnapshotManager::deleteSnapshot(int levelID, const Snapshot& snap) {
    auto filepath = getLevelDir(levelID) / snap.filename;

    std::error_code ec;
    fs::remove(filepath, ec);
    if (ec) {
        log::error("[GitDash] Failed to delete snap file: {}", ec.message());
        return false;
    }

    auto& cached = m_cache[levelID];
    cached.erase(
        std::remove_if(cached.begin(), cached.end(), [&snap](const Snapshot& s) {
            return s.filename == snap.filename;
        }),
        cached.end()
    );

    saveIndex(levelID, cached);
    return true;
}

void SnapshotManager::deleteAllSnapshots(int levelID) {
    auto dir = getLevelDir(levelID);
    std::error_code ec;
    fs::remove_all(dir, ec);
    m_cache.erase(levelID);
    log::info("[GitDash] Deleted all snapshots for level {}", levelID);
}

size_t SnapshotManager::snapshotCount(int levelID) {
    return getSnapshots(levelID).size();
}

void SnapshotManager::pruneIfNeeded(int levelID) {
    int maxSnaps = Mod::get()->getSettingValue<int64_t>("max-snapshots");
    auto& cached = m_cache[levelID];

    while (static_cast<int>(cached.size()) > maxSnaps) {
        // Remove oldest (last in the newest-first sorted list)
        auto& oldest = cached.back();
        auto filepath = getLevelDir(levelID) / oldest.filename;
        std::error_code ec;
        fs::remove(filepath, ec);
        log::info("[GitDash] Pruned old snapshot: {}", oldest.filename);
        cached.pop_back();
    }

    saveIndex(levelID, cached);
}
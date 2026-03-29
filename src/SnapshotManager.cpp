#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>
#include <matjson.hpp>
#include <zlib.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

using namespace geode::prelude;
namespace fs = std::filesystem;

// ─── Snapshot helpers ────────────────────────────────────────────────────────

std::string Snapshot::relativeTimeString() const {
    auto now = std::time(nullptr);
    auto diff = static_cast<long long>(now - timestamp);
    if (diff < 60)      return std::to_string(diff) + "s ago";
    if (diff < 3600)    return std::to_string(diff / 60) + " min ago";
    if (diff < 86400)   return std::to_string(diff / 3600) + "h ago";
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
        if (ec) log::error("[GitDash] Failed to create dir: {}", ec.message());
    }
    return dir;
}

fs::path SnapshotManager::getIndexPath(int levelID) {
    return getLevelDir(levelID) / "index.json";
}

// ─── Index I/O ───────────────────────────────────────────────────────────────

std::vector<Snapshot> SnapshotManager::loadIndex(int levelID) {
    auto path = getIndexPath(levelID);
    std::vector<Snapshot> result;
    if (!fs::exists(path)) return result;

    std::ifstream f(path);
    if (!f.is_open()) { log::error("[GitDash] Could not open index"); return result; }

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();

    auto parsed = matjson::parse(content);
    if (!parsed.has_value() || !parsed->isArray()) {
        log::error("[GitDash] Failed to parse index JSON");
        return result;
    }

    for (auto& entry : parsed->asArray().unwrap()) {
        if (!entry.isObject()) continue;
        Snapshot s;
        s.timestamp = static_cast<std::time_t>(entry["timestamp"].asInt().unwrapOr(0));
        s.filename  = entry["filename"].asString().unwrapOr("");
        s.label     = entry["label"].asString().unwrapOr("");
        s.dataSize  = static_cast<size_t>(entry["dataSize"].asInt().unwrapOr(0));
        if (!s.filename.empty()) result.push_back(s);
    }

    std::sort(result.begin(), result.end(), [](const Snapshot& a, const Snapshot& b) {
        return a.timestamp > b.timestamp;
    });
    return result;
}

bool SnapshotManager::saveIndex(int levelID, const std::vector<Snapshot>& snapshots) {
    auto path = getIndexPath(levelID);
    matjson::Array arr;
    for (const auto& s : snapshots) {
        matjson::Value obj = matjson::Object{};
        obj["timestamp"] = static_cast<int>(s.timestamp);
        obj["filename"]  = s.filename;
        obj["label"]     = s.label;
        obj["dataSize"]  = static_cast<int>(s.dataSize);
        arr.push_back(obj);
    }
    std::ofstream f(path);
    if (!f.is_open()) { log::error("[GitDash] Could not write index"); return false; }
    f << matjson::Value(arr).dump(2);
    return true;
}

// ─── zlib ────────────────────────────────────────────────────────────────────

std::string SnapshotManager::compressString(const std::string& data) {
    uLongf size = compressBound(data.size());
    std::string out(size, '\0');
    int r = compress2(reinterpret_cast<Bytef*>(out.data()), &size,
        reinterpret_cast<const Bytef*>(data.data()), data.size(), Z_BEST_COMPRESSION);
    if (r != Z_OK) { log::error("[GitDash] compress failed: {}", r); return ""; }
    out.resize(size);
    return out;
}

std::string SnapshotManager::decompressString(const std::string& compressed) {
    std::string out;
    out.resize(compressed.size() * 4);
    z_stream s{};
    inflateInit(&s);
    s.next_in  = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    s.avail_in = static_cast<uInt>(compressed.size());
    int ret = Z_OK;
    while (ret != Z_STREAM_END) {
        s.next_out  = reinterpret_cast<Bytef*>(out.data() + s.total_out);
        s.avail_out = static_cast<uInt>(out.size() - s.total_out);
        ret = inflate(&s, Z_NO_FLUSH);
        if (ret == Z_BUF_ERROR || s.avail_out == 0) { out.resize(out.size() * 2); continue; }
        if (ret != Z_OK && ret != Z_STREAM_END) {
            log::error("[GitDash] decompress failed: {}", ret);
            inflateEnd(&s); return "";
        }
    }
    out.resize(s.total_out);
    inflateEnd(&s);
    return out;
}

// ─── Core API ─────────────────────────────────────────────────────────────────

bool SnapshotManager::takeSnapshot(int levelID, const std::string& levelString) {
    if (levelString.empty()) { log::warn("[GitDash] Empty level string, skipping."); return false; }

    auto dir       = getLevelDir(levelID);
    auto timestamp = std::time(nullptr);
    auto filename  = std::to_string(timestamp) + ".snap";
    auto filepath  = dir / filename;

    auto compressed = compressString(levelString);
    if (compressed.empty()) return false;

    std::ofstream f(filepath, std::ios::binary);
    if (!f.is_open()) { log::error("[GitDash] Cannot write snap file"); return false; }
    f.write(compressed.data(), compressed.size());
    f.close();

    Snapshot snap;
    snap.timestamp = timestamp;
    snap.filename  = filename;
    snap.label     = "";
    snap.dataSize  = compressed.size();

    auto& cached = m_cache[levelID];
    if (cached.empty()) cached = loadIndex(levelID);
    cached.insert(cached.begin(), snap);
    saveIndex(levelID, cached);
    pruneIfNeeded(levelID);

    log::info("[GitDash] Snapshot #{} saved: {} ({} bytes compressed)",
        cached.size(), filename, compressed.size());
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
    if (!f.is_open()) { log::error("[GitDash] Cannot open snap file: {}", snap.filename); return ""; }
    std::string compressed((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    auto decompressed = decompressString(compressed);
    if (decompressed.empty()) { log::error("[GitDash] Decompression failed"); return ""; }
    log::info("[GitDash] Loaded snapshot: {} ({} bytes)", snap.filename, decompressed.size());
    return decompressed;
}

bool SnapshotManager::deleteSnapshot(int levelID, const Snapshot& snap) {
    std::error_code ec;
    fs::remove(getLevelDir(levelID) / snap.filename, ec);
    if (ec) { log::error("[GitDash] Delete failed: {}", ec.message()); return false; }
    auto& cached = m_cache[levelID];
    cached.erase(std::remove_if(cached.begin(), cached.end(),
        [&snap](const Snapshot& s){ return s.filename == snap.filename; }), cached.end());
    saveIndex(levelID, cached);
    return true;
}

void SnapshotManager::deleteAllSnapshots(int levelID) {
    std::error_code ec;
    fs::remove_all(getLevelDir(levelID), ec);
    m_cache.erase(levelID);
    log::info("[GitDash] Deleted all snapshots for level {}", levelID);
}

size_t SnapshotManager::snapshotCount(int levelID) {
    return getSnapshots(levelID).size();
}

void SnapshotManager::pruneIfNeeded(int levelID) {
    int maxSnaps = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-snapshots"));
    auto& cached = m_cache[levelID];
    while (static_cast<int>(cached.size()) > maxSnaps) {
        std::error_code ec;
        fs::remove(getLevelDir(levelID) / cached.back().filename, ec);
        log::info("[GitDash] Pruned: {}", cached.back().filename);
        cached.pop_back();
    }
    saveIndex(levelID, cached);
}

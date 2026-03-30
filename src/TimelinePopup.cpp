#include "TimelinePopup.hpp"
#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// ─── Factory ──────────────────────────────────────────────────────────────────

TimelinePopup* TimelinePopup::create(LevelEditorLayer* editorLayer) {
    auto ret = new TimelinePopup();
    if (ret && ret->init(editorLayer)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// ─── Init ─────────────────────────────────────────────────────────────────────

bool TimelinePopup::init(LevelEditorLayer* editorLayer) {
    if (!FLAlertLayer::init(nullptr, "GitDash Timeline", "OK", nullptr, 360.f))
        return false;

    m_editorLayer  = editorLayer;
    m_levelID      = editorLayer->m_level->m_levelID.value();
    m_snapshots    = SnapshotManager::get().getSnapshots(m_levelID);
    m_currentIndex = 0;

    // Hide the default OK button — we use our own
    if (m_button1) m_button1->setVisible(false);

    buildUI();
    return true;
}

// ─── UI ───────────────────────────────────────────────────────────────────────

void TimelinePopup::buildUI() {
    auto winSize = CCDirector::get()->getWinSize();
    float cx     = winSize.width  / 2.f;
    float cy     = winSize.height / 2.f;

    // Background layer
    auto bg = CCLayerColor::create({ 0, 0, 0, 180 });
    bg->setContentSize({ 340.f, 200.f });
    bg->setPosition({ cx - 170.f, cy - 100.f });
    this->addChild(bg, 0);

    // ── Empty state ──────────────────────────────────────────────────────
    if (m_snapshots.empty()) {
        m_emptyLabel = CCLabelBMFont::create(
            "No snapshots yet.\nSave your level to create one!",
            "bigFont.fnt"
        );
        m_emptyLabel->setScale(0.35f);
        m_emptyLabel->setAlignment(kCCTextAlignmentCenter);
        m_emptyLabel->setPosition({ cx, cy + 10.f });
        this->addChild(m_emptyLabel, 5);

        auto closeBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.7f),
            this, menu_selector(TimelinePopup::onClose)
        );
        auto menu = CCMenu::create(closeBtn, nullptr);
        menu->setPosition({ cx, cy - 70.f });
        this->addChild(menu, 5);
        return;
    }

    // ── Navigation row ────────────────────────────────────────────────────
    float navY = cy + 65.f;

    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setScale(0.7f);
    auto prevBtn = CCMenuItemSpriteExtra::create(
        prevSpr, this, menu_selector(TimelinePopup::onPrev)
    );

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setScale(0.7f);
    nextSpr->setFlipX(true);
    auto nextBtn = CCMenuItemSpriteExtra::create(
        nextSpr, this, menu_selector(TimelinePopup::onNext)
    );

    auto navMenu = CCMenu::create(prevBtn, nextBtn, nullptr);
    navMenu->setPosition({ cx, navY });
    navMenu->setLayout(RowLayout::create()->setGap(120.f));
    this->addChild(navMenu, 5);

    m_indexLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_indexLabel->setScale(0.45f);
    m_indexLabel->setPosition({ cx, navY });
    this->addChild(m_indexLabel, 5);

    // ── Info labels ───────────────────────────────────────────────────────
    m_timeLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_timeLabel->setScale(0.4f);
    m_timeLabel->setPosition({ cx, cy + 30.f });
    this->addChild(m_timeLabel, 5);

    m_sizeLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_sizeLabel->setScale(0.4f);
    m_sizeLabel->setColor({ 150, 150, 150 });
    m_sizeLabel->setPosition({ cx, cy + 10.f });
    this->addChild(m_sizeLabel, 5);

    // ── Buttons ───────────────────────────────────────────────────────────
    float btnY = cy - 55.f;

    auto restoreBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Restore", "goldFont.fnt", "GJ_button_01.png", 0.7f),
        this, menu_selector(TimelinePopup::onRestore)
    );
    auto deleteBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Delete", "bigFont.fnt", "GJ_button_06.png", 0.6f),
        this, menu_selector(TimelinePopup::onDelete)
    );
    auto deleteAllBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Delete All", "bigFont.fnt", "GJ_button_06.png", 0.55f),
        this, menu_selector(TimelinePopup::onDeleteAll)
    );
    auto closeBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_04.png", 0.6f),
        this, menu_selector(TimelinePopup::onClose)
    );

    auto btnMenu = CCMenu::create(restoreBtn, deleteBtn, deleteAllBtn, closeBtn, nullptr);
    btnMenu->setPosition({ cx, btnY });
    btnMenu->setLayout(RowLayout::create()->setGap(6.f));
    this->addChild(btnMenu, 5);

    refreshLabels();
}

void TimelinePopup::refreshLabels() {
    if (m_snapshots.empty()) return;
    const auto& snap = m_snapshots[m_currentIndex];
    int total = static_cast<int>(m_snapshots.size());

    if (m_indexLabel)
        m_indexLabel->setString(fmt::format("{}/{}", m_currentIndex + 1, total).c_str());

    if (m_timeLabel) {
        bool showTs = Mod::get()->getSettingValue<bool>("show-timestamps");
        std::string t = showTs
            ? fmt::format("{} ({})", snap.relativeTimeString(), snap.timeString())
            : snap.relativeTimeString();
        m_timeLabel->setString(t.c_str());
    }

    if (m_sizeLabel) {
        float kb = snap.dataSize / 1024.f;
        m_sizeLabel->setString(fmt::format("{:.1f} KB compressed", kb).c_str());
    }
}

// ─── Navigation ───────────────────────────────────────────────────────────────

void TimelinePopup::onPrev(CCObject*) {
    if (m_currentIndex > 0) { m_currentIndex--; refreshLabels(); }
}

void TimelinePopup::onNext(CCObject*) {
    if (m_currentIndex < static_cast<int>(m_snapshots.size()) - 1) {
        m_currentIndex++;
        refreshLabels();
    }
}

// ─── Actions ──────────────────────────────────────────────────────────────────

void TimelinePopup::onRestore(CCObject*) {
    if (m_snapshots.empty()) return;
    auto snap = m_snapshots[m_currentIndex];

    auto alert = FLAlertLayer::create(nullptr,
        "Restore Snapshot",
        fmt::format("Restore to <cy>{}</c>? Current unsaved changes will be lost.", snap.relativeTimeString()).c_str(),
        "Cancel", "Restore", 320.f
    );
    alert->m_button2->addClickEventListener([this, snap](CCObject*) {
        applySnapshot(snap);
    });
    alert->show();
}

void TimelinePopup::applySnapshot(const Snapshot& snap) {
    auto levelString = SnapshotManager::get().loadSnapshot(m_levelID, snap);
    if (levelString.empty()) {
        FLAlertLayer::create(nullptr, "Error", "Failed to load snapshot.", "OK", nullptr)->show();
        return;
    }

    m_editorLayer->m_level->m_levelString = levelString;
    m_editorLayer->loadLevelData();

    log::info("[GitDash] Restored snapshot: {}", snap.filename);
    onClose(nullptr);

    Notification::create(
        fmt::format("Restored: {}", snap.relativeTimeString()),
        NotificationIcon::Success
    )->show();
}

void TimelinePopup::onDelete(CCObject*) {
    if (m_snapshots.empty()) return;
    auto snap = m_snapshots[m_currentIndex];

    auto alert = FLAlertLayer::create(nullptr,
        "Delete Snapshot",
        fmt::format("Delete snapshot from {}?", snap.relativeTimeString()).c_str(),
        "Cancel", "Delete", 300.f
    );
    alert->m_button2->addClickEventListener([this, snap](CCObject*) {
        SnapshotManager::get().deleteSnapshot(m_levelID, snap);
        m_snapshots = SnapshotManager::get().getSnapshots(m_levelID);
        if (m_currentIndex >= static_cast<int>(m_snapshots.size()) && m_currentIndex > 0)
            m_currentIndex--;
        refreshLabels();
    });
    alert->show();
}

void TimelinePopup::onDeleteAll(CCObject*) {
    auto alert = FLAlertLayer::create(nullptr,
        "Delete All",
        "Permanently delete <cr>ALL snapshots</c> for this level?",
        "Cancel", "Delete All", 300.f
    );
    alert->m_button2->addClickEventListener([this](CCObject*) {
        SnapshotManager::get().deleteAllSnapshots(m_levelID);
        m_snapshots.clear();
        m_currentIndex = 0;
        onClose(nullptr);
        Notification::create("All snapshots deleted.", NotificationIcon::Warning)->show();
    });
    alert->show();
}

void TimelinePopup::onClose(CCObject*) {
    this->setKeyboardEnabled(false);
    this->removeFromParentAndCleanup(true);
}

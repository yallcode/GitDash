#include "TimelinePopup.hpp"
#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>

using namespace geode::prelude;

TimelinePopup* TimelinePopup::create(LevelEditorLayer* editorLayer) {
    auto ret = new TimelinePopup();
    if (ret && ret->init(editorLayer)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool TimelinePopup::init(LevelEditorLayer* editorLayer) {
    // Full FLAlertLayer::init signature (Geode 5):
    // delegate, title, desc, btn1, btn2, width, scroll, height, textScale
    if (!FLAlertLayer::init(nullptr, "GitDash Timeline", "", "x", nullptr, 340.f, false, 230.f, 1.f))
        return false;

    m_editorLayer  = editorLayer;
    m_levelID      = editorLayer->m_level->m_levelID.value();
    m_snapshots    = SnapshotManager::get().getSnapshots(m_levelID);
    m_currentIndex = 0;

    // Hide the default "x" close button - we place our own
    if (m_button1) m_button1->setVisible(false);

    buildUI();
    return true;
}

void TimelinePopup::buildUI() {
    auto winSize = CCDirector::get()->getWinSize();
    // All positions are in SCREEN coordinates
    // The popup background is centered at (winSize/2)
    // Popup is 340x230, so content area runs:
    //   X: cx ± 160
    //   Y: cy - 100 to cy + 90  (title bar takes ~40px at top)
    float cx = winSize.width  / 2.f;
    float cy = winSize.height / 2.f;

    // ── Empty state ───────────────────────────────────────────────────────
    if (m_snapshots.empty()) {
        auto lbl = CCLabelBMFont::create(
            "No snapshots yet!\nSave your level first.",
            "bigFont.fnt"
        );
        lbl->setScale(0.35f);
        lbl->setAlignment(kCCTextAlignmentCenter);
        lbl->setPosition({ cx, cy + 10.f });
        this->addChild(lbl, 10);

        auto closeBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.7f),
            this, menu_selector(TimelinePopup::onClose)
        );
        auto menu = CCMenu::create(closeBtn, nullptr);
        menu->setPosition({ cx, cy - 75.f });
        this->addChild(menu, 10);
        return;
    }

    // ── Index label (e.g. "1/3") ──────────────────────────────────────────
    m_indexLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_indexLabel->setScale(0.4f);
    m_indexLabel->setPosition({ cx, cy + 55.f });
    this->addChild(m_indexLabel, 10);

    // ── Nav arrows ────────────────────────────────────────────────────────
    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setScale(0.6f);
    auto prevBtn = CCMenuItemSpriteExtra::create(
        prevSpr, this, menu_selector(TimelinePopup::onPrev)
    );

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setScale(0.6f);
    nextSpr->setFlipX(true);
    auto nextBtn = CCMenuItemSpriteExtra::create(
        nextSpr, this, menu_selector(TimelinePopup::onNext)
    );

    auto navMenu = CCMenu::create(prevBtn, nextBtn, nullptr);
    navMenu->setPosition({ cx, cy + 55.f });
    navMenu->setLayout(RowLayout::create()->setGap(90.f));
    this->addChild(navMenu, 10);

    // ── Time + size labels ────────────────────────────────────────────────
    m_timeLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_timeLabel->setScale(0.38f);
    m_timeLabel->setPosition({ cx, cy + 25.f });
    this->addChild(m_timeLabel, 10);

    m_sizeLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_sizeLabel->setScale(0.38f);
    m_sizeLabel->setColor({ 180, 180, 180 });
    m_sizeLabel->setPosition({ cx, cy + 5.f });
    this->addChild(m_sizeLabel, 10);

    // ── Action buttons ────────────────────────────────────────────────────
    auto restoreBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Restore", "goldFont.fnt", "GJ_button_01.png", 0.65f),
        this, menu_selector(TimelinePopup::onRestore)
    );
    auto deleteBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Delete", "bigFont.fnt", "GJ_button_06.png", 0.55f),
        this, menu_selector(TimelinePopup::onDelete)
    );
    auto deleteAllBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Del All", "bigFont.fnt", "GJ_button_06.png", 0.5f),
        this, menu_selector(TimelinePopup::onDeleteAll)
    );
    auto closeBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_04.png", 0.55f),
        this, menu_selector(TimelinePopup::onClose)
    );

    auto btnMenu = CCMenu::create(restoreBtn, deleteBtn, deleteAllBtn, closeBtn, nullptr);
    btnMenu->setPosition({ cx, cy - 75.f });
    btnMenu->setLayout(RowLayout::create()->setGap(5.f));
    this->addChild(btnMenu, 10);

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
        m_sizeLabel->setString(fmt::format("{:.1f} KB", kb).c_str());
    }
}

void TimelinePopup::onPrev(CCObject*) {
    if (m_currentIndex > 0) { m_currentIndex--; refreshLabels(); }
}

void TimelinePopup::onNext(CCObject*) {
    if (m_currentIndex < static_cast<int>(m_snapshots.size()) - 1) {
        m_currentIndex++;
        refreshLabels();
    }
}

void TimelinePopup::onRestore(CCObject*) {
    if (m_snapshots.empty()) return;
    applySnapshot(m_snapshots[m_currentIndex]);
}

void TimelinePopup::applySnapshot(const Snapshot& snap) {
    auto levelString = SnapshotManager::get().loadSnapshot(m_levelID, snap);
    if (levelString.empty()) {
        FLAlertLayer::create(nullptr, "Error", "Failed to load snapshot.", "OK", nullptr)->show();
        return;
    }

    m_editorLayer->m_level->m_levelString = levelString;

    auto scene     = CCScene::create();
    auto newEditor = LevelEditorLayer::create(m_editorLayer->m_level, false);
    scene->addChild(newEditor);
    CCDirector::get()->replaceScene(CCTransitionFade::create(0.5f, scene));

    log::info("[GitDash] Restored snapshot: {}", snap.filename);
    Notification::create(
        fmt::format("Restored: {}", snap.relativeTimeString()),
        NotificationIcon::Success
    )->show();
}

void TimelinePopup::onDelete(CCObject*) {
    if (m_snapshots.empty()) return;
    SnapshotManager::get().deleteSnapshot(m_levelID, m_snapshots[m_currentIndex]);
    m_snapshots = SnapshotManager::get().getSnapshots(m_levelID);
    if (m_currentIndex >= static_cast<int>(m_snapshots.size()) && m_currentIndex > 0)
        m_currentIndex--;
    refreshLabels();
    Notification::create("Snapshot deleted.", NotificationIcon::Warning)->show();
}

void TimelinePopup::onDeleteAll(CCObject*) {
    SnapshotManager::get().deleteAllSnapshots(m_levelID);
    m_snapshots.clear();
    m_currentIndex = 0;
    onClose(nullptr);
    Notification::create("All snapshots deleted.", NotificationIcon::Warning)->show();
}

void TimelinePopup::onClose(CCObject*) {
    this->setKeyboardEnabled(false);
    this->removeFromParentAndCleanup(true);
}
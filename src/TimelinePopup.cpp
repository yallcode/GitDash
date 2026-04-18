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

void TimelinePopup::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(
        this, -500, true
    );
}

bool TimelinePopup::init(LevelEditorLayer* editorLayer) {
    // Full-screen dim layer
    if (!CCLayerColor::initWithColor({ 0, 0, 0, 150 }))
        return false;

    m_editorLayer  = editorLayer;
    m_levelID      = editorLayer->m_level->m_levelID.value();
    m_snapshots    = SnapshotManager::get().getSnapshots(m_levelID);
    m_currentIndex = 0;

    this->setTouchEnabled(true);
    this->setKeyboardEnabled(true);

    buildUI();
    return true;
}

void TimelinePopup::buildUI() {
    auto winSize = CCDirector::get()->getWinSize();
    float cx = winSize.width  / 2.f;
    float cy = winSize.height / 2.f;

    // ── Popup background (GD style) ───────────────────────────────────────
    auto bg = CCScale9Sprite::create("GJ_square01.png");
    bg->setContentSize({ 360.f, 240.f });
    bg->setPosition({ cx, cy });
    this->addChild(bg, 1);

    // ── Title ─────────────────────────────────────────────────────────────
    auto title = CCLabelBMFont::create("GitDash Timeline", "goldFont.fnt");
    title->setScale(0.7f);
    title->setPosition({ cx, cy + 95.f });
    this->addChild(title, 2);

    // ── Separator line under title ────────────────────────────────────────
    auto line = CCSprite::createWithSpriteFrameName("floorLine_001.png");
    line->setScaleX(1.8f);
    line->setOpacity(100);
    line->setPosition({ cx, cy + 78.f });
    this->addChild(line, 2);

    // ── Empty state ───────────────────────────────────────────────────────
    if (m_snapshots.empty()) {
        auto emptyIcon = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
        if (emptyIcon) {
            emptyIcon->setScale(0.8f);
            emptyIcon->setOpacity(120);
            emptyIcon->setPosition({ cx, cy + 25.f });
            this->addChild(emptyIcon, 2);
        }

        auto lbl = CCLabelBMFont::create("No snapshots yet!", "bigFont.fnt");
        lbl->setScale(0.45f);
        lbl->setColor({ 200, 200, 200 });
        lbl->setPosition({ cx, cy - 15.f });
        this->addChild(lbl, 2);

        auto hint = CCLabelBMFont::create("Save your level to create one.", "chatFont.fnt");
        hint->setScale(0.4f);
        hint->setColor({ 150, 150, 150 });
        hint->setPosition({ cx, cy - 35.f });
        this->addChild(hint, 2);

        auto closeBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_06.png", 0.8f),
            this, menu_selector(TimelinePopup::onClose)
        );
        auto menu = CCMenu::create(closeBtn, nullptr);
        menu->setPosition({ cx, cy - 85.f });
        this->addChild(menu, 2);
        return;
    }

    // ── Snapshot card background ──────────────────────────────────────────
    auto card = CCScale9Sprite::create("square02_001.png");
    card->setContentSize({ 320.f, 100.f });
    card->setPosition({ cx, cy + 20.f });
    card->setOpacity(80);
    this->addChild(card, 2);

    // ── Navigation: prev arrow ────────────────────────────────────────────
    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setScale(0.8f);
    auto prevBtn = CCMenuItemSpriteExtra::create(
        prevSpr, this, menu_selector(TimelinePopup::onPrev)
    );

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setScale(0.8f);
    nextSpr->setFlipX(true);
    auto nextBtn = CCMenuItemSpriteExtra::create(
        nextSpr, this, menu_selector(TimelinePopup::onNext)
    );

    auto navMenu = CCMenu::create(prevBtn, nextBtn, nullptr);
    navMenu->setPosition({ cx, cy + 55.f });
    navMenu->setLayout(RowLayout::create()->setGap(240.f));
    this->addChild(navMenu, 3);

    // ── Index label (centered between arrows) ─────────────────────────────
    m_indexLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_indexLabel->setScale(0.45f);
    m_indexLabel->setColor({ 255, 220, 100 });
    m_indexLabel->setPosition({ cx, cy + 55.f });
    this->addChild(m_indexLabel, 3);

    // ── Clock icon ────────────────────────────────────────────────────────
    auto clockIcon = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    if (clockIcon) {
        clockIcon->setScale(0.5f);
        clockIcon->setPosition({ cx - 120.f, cy + 25.f });
        this->addChild(clockIcon, 3);
    }

    // ── Time label ────────────────────────────────────────────────────────
    m_timeLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_timeLabel->setScale(0.5f);
    m_timeLabel->setAnchorPoint({ 0.f, 0.5f });
    m_timeLabel->setPosition({ cx - 95.f, cy + 25.f });
    this->addChild(m_timeLabel, 3);

    // ── Size label ────────────────────────────────────────────────────────
    m_sizeLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_sizeLabel->setScale(0.42f);
    m_sizeLabel->setColor({ 160, 200, 255 });
    m_sizeLabel->setPosition({ cx, cy - 5.f });
    this->addChild(m_sizeLabel, 3);

    // ── Restore button (big green, prominent) ─────────────────────────────
    auto restoreBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Restore", "goldFont.fnt", "GJ_button_01.png", 0.9f),
        this, menu_selector(TimelinePopup::onRestore)
    );

    // ── Delete / Del All / Close ──────────────────────────────────────────
    auto deleteBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Delete", "bigFont.fnt", "GJ_button_06.png", 0.7f),
        this, menu_selector(TimelinePopup::onDelete)
    );
    auto deleteAllBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Del All", "bigFont.fnt", "GJ_button_06.png", 0.65f),
        this, menu_selector(TimelinePopup::onDeleteAll)
    );
    auto closeBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_05.png", 0.7f),
        this, menu_selector(TimelinePopup::onClose)
    );

    // Restore on its own row (prominent)
    auto restoreMenu = CCMenu::create(restoreBtn, nullptr);
    restoreMenu->setPosition({ cx, cy - 60.f });
    this->addChild(restoreMenu, 3);

    // Delete row
    auto actionMenu = CCMenu::create(deleteBtn, deleteAllBtn, closeBtn, nullptr);
    actionMenu->setPosition({ cx, cy - 95.f });
    actionMenu->setLayout(RowLayout::create()->setGap(8.f));
    this->addChild(actionMenu, 3);

    refreshLabels();
}

void TimelinePopup::refreshLabels() {
    if (m_snapshots.empty()) return;
    const auto& snap = m_snapshots[m_currentIndex];
    int total = static_cast<int>(m_snapshots.size());

    if (m_indexLabel)
        m_indexLabel->setString(fmt::format("Snap {}/{}", m_currentIndex + 1, total).c_str());

    if (m_timeLabel) {
        bool showTs = Mod::get()->getSettingValue<bool>("show-timestamps");
        std::string t = showTs
            ? fmt::format("{} ({})", snap.relativeTimeString(), snap.timeString())
            : snap.relativeTimeString();
        m_timeLabel->setString(t.c_str());
    }

    if (m_sizeLabel) {
        float kb = snap.dataSize / 1024.f;
        m_sizeLabel->setString(fmt::format("{:.1f} KB  |  label: {}", kb,
            snap.label.empty() ? "none" : snap.label).c_str());
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
        FLAlertLayer::create(nullptr, "GitDash Error",
            "Failed to load snapshot data.", "OK", nullptr)->show();
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
    this->setTouchEnabled(false);
    this->removeFromParentAndCleanup(true);
}
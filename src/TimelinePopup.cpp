#include "TimelinePopup.hpp"
#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

TimelinePopup* TimelinePopup::create(LevelEditorLayer* editorLayer) {
    auto ret = new TimelinePopup();
    // Popup size: 340 wide x 200 tall
    if (ret && ret->initAnchored(340.f, 200.f, editorLayer, "GJ_square01.png")) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool TimelinePopup::setup(LevelEditorLayer* editorLayer) {
    m_editorLayer  = editorLayer;
    m_levelID      = editorLayer->m_level->m_levelID.value();
    m_snapshots    = SnapshotManager::get().getSnapshots(m_levelID);
    m_currentIndex = 0;

    this->setTitle("GitDash Timeline");
    buildUI();
    return true;
}

void TimelinePopup::buildUI() {
    // m_mainLayer is the popup's content layer
    // Its coordinate origin is bottom-left of the popup
    auto size = m_mainLayer->getContentSize();
    float cx  = size.width  / 2.f;
    float cy  = size.height / 2.f;

    // ── Empty state ───────────────────────────────────────────────────────
    if (m_snapshots.empty()) {
        auto lbl = CCLabelBMFont::create(
            "No snapshots yet!\nSave your level first.",
            "bigFont.fnt"
        );
        lbl->setScale(0.35f);
        lbl->setAlignment(kCCTextAlignmentCenter);
        lbl->setPosition({ cx, cy + 10.f });
        m_mainLayer->addChild(lbl);

        auto closeBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Close", "bigFont.fnt", "GJ_button_01.png", 0.7f),
            this, menu_selector(TimelinePopup::onClose)
        );
        auto menu = CCMenu::create(closeBtn, nullptr);
        menu->setPosition({ cx, 25.f });
        m_mainLayer->addChild(menu);
        return;
    }

    // ── Nav row (arrows + index label) ────────────────────────────────────
    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setScale(0.65f);
    auto prevBtn = CCMenuItemSpriteExtra::create(
        prevSpr, this, menu_selector(TimelinePopup::onPrev)
    );

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSpr->setScale(0.65f);
    nextSpr->setFlipX(true);
    auto nextBtn = CCMenuItemSpriteExtra::create(
        nextSpr, this, menu_selector(TimelinePopup::onNext)
    );

    auto navMenu = CCMenu::create(prevBtn, nextBtn, nullptr);
    navMenu->setPosition({ cx, cy + 45.f });
    navMenu->setLayout(RowLayout::create()->setGap(100.f));
    m_mainLayer->addChild(navMenu);

    m_indexLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_indexLabel->setScale(0.4f);
    m_indexLabel->setPosition({ cx, cy + 45.f });
    m_mainLayer->addChild(m_indexLabel);

    // ── Info labels ───────────────────────────────────────────────────────
    m_timeLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_timeLabel->setScale(0.38f);
    m_timeLabel->setPosition({ cx, cy + 15.f });
    m_mainLayer->addChild(m_timeLabel);

    m_sizeLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_sizeLabel->setScale(0.38f);
    m_sizeLabel->setColor({ 180, 180, 180 });
    m_sizeLabel->setPosition({ cx, cy - 5.f });
    m_mainLayer->addChild(m_sizeLabel);

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
    btnMenu->setPosition({ cx, 28.f });
    btnMenu->setLayout(RowLayout::create()->setGap(5.f));
    m_mainLayer->addChild(btnMenu);

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

    // Reload the editor scene with restored data
    auto scene    = CCScene::create();
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
    this->onClose(nullptr);
    Notification::create("All snapshots deleted.", NotificationIcon::Warning)->show();
}
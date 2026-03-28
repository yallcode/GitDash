#include "TimelinePopup.hpp"
#include "SnapshotManager.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

// ─── Factory ──────────────────────────────────────────────────────────────────

TimelinePopup* TimelinePopup::create(LevelEditorLayer* editorLayer) {
    auto ret = new TimelinePopup();
    // Popup size: 340 wide x 220 tall
    if (ret && ret->initAnchored(340.f, 220.f, editorLayer)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

// ─── Setup ────────────────────────────────────────────────────────────────────

bool TimelinePopup::setup(LevelEditorLayer* editorLayer) {
    m_editorLayer = editorLayer;
    m_levelID     = editorLayer->m_level->m_levelID.value();
    m_snapshots   = SnapshotManager::get().getSnapshots(m_levelID);
    m_currentIndex = 0;

    this->setTitle("GitDash Timeline");

    buildContent();
    return true;
}

void TimelinePopup::buildContent() {
    auto winSize  = m_mainLayer->getContentSize();
    auto centerX  = winSize.width  / 2.f;
    auto centerY  = winSize.height / 2.f;

    // ── Empty state ───────────────────────────────────────────────────────
    if (m_snapshots.empty()) {
        m_emptyLabel = CCLabelBMFont::create(
            "No snapshots yet.\nSave your level to create one!",
            "bigFont.fnt"
        );
        m_emptyLabel->setScale(0.35f);
        m_emptyLabel->setAlignment(kCCTextAlignmentCenter);
        m_emptyLabel->setPosition({ centerX, centerY });
        m_mainLayer->addChild(m_emptyLabel);
        return;
    }

    // ── Navigation row: [←]  "Snap X/Y"  [→] ────────────────────────────
    float navY = winSize.height - 55.f;

    auto prevBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"),
        this, menu_selector(TimelinePopup::onPrev)
    );
    prevBtn->setTag(0);

    auto nextSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    nextSprite->setFlipX(true);
    auto nextBtn = CCMenuItemSpriteExtra::create(
        nextSprite, this, menu_selector(TimelinePopup::onNext)
    );
    nextBtn->setTag(1);

    m_indexLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_indexLabel->setScale(0.5f);

    auto navMenu = CCMenu::create(prevBtn, nextBtn, nullptr);
    navMenu->setPosition({ centerX, navY });
    navMenu->setLayout(
        RowLayout::create()
            ->setGap(80.f)
    );
    m_mainLayer->addChild(navMenu);

    m_indexLabel->setPosition({ centerX, navY });
    m_mainLayer->addChild(m_indexLabel);

    // ── Snapshot info card ────────────────────────────────────────────────
    float cardY = centerY + 5.f;

    // Time ago label
    m_timeLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_timeLabel->setScale(0.45f);
    m_timeLabel->setPosition({ centerX, cardY + 28.f });
    m_mainLayer->addChild(m_timeLabel);

    // User label (editable)
    m_labelText = CCLabelBMFont::create("", "chatFont.fnt");
    m_labelText->setScale(0.5f);
    m_labelText->setColor({ 180, 220, 255 });
    m_labelText->setPosition({ centerX, cardY + 8.f });
    m_mainLayer->addChild(m_labelText);

    // Size label
    m_sizeLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_sizeLabel->setScale(0.4f);
    m_sizeLabel->setColor({ 150, 150, 150 });
    m_sizeLabel->setPosition({ centerX, cardY - 12.f });
    m_mainLayer->addChild(m_sizeLabel);

    // ── Action buttons row ────────────────────────────────────────────────
    float btnY = 38.f;

    auto restoreBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Restore", "goldFont.fnt", "GJ_button_01.png", 0.7f),
        this, menu_selector(TimelinePopup::onRestore)
    );

    auto deleteBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Delete", "bigFont.fnt", "GJ_button_06.png", 0.6f),
        this, menu_selector(TimelinePopup::onDelete)
    );

    auto labelBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Label", "bigFont.fnt", "GJ_button_04.png", 0.6f),
        this, menu_selector(TimelinePopup::onEditLabel)
    );

    auto deleteAllBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Delete All", "bigFont.fnt", "GJ_button_06.png", 0.5f),
        this, menu_selector(TimelinePopup::onDeleteAll)
    );

    auto actionMenu = CCMenu::create(restoreBtn, deleteBtn, labelBtn, deleteAllBtn, nullptr);
    actionMenu->setPosition({ centerX, btnY });
    actionMenu->setLayout(
        RowLayout::create()
            ->setGap(8.f)
    );
    m_mainLayer->addChild(actionMenu);

    // Initial populate
    refreshLabels();
}

void TimelinePopup::refreshLabels() {
    if (m_snapshots.empty() || m_currentIndex >= static_cast<int>(m_snapshots.size()))
        return;

    const auto& snap = m_snapshots[m_currentIndex];
    int total        = static_cast<int>(m_snapshots.size());

    // Index indicator
    m_indexLabel->setString(
        fmt::format("Snap {}/{}", m_currentIndex + 1, total).c_str()
    );

    // Time
    bool showTimestamps = Mod::get()->getSettingValue<bool>("show-timestamps");
    std::string timeStr = showTimestamps
        ? fmt::format("{} ({})", snap.relativeTimeString(), snap.timeString())
        : snap.relativeTimeString();
    m_timeLabel->setString(timeStr.c_str());

    // Label
    std::string labelDisplay = snap.label.empty() ? "(no label)" : snap.label;
    m_labelText->setString(labelDisplay.c_str());

    // Size
    float kb = snap.dataSize / 1024.f;
    m_sizeLabel->setString(fmt::format("{:.1f} KB compressed", kb).c_str());
}

// ─── Navigation ───────────────────────────────────────────────────────────────

void TimelinePopup::onPrev(CCObject*) {
    // "Prev" = move to a more-recent snapshot (lower index)
    if (m_currentIndex > 0) {
        m_currentIndex--;
        refreshLabels();
    }
}

void TimelinePopup::onNext(CCObject*) {
    // "Next" = move to an older snapshot (higher index)
    if (m_currentIndex < static_cast<int>(m_snapshots.size()) - 1) {
        m_currentIndex++;
        refreshLabels();
    }
}

// ─── Actions ──────────────────────────────────────────────────────────────────

void TimelinePopup::onRestore(CCObject*) {
    if (m_snapshots.empty()) return;
    auto& snap = m_snapshots[m_currentIndex];

    // Confirm dialog before overwriting
    auto alert = FLAlertLayer::create(
        nullptr,
        "Restore Snapshot",
        fmt::format(
            "Restore to <cy>{}</c>?\n"
            "Your <cr>current unsaved changes will be lost</c>.",
            snap.relativeTimeString()
        ).c_str(),
        "Cancel", "Restore"
    );
    alert->m_button2->addClickEventListener([this, snap](CCObject*) {
        applySnapshot(snap);
    });
    alert->show();
}

void TimelinePopup::applySnapshot(const Snapshot& snap) {
    std::string levelString = SnapshotManager::get().loadSnapshot(m_levelID, snap);
    if (levelString.empty()) {
        FLAlertLayer::create(nullptr,
            "Error", "Failed to load snapshot data.", "OK", nullptr)->show();
        return;
    }

    // Write the snapshot data back as the level's active string
    m_editorLayer->m_level->m_levelString = levelString;

    // Force the editor to re-parse and reload the level objects
    // This re-initialises the object layer from the new levelString
    m_editorLayer->m_editorUI->clearObjects();
    m_editorLayer->loadLevelData();

    log::info("[GitDash] Restored snapshot: {} for level {}",
        snap.filename, m_levelID);

    this->onClose(nullptr);

    // Show a brief notification
    Notification::create(
        fmt::format("Restored: {}", snap.relativeTimeString()),
        NotificationIcon::Success
    )->show();
}

void TimelinePopup::onDelete(CCObject*) {
    if (m_snapshots.empty()) return;
    auto snap = m_snapshots[m_currentIndex];

    auto alert = FLAlertLayer::create(
        nullptr,
        "Delete Snapshot",
        fmt::format("Permanently delete snapshot from {}?", snap.relativeTimeString()).c_str(),
        "Cancel", "Delete"
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
    auto alert = FLAlertLayer::create(
        nullptr,
        "Delete ALL Snapshots",
        "This will <cr>permanently delete ALL snapshots</c> for this level. Are you sure?",
        "Cancel", "Delete All"
    );
    alert->m_button2->addClickEventListener([this](CCObject*) {
        SnapshotManager::get().deleteAllSnapshots(m_levelID);
        m_snapshots.clear();
        m_currentIndex = 0;
        // Rebuild UI to show empty state
        m_mainLayer->removeAllChildren();
        this->setTitle("GitDash Timeline");
        buildContent();
    });
    alert->show();
}

void TimelinePopup::onEditLabel(CCObject*) {
    if (m_snapshots.empty()) return;
    int idx = m_currentIndex;

    // Use Geode's built-in text input popup
    auto input = InputNode::create(200.f, "Enter label...", "chatFont.fnt", "", 32);

    auto alert = FLAlertLayer::create(
        nullptr,
        "Label This Snapshot",
        "",
        "Cancel", "Save"
    );

    // TODO: wire up the InputNode into the alert.
    // For v0.1 this is left as a stub — full implementation
    // would use a custom CCLayer with a TextInput node.
    log::info("[GitDash] Label editing: stub for v0.1");
    alert->show();
}
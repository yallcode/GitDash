#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

#include "SnapshotManager.hpp"
#include "TimelinePopup.hpp"

using namespace geode::prelude;

// ══════════════════════════════════════════════════════════════════════════════
// Hook 1 — EditorPauseLayer::onSaveAndPlay / onSaveAndExit / onSave
// These are the three save-related buttons in the pause menu.
// All three are bound in Geode 5 bindings.
// ══════════════════════════════════════════════════════════════════════════════

class $modify(GitDashPauseLayer, EditorPauseLayer) {

    // Runs when creator taps "Save and Play"
    void onSaveAndPlay(CCObject* sender) {
        EditorPauseLayer::onSaveAndPlay(sender);
        takeSnapshot();
    }

    // Runs when creator taps "Save and Exit"
    void onSaveAndExit(CCObject* sender) {
        EditorPauseLayer::onSaveAndExit(sender);
        takeSnapshot();
    }

    // Runs when creator taps "Save" (stay in editor)
    void onSave(CCObject* sender) {
        EditorPauseLayer::onSave(sender);
        takeSnapshot();
    }

    // ── Timeline button injection ─────────────────────────────────────────
    void customSetup() {
        EditorPauseLayer::customSetup();

        auto spr = ButtonSprite::create("Timeline", "goldFont.fnt", "GJ_button_04.png", 0.7f);
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(GitDashPauseLayer::onOpenTimeline)
        );

        auto menu = this->getChildByType<CCMenu>(0);
        if (menu) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            auto newMenu = CCMenu::create(btn, nullptr);
            auto winSize = CCDirector::get()->getWinSize();
            newMenu->setPosition({ winSize.width - 60.f, winSize.height - 30.f });
            this->addChild(newMenu, 10);
        }
    }

    void onOpenTimeline(CCObject*) {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;
        auto popup = TimelinePopup::create(editorLayer);
        if (popup) popup->show();
    }

    // ── Helper ────────────────────────────────────────────────────────────
    void takeSnapshot() {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;

        int  levelID     = editorLayer->m_level->m_levelID.value();
        auto levelString = editorLayer->m_level->m_levelString;

        log::info("[GitDash] Saving snapshot for level {}", levelID);

        bool ok = SnapshotManager::get().takeSnapshot(levelID, levelString);
        if (ok) {
            size_t count = SnapshotManager::get().snapshotCount(levelID);
            Notification::create(
                fmt::format("GitDash: Snapshot #{} saved", count),
                NotificationIcon::Success,
                1.5f
            )->show();
        }
    }
};
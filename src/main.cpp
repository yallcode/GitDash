#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

#include "SnapshotManager.hpp"
#include "TimelinePopup.hpp"

using namespace geode::prelude;

// ══════════════════════════════════════════════════════════════════════════════
// Hook 1 — EditorUI::onSave
// This is the button callback that fires when the creator taps Save.
// It's reliably bound in Geode 5 bindings.
// ══════════════════════════════════════════════════════════════════════════════

class $modify(GitDashEditorUI, EditorUI) {
    void onSave(CCObject* sender) {
        // Let GD save first
        EditorUI::onSave(sender);

        // Then snapshot
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;

        auto levelString = editorLayer->m_level->m_levelString;
        int  levelID     = editorLayer->m_level->m_levelID.value();

        log::info("[GitDash] onSave triggered for level {}", levelID);

        bool ok = SnapshotManager::get().takeSnapshot(levelID, levelString);
        if (ok) {
            size_t count = SnapshotManager::get().snapshotCount(levelID);
            Notification::create(
                fmt::format("GitDash: Snapshot #{} saved", count),
                NotificationIcon::Success,
                1.5f
            )->show();
        } else {
            log::error("[GitDash] Snapshot failed for level {}", levelID);
        }
    }
};

// ══════════════════════════════════════════════════════════════════════════════
// Hook 2 — EditorPauseLayer::customSetup
// Injects the Timeline button into the editor pause menu.
// ══════════════════════════════════════════════════════════════════════════════

class $modify(GitDashPauseLayer, EditorPauseLayer) {
    void customSetup() {
        EditorPauseLayer::customSetup();

        // Create Timeline button
        auto spr = ButtonSprite::create("Timeline", "goldFont.fnt", "GJ_button_04.png", 0.7f);
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(GitDashPauseLayer::onOpenTimeline)
        );

        // Find the existing button menu and add to it
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

        log::info("[GitDash] Timeline button injected into pause menu.");
    }

    void onOpenTimeline(CCObject*) {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) {
            log::error("[GitDash] LevelEditorLayer not found!");
            return;
        }
        auto popup = TimelinePopup::create(editorLayer);
        if (popup) popup->show();
    }
};
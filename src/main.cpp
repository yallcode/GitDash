#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

#include "SnapshotManager.hpp"
#include "TimelinePopup.hpp"

using namespace geode::prelude;

// ══════════════════════════════════════════════════════════════════════════════
//  Hook 1 — LevelEditorLayer
//  Intercepts save and autosave to create snapshots.
// ══════════════════════════════════════════════════════════════════════════════

class $modify(GitDashEditorLayer, LevelEditorLayer) {

    // ── Manual save (Save button in the editor) ───────────────────────────
    void saveLevel() {
        // 1. Let GD perform its own save first
        LevelEditorLayer::saveLevel();

        // 2. After the native save, grab the freshly serialised level string
        auto levelString = m_level->m_levelString;
        int  levelID     = m_level->m_levelID.value();

        log::info("[GitDash] saveLevel() triggered for level ID {}", levelID);
        log::info("[GitDash] Level string size: {} chars", levelString.size());

        // 3. Take snapshot
        bool ok = SnapshotManager::get().takeSnapshot(levelID, levelString);
        if (ok) {
            size_t count = SnapshotManager::get().snapshotCount(levelID);
            log::info("[GitDash] Snapshot #{} created successfully.", count);

            // Show a subtle notification so the creator knows it worked
            Notification::create(
                fmt::format("GitDash: Snapshot #{} saved", count),
                NotificationIcon::Success,
                1.5f   // auto-dismiss after 1.5 seconds
            )->show();
        } else {
            log::error("[GitDash] Snapshot FAILED for level {}", levelID);
            Notification::create(
                "GitDash: Snapshot failed!",
                NotificationIcon::Error,
                3.0f
            )->show();
        }
    }

    // ── Autosave ──────────────────────────────────────────────────────────
    void autoSave() {
        LevelEditorLayer::autoSave();

        bool snapshotOnAutosave = Mod::get()->getSettingValue<bool>("snapshot-on-autosave");
        if (!snapshotOnAutosave) {
            log::debug("[GitDash] autoSave() fired — snapshot-on-autosave is OFF, skipping.");
            return;
        }

        auto levelString = m_level->m_levelString;
        int  levelID     = m_level->m_levelID.value();

        log::info("[GitDash] autoSave() — creating snapshot for level {}", levelID);
        SnapshotManager::get().takeSnapshot(levelID, levelString);
    }
};

// ══════════════════════════════════════════════════════════════════════════════
//  Hook 2 — EditorPauseLayer
//  Injects the "Timeline" button into the editor pause menu.
// ══════════════════════════════════════════════════════════════════════════════

class $modify(GitDashPauseLayer, EditorPauseLayer) {

    void customSetup() {
        // Run original setup first so all stock buttons exist
        EditorPauseLayer::customSetup();

        log::info("[GitDash] EditorPauseLayer::customSetup() — injecting Timeline button");

        // ── Create the Timeline button ─────────────────────────────────────
        //
        // We use a standard GD button sprite. If you add a custom sprite to
        // resources/, swap "GJ_timeIcon_001.png" for your asset name.
        //
        auto timelineSprite = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
        if (!timelineSprite) {
            // Fallback to a generic sprite if the frame isn't found
            timelineSprite = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        }
        timelineSprite->setScale(0.9f);

        auto timelineBtn = CCMenuItemSpriteExtra::create(
            timelineSprite,
            this,
            menu_selector(GitDashPauseLayer::onOpenTimeline)
        );

        // ── Find a good place to insert it ────────────────────────────────
        //
        // EditorPauseLayer has a button menu in the top-right area.
        // We look for the node tagged with a known GD tag, or fall back
        // to positioning it manually.
        //
        auto menu = this->getChildByType<CCMenu>(0);
        if (menu) {
            menu->addChild(timelineBtn);
            menu->updateLayout();
        } else {
            // Manual fallback position (top-right corner of pause overlay)
            auto btnMenu = CCMenu::create(timelineBtn, nullptr);
            auto winSize = CCDirector::get()->getWinSize();
            btnMenu->setPosition({ winSize.width - 30.f, winSize.height - 30.f });
            this->addChild(btnMenu, 10);
        }

        log::info("[GitDash] Timeline button injected.");
    }

    void onOpenTimeline(CCObject*) {
        log::info("[GitDash] Opening Timeline popup");

        // Get the LevelEditorLayer from the parent scene
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) {
            log::error("[GitDash] Could not find LevelEditorLayer!");
            return;
        }

        // Open the timeline popup
        auto popup = TimelinePopup::create(editorLayer);
        if (popup) {
            popup->show();
        } else {
            log::error("[GitDash] Failed to create TimelinePopup!");
        }
    }
};

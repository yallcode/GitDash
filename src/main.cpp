#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>

#include "SnapshotManager.hpp"
#include "TimelinePopup.hpp"

using namespace geode::prelude;

class $modify(GitDashPauseLayer, EditorPauseLayer) {

    void onSaveAndPlay(CCObject* sender) {
        EditorPauseLayer::onSaveAndPlay(sender);
        takeSnapshot();
    }

    void onSaveAndExit(CCObject* sender) {
        EditorPauseLayer::onSaveAndExit(sender);
        takeSnapshot();
    }

    void onSave(CCObject* sender) {
        EditorPauseLayer::onSave(sender);
        takeSnapshot();
    }

    void customSetup() {
        EditorPauseLayer::customSetup();

        auto winSize = CCDirector::get()->getWinSize();

        // Create a small clock-style button for the top-right corner
        // so it doesn't overlap any existing buttons
        auto spr = ButtonSprite::create(
            "Timeline", "bigFont.fnt", "GJ_button_04.png", 0.5f
        );

        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(GitDashPauseLayer::onOpenTimeline)
        );

        // Place it in its own menu in the top-right corner
        auto menu = CCMenu::create(btn, nullptr);
        menu->setPosition({ winSize.width - 55.f, winSize.height - 25.f });
        menu->setZOrder(10);
        this->addChild(menu, 10);
    }

    void onOpenTimeline(CCObject*) {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;
        auto popup = TimelinePopup::create(editorLayer);
        if (popup) popup->show();
    }

    void takeSnapshot() {
        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return;

        int  levelID     = editorLayer->m_level->m_levelID.value();
        auto levelString = editorLayer->m_level->m_levelString;

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
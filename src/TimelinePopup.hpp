#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "SnapshotManager.hpp"

using namespace geode::prelude;

/**
 * TimelinePopup
 *
 * The main GitDash UI. Opened from the EditorPauseLayer.
 * Displays a list of snapshots for the current level and lets the
 * user restore, label, or delete them.
 *
 * Layout (approximate):
 * ┌─────────────────────────────────┐
 * │          GitDash Timeline        │
 * ├─────────────────────────────────┤
 * │  [← Newer]  Snap 3/5  [Older →] │
 * │                                  │
 * │  ┌──────────────────────────┐   │
 * │  │  🕐 2 min ago             │   │
 * │  │  Label: (tap to edit)     │   │
 * │  │  Size: 42 KB compressed   │   │
 * │  └──────────────────────────┘   │
 * │                                  │
 * │    [Restore This]  [Delete]      │
 * │    [Delete All]    [Close]       │
 * └─────────────────────────────────┘
 */
class TimelinePopup : public geode::Popup<LevelEditorLayer*> {
protected:
    LevelEditorLayer* m_editorLayer = nullptr;
    int m_levelID                   = 0;
    std::vector<Snapshot> m_snapshots;
    int m_currentIndex              = 0;   // Which snapshot is currently shown

    // UI elements we need to update when navigating
    CCLabelBMFont* m_indexLabel     = nullptr;
    CCLabelBMFont* m_timeLabel      = nullptr;
    CCLabelBMFont* m_labelText      = nullptr;
    CCLabelBMFont* m_sizeLabel      = nullptr;
    CCLabelBMFont* m_emptyLabel     = nullptr;

    bool setup(LevelEditorLayer* editorLayer) override;

    // Build the inner content area
    void buildContent();

    // Refresh just the data labels (called when navigating)
    void refreshLabels();

    // Button callbacks
    void onPrev(CCObject*);
    void onNext(CCObject*);
    void onRestore(CCObject*);
    void onDelete(CCObject*);
    void onDeleteAll(CCObject*);
    void onEditLabel(CCObject*);

    // Applies the snapshot at m_currentIndex to the level.
    // This is the core restore logic.
    void applySnapshot(const Snapshot& snap);

public:
    static TimelinePopup* create(LevelEditorLayer* editorLayer);
};
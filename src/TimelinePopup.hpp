#pragma once

#include <Geode/Geode.hpp>
#include "SnapshotManager.hpp"

using namespace geode::prelude;

/**
 * TimelinePopup — GitDash's main UI.
 * Uses FLAlertLayer as base for maximum Geode 5 compatibility.
 */
class TimelinePopup : public FLAlertLayer {
protected:
    LevelEditorLayer* m_editorLayer = nullptr;
    int               m_levelID     = 0;
    std::vector<Snapshot> m_snapshots;
    int               m_currentIndex = 0;

    CCLabelBMFont* m_indexLabel  = nullptr;
    CCLabelBMFont* m_timeLabel   = nullptr;
    CCLabelBMFont* m_sizeLabel   = nullptr;
    CCLabelBMFont* m_emptyLabel  = nullptr;

    bool init(LevelEditorLayer* editorLayer);
    void buildUI();
    void refreshLabels();

    void onPrev(CCObject*);
    void onNext(CCObject*);
    void onRestore(CCObject*);
    void onDelete(CCObject*);
    void onDeleteAll(CCObject*);
    void applySnapshot(const Snapshot& snap);

public:
    static TimelinePopup* create(LevelEditorLayer* editorLayer);
    void onClose(CCObject*) override;

    // Intercept back button
    void keyBackClicked() override { onClose(nullptr); }
};

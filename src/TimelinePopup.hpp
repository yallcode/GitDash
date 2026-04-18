#pragma once

#include <Geode/Geode.hpp>
#include "SnapshotManager.hpp"

using namespace geode::prelude;

class TimelinePopup : public CCLayerColor {
protected:
    LevelEditorLayer* m_editorLayer = nullptr;
    int               m_levelID     = 0;
    std::vector<Snapshot> m_snapshots;
    int               m_currentIndex = 0;

    CCLabelBMFont* m_indexLabel = nullptr;
    CCLabelBMFont* m_timeLabel  = nullptr;
    CCLabelBMFont* m_sizeLabel  = nullptr;
    CCLabelBMFont* m_labelText  = nullptr;

    bool init(LevelEditorLayer* editorLayer);
    void buildUI();
    void refreshLabels();

    void onPrev(CCObject*);
    void onNext(CCObject*);
    void onRestore(CCObject*);
    void onDelete(CCObject*);
    void onDeleteAll(CCObject*);
    void onClose(CCObject*);
    void applySnapshot(const Snapshot& snap);

public:
    static TimelinePopup* create(LevelEditorLayer* editorLayer);

    // Touch handling
    bool ccTouchBegan(CCTouch*, CCEvent*) override { return true; }
    void registerWithTouchDispatcher() override;
    void keyBackClicked() override { onClose(nullptr); }
};
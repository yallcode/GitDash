#pragma once

#include <Geode/Geode.hpp>
#include "SnapshotManager.hpp"

using namespace geode::prelude;

class TimelinePopup : public FLAlertLayer, public FLAlertLayerProtocol {
protected:
    LevelEditorLayer* m_editorLayer = nullptr;
    int               m_levelID     = 0;
    std::vector<Snapshot> m_snapshots;
    int               m_currentIndex = 0;

    CCLabelBMFont* m_indexLabel = nullptr;
    CCLabelBMFont* m_timeLabel  = nullptr;
    CCLabelBMFont* m_sizeLabel  = nullptr;

    bool init(LevelEditorLayer* editorLayer);
    void buildUI();
    void refreshLabels();

    void onPrev(CCObject*);
    void onNext(CCObject*);
    void onRestore(CCObject*);
    void onDelete(CCObject*);
    void onDeleteAll(CCObject*);
    void applySnapshot(const Snapshot& snap);

    // FLAlertLayerProtocol
    void FLAlert_Clicked(FLAlertLayer*, bool btn2) override {}

public:
    static TimelinePopup* create(LevelEditorLayer* editorLayer);
    void onClose(CCObject*) override;
    void keyBackClicked() override { onClose(nullptr); }
};
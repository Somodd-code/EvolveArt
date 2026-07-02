#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include "EvolutionEngine.hpp"
#include "LevelBuilder.hpp"
#include <filesystem>
#include <vector>

using namespace geode::prelude;

class EditorUI;

class EvolvePopup : public geode::Popup {
protected:
    bool init(float width, float height);
    bool setup();
    void onChooseImage(CCObject*);
    void onChooseFrames(CCObject*);
    void onStartPause(CCObject*);
    void onStop(CCObject*);
    bool loadCurrentFrame();
    void step(float dt);
    void refreshLabel();

    EvolutionEngine m_engine;
    std::unique_ptr<LevelBuilder> m_builder;
    std::vector<std::filesystem::path> m_imagePaths;
    int m_currentFrameIndex = 0;
    bool m_running = false;
    bool m_hasStarted = false;
    bool m_isVideo = false;

    CCLabelBMFont* m_statusLabel = nullptr;
    CCLabelBMFont* m_imageLabel = nullptr;
    CCMenuItemSpriteExtra* m_startBtn = nullptr;

public:
    static EvolvePopup* create();
};

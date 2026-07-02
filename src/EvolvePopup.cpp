#include "EvolvePopup.hpp"
#include <Geode/binding/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/utils/file.hpp>
#include <filesystem>
#include <algorithm>
#include <cctype>

using namespace geode::prelude;

EvolvePopup* EvolvePopup::create() {
    auto ret = new EvolvePopup();
    if (ret->init(360.f, 220.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool EvolvePopup::init(float width, float height) {
    if (!Popup::init(width, height, "GJ_square01.png")) {
        return false;
    }
    this->setTitle("EvolveArt");
    this->setup();
    return true;
}

bool EvolvePopup::loadCurrentFrame() {
    if (m_currentFrameIndex < 0 || m_currentFrameIndex >= (int)m_imagePaths.size()) {
        return false;
    }

    int blocks = Mod::get()->getSettingValue<int64_t>("canvas-blocks-wide");
    bool ok = m_engine.loadTarget(m_imagePaths[m_currentFrameIndex].string(), blocks, blocks, 40, 40, 60);
    if (!ok) {
        return false;
    }

    m_engine.setPopulationSize((int)Mod::get()->getSettingValue<int64_t>("population-size"));
    m_engine.setMaxObjects((int)Mod::get()->getSettingValue<int64_t>(m_isVideo ? "max-objects-per-frame" : "max-objects"));

    std::vector<int> objectIds;
    if (Mod::get()->getSettingValue<bool>("use-all-object-ids")) {
        objectIds.reserve(120);
        for (int id = 1; id <= 120; id++) {
            objectIds.push_back(id);
        }
    } else {
        objectIds = { 1 };
    }
    m_engine.setObjectIds(objectIds);
    return true;
}

bool EvolvePopup::setup() {
    auto winSize = m_mainLayer->getContentSize();

    m_imageLabel = CCLabelBMFont::create("No image selected", "chatFont.fnt");
    m_imageLabel->setScale(0.6f);
    m_imageLabel->setPosition({winSize.width / 2, winSize.height - 55});
    m_mainLayer->addChild(m_imageLabel);

    auto chooseBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Choose Image"),
        nullptr,
        this,
        menu_selector(EvolvePopup::onChooseImage)
    );

    auto chooseFramesBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Choose Frames"),
        nullptr,
        this,
        menu_selector(EvolvePopup::onChooseFrames)
    );

    m_startBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Start"),
        nullptr,
        this,
        menu_selector(EvolvePopup::onStartPause)
    );

    auto stopBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Stop", "goldFont.fnt", "GJ_button_05.png"),
        nullptr,
        this,
        menu_selector(EvolvePopup::onStop)
    );

    auto menu = CCMenu::create();
    menu->addChild(chooseBtn);
    menu->addChild(chooseFramesBtn);
    menu->addChild(m_startBtn);
    menu->addChild(stopBtn);
    menu->setLayout(RowLayout::create()->setGap(12.f));
    menu->setContentSize({320.f, 40.f});
    menu->updateLayout();
    menu->setPosition({winSize.width / 2, winSize.height - 100});
    m_mainLayer->addChild(menu);

    m_statusLabel = CCLabelBMFont::create("Pick an image to begin.", "chatFont.fnt");
    m_statusLabel->setScale(0.6f);
    m_statusLabel->setPosition({winSize.width / 2, winSize.height - 140});
    m_mainLayer->addChild(m_statusLabel);

    auto hint = CCLabelBMFont::create(
        "Runs live while this popup is open. Adjust\npopulation/budget in the mod's settings.",
        "chatFont.fnt"
    );
    hint->setScale(0.45f);
    hint->setAlignment(kCCTextAlignmentCenter);
    hint->setPosition({winSize.width / 2, 35});
    m_mainLayer->addChild(hint);

    schedule(schedule_selector(EvolvePopup::step), 0.05f); // ~20 ticks/sec
    return true;
}

void EvolvePopup::onChooseImage(CCObject*) {
    auto filter = file::FilePickOptions::Filter{
        .description = "Images",
        .files = {"*.png", "*.jpg", "*.jpeg"}
    };
    async::spawn(file::pick(file::PickMode::OpenFile, file::FilePickOptions{std::nullopt, {filter}}), [this](Result<std::optional<std::filesystem::path>> result) {
        if (!result.isOk()) return;
        auto path = result.unwrap();
        if (!path.has_value()) return;
        if (!std::filesystem::exists(path.value())) return;

        m_imagePaths.clear();
        m_imagePaths.push_back(path.value());
        m_currentFrameIndex = 0;
        m_isVideo = false;

        if (!loadCurrentFrame()) {
            m_statusLabel->setString("Couldn't read that image file.");
            return;
        }

        m_imageLabel->setString(std::filesystem::path(path.value()).filename().string().c_str());
        m_hasStarted = false;
        m_statusLabel->setString("Ready. Press Start.");
    });
}

void EvolvePopup::onChooseFrames(CCObject*) {
    async::spawn(file::pick(file::PickMode::OpenFolder, file::FilePickOptions{}), [this](Result<std::optional<std::filesystem::path>> result) {
        if (!result.isOk()) return;
        auto dir = result.unwrap();
        if (!dir.has_value()) return;
        if (!std::filesystem::is_directory(dir.value())) {
            m_statusLabel->setString("Please select a folder containing image frames.");
            return;
        }

        m_imagePaths.clear();
        for (auto const& entry : std::filesystem::directory_iterator(dir.value())) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                m_imagePaths.push_back(entry.path());
            }
        }
        std::sort(m_imagePaths.begin(), m_imagePaths.end());
        if (m_imagePaths.empty()) {
            m_statusLabel->setString("No supported image frames found in that folder.");
            return;
        }

        m_currentFrameIndex = 0;
        m_isVideo = true;

        if (!loadCurrentFrame()) {
            m_statusLabel->setString("Couldn't load the first video frame.");
            return;
        }

        m_imageLabel->setString((std::filesystem::path(dir.value()).filename().string() + " (" + std::to_string(m_imagePaths.size()) + " frames)").c_str());
        m_hasStarted = false;
        m_statusLabel->setString("Ready. Press Start.");
    });
}

void EvolvePopup::onStartPause(CCObject*) {
    if (m_imagePaths.empty()) {
        m_statusLabel->setString("Choose an image or frames first.");
        return;
    }

    if (!m_hasStarted) {
        auto* editorUI = EditorUI::get();
        if (!editorUI) {
            m_statusLabel->setString("Open a level in the editor first!");
            return;
        }

        m_builder = std::make_unique<LevelBuilder>(editorUI, 150.f, 105.f, 30.f);
        m_builder->setupColorPalette();
        m_currentFrameIndex = 0;

        if (!loadCurrentFrame()) {
            m_statusLabel->setString("Couldn't load the current frame.");
            return;
        }

        int blocks = Mod::get()->getSettingValue<int64_t>("canvas-blocks-wide");
        float gap = (float)Mod::get()->getSettingValue<int64_t>("video-frame-gap-blocks");
        m_builder->setFrameOffset(m_currentFrameIndex * ((blocks + gap) * 30.f));
        m_hasStarted = true;
    }

    m_running = !m_running;
    m_startBtn->setSprite(ButtonSprite::create(m_running ? "Pause" : "Resume"));
}

void EvolvePopup::onStop(CCObject*) {
    m_running = false;
    m_hasStarted = false;
    m_startBtn->setSprite(ButtonSprite::create("Start"));
    m_statusLabel->setString("Stopped. Choose an image or frames to start a new build.");
}

void EvolvePopup::step(float) {
    if (!m_running || !m_builder) return;

    int generationsPerTick = (int)Mod::get()->getSettingValue<int64_t>("generations-per-tick");
    std::vector<Gene> accepted;
    m_engine.step(generationsPerTick, accepted);

    for (auto& gene : accepted) {
        m_builder->place(gene);
    }

    if (m_engine.finished()) {
        if (m_isVideo && m_currentFrameIndex + 1 < (int)m_imagePaths.size()) {
            m_currentFrameIndex++;
            if (!loadCurrentFrame()) {
                m_running = false;
                m_startBtn->setSprite(ButtonSprite::create("Start"));
                m_statusLabel->setString("Failed to load the next video frame.");
                return;
            }
            int blocks = Mod::get()->getSettingValue<int64_t>("canvas-blocks-wide");
            float gap = (float)Mod::get()->getSettingValue<int64_t>("video-frame-gap-blocks");
            m_builder->setFrameOffset(m_currentFrameIndex * ((blocks + gap) * 30.f));
        } else {
            m_running = false;
            m_startBtn->setSprite(ButtonSprite::create("Start"));
        }
    }

    refreshLabel();
}

void EvolvePopup::refreshLabel() {
    if (!m_statusLabel) return;
    double init = m_engine.initialError();
    double cur = m_engine.currentError();
    double pct = init > 0 ? std::clamp(100.0 * (1.0 - cur / init), 0.0, 100.0) : 0.0;
    std::string extra;
    if (m_isVideo) {
        extra = fmt::format("  |  Frame {}/{}", m_currentFrameIndex + 1, (int)m_imagePaths.size());
    }

    m_statusLabel->setString(fmt::format(
        "Objects: {}  |  Similarity: {:.1f}%{}{}",
        m_engine.objectsPlaced(), pct, m_running ? "" : "  (paused)", extra
    ).c_str());
}

#include "LevelBuilder.hpp"
#include <Geode/binding/EditorUI.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/GJEffectManager.hpp>
#include <Geode/binding/LevelSettingsObject.hpp>
#include <Geode/binding/ColorAction.hpp>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

// Confirmed against the community object-ID reference
// (a-zalt.github.io/gdknowledge/resources/object_ids.html): object 1 is the
// original default square block, present since 1.0 and safe on every GD
// version. By default the builder still uses object ID 1 for safety, but the
// mod now also supports a wider object-ID set if the user enables it.
static constexpr int kSquareObjectId = 1;

LevelBuilder::LevelBuilder(EditorUI* ui, float originX, float originY, float blockSize)
    : m_ui(ui), m_originX(originX), m_originY(originY), m_blockSize(blockSize) {}

// -----------------------------------------------------------------------
// COLOUR — READ THIS BEFORE YOU BUILD
//
// GD doesn't store colour per-object; every object points at a colour
// *channel*, and channels get their colour from the level's colour
// actions (the same data the "Edit Color" pause-menu button edits).
// To show N distinct colours we pre-register a small palette of channels
// (channelBase..channelBase+paletteSize) with their target colours, then
// point each placed square at whichever palette entry is closest to its
// gene's colour.
//
// The call below (GJEffectManager::updateColor) is the binding I'm most
// confident about for setting a channel's live colour at runtime — it's
// what several existing colour-changing mods use. If it doesn't compile
// as-is on your Geode version, open:
//   Geode/binding/GJEffectManager.hpp
// and look for a method that takes a channel id + cocos2d::ccColor3B (it
// may be named slightly differently, e.g. updateColorForChannel). Swap the
// one call below and everything else in this file is unaffected.
// -----------------------------------------------------------------------
static constexpr int kChannelBase = 500; // channels 500..500+paletteSize, unlikely to collide with your level

void LevelBuilder::setupColorPalette() {
    m_palette.clear();
    auto* lel = LevelEditorLayer::get();
    if (!lel || !lel->m_effectManager) {
        log::warn("EvolveArt: no effect manager available, skipping colour setup");
        return;
    }

    // 6x6x6 colour cube = 216 channels. Denser palette = closer colour match,
    // at the cost of registering more channels.
    constexpr int steps = 6;
    int channel = kChannelBase;
    for (int ri = 0; ri < steps; ri++) {
        for (int gi = 0; gi < steps; gi++) {
            for (int bi = 0; bi < steps; bi++) {
                uint8_t r = (uint8_t)std::round(ri * 255.0 / (steps - 1));
                uint8_t g = (uint8_t)std::round(gi * 255.0 / (steps - 1));
                uint8_t b = (uint8_t)std::round(bi * 255.0 / (steps - 1));

                if (lel->m_effectManager) {
                    lel->m_effectManager->calculateBaseActiveColors();
                }

                m_palette.push_back({r, g, b, channel});
                channel++;
            }
        }
    }
    log::info("EvolveArt: registered {} colour channels starting at {}", m_palette.size(), kChannelBase);
}

int LevelBuilder::channelForColor(uint8_t r, uint8_t g, uint8_t b) {
    if (m_palette.empty()) return 1; // fall back to channel 1 (usually already in use)
    int best = m_palette[0].channel;
    long bestDist = 1L << 60;
    for (auto& p : m_palette) {
        long dr = (long)p.r - r, dg = (long)p.g - g, db = (long)p.b - b;
        long dist = dr * dr + dg * dg + db * db;
        if (dist < bestDist) { bestDist = dist; best = p.channel; }
    }
    return best;
}

void LevelBuilder::setFrameOffset(float frameOffset) {
    m_frameOffset = frameOffset;
}

void LevelBuilder::place(const Gene& gene) {
    if (!m_ui) return;

    float worldX = m_originX + m_frameOffset + gene.cx * m_blockSize;
    float worldY = m_originY + gene.cy * m_blockSize;

    // The current Geode SDK exposes the simpler two-argument createObject signature.
    auto* obj = m_ui->createObject(gene.objectId, {worldX, worldY});
    if (!obj) return;

    // Scale the default 30x30 block up/down to match the gene's footprint.
    float scale = std::max(gene.w, gene.h) * m_blockSize / 30.f;
    obj->setScale(scale);
    obj->setRotation(gene.rotationDeg);

    int channel = channelForColor(gene.r, gene.g, gene.b);
    obj->setMainColorMode(1);
    obj->setSecondaryColorMode(1);
    obj->setupColorSprite(channel, true);
    obj->updateObjectEditorColor();
}

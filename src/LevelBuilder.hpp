#pragma once
#include "EvolutionEngine.hpp"
#include <Geode/Geode.hpp>

class EditorUI;

// Turns Genes (abstract, engine-space) into real GameObjects placed in the
// currently open level editor. This is the one file that talks to
// GD-internal bindings directly, kept separate from EvolutionEngine on
// purpose so the algorithm itself never depends on GD at all.
class LevelBuilder {
public:
    // originX/originY = world position (GD units) of the canvas's bottom-left
    // corner, blockSize = GD units per canvas block (30 = one default square).
    LevelBuilder(EditorUI* ui, float originX, float originY, float blockSize);

    // Places one object for `gene`. Call once per accepted gene.
    void place(const Gene& gene);

    // Call once, before placing anything, to set up the colour-channel
    // palette used by place(). See the note in the .cpp file.
    void setupColorPalette();

    void setFrameOffset(float frameOffset);

private:
    int channelForColor(uint8_t r, uint8_t g, uint8_t b);

    EditorUI* m_ui;
    float m_originX, m_originY, m_blockSize;
    float m_frameOffset = 0.f;

    struct PaletteEntry { uint8_t r, g, b; int channel; };
    std::vector<PaletteEntry> m_palette;
};

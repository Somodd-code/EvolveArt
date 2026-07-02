#pragma once
#include <vector>
#include <string>
#include <random>
#include <cstdint>

// One placed "gene" = one object that will end up in the GD level.
// Coordinates are in CANVAS BLOCKS (integer grid cells), not GD units.
// LevelBuilder.cpp is responsible for turning this into a real object.
struct Gene {
    enum class Shape { Square, Triangle } shape = Shape::Square;
    int objectId = 1;
    float cx = 0.f, cy = 0.f;      // center, in blocks
    float w = 1.f, h = 1.f;        // size, in blocks
    float rotationDeg = 0.f;       // 0/90/180/270 kept simple on purpose
    uint8_t r = 255, g = 255, b = 255;
};

// A tiny RGB raster the engine uses to simulate "what the level looks like
// so far" without ever touching the actual game. This is what the fitness
// function compares against the target image.
struct Canvas {
    int w = 0, h = 0;
    std::vector<float> px; // r,g,b interleaved, 0..255 range, size w*h*3

    void init(int width, int height, uint8_t bgR, uint8_t bgG, uint8_t bgB);
    void blendRect(const Gene& gene, float alpha = 1.0f);
    double errorInRect(const Canvas& target, int x0, int y0, int x1, int y1) const;
};

class EvolutionEngine {
public:
    // Loads and downsamples `imagePath` to (blocksWide x blocksHigh).
    // Returns false if the file couldn't be decoded.
    bool loadTarget(const std::string& imagePath, int blocksWide, int blocksHigh,
                     uint8_t bgR, uint8_t bgG, uint8_t bgB);

    // Runs `n` generations. Any gene that is accepted (because it reduced
    // total error) is appended to acceptedThisCall and also to m_history.
    // Returns the number of generations that resulted in an accepted gene.
    int step(int n, std::vector<Gene>& acceptedThisCall);

    bool finished() const { return m_objectsPlaced >= m_maxObjects; }
    void setMaxObjects(int n) { m_maxObjects = n; }
    void setPopulationSize(int n) { m_populationSize = n; }
    void setObjectIds(const std::vector<int>& objectIds) { m_objectIds = objectIds; }

    double currentError() const { return m_currentError; }
    double initialError() const { return m_initialError; }
    int objectsPlaced() const { return m_objectsPlaced; }
    int canvasW() const { return m_canvas.w; }
    int canvasH() const { return m_canvas.h; }

private:
    Gene randomCandidate();
    Gene mutateFromErrorRegion();
    double totalError() const;

    Canvas m_target;
    Canvas m_canvas;
    std::vector<Gene> m_history;

    int m_populationSize = 40;
    int m_maxObjects = 6000;
    int m_objectsPlaced = 0;
    double m_currentError = 0.0;
    double m_initialError = 0.0;

    std::vector<int> m_objectIds = { 1 };
    std::mt19937 m_rng{std::random_device{}()};
};

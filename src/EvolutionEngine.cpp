#include "EvolutionEngine.hpp"
#include <Geode/Geode.hpp>
#include <cocos2d.h>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

// ---------------------------------------------------------------- Canvas --

void Canvas::init(int width, int height, uint8_t bgR, uint8_t bgG, uint8_t bgB) {
    w = width;
    h = height;
    px.assign((size_t)w * h * 3, 0.f);
    for (int i = 0; i < w * h; i++) {
        px[i * 3 + 0] = bgR;
        px[i * 3 + 1] = bgG;
        px[i * 3 + 2] = bgB;
    }
}

// Draws a (possibly rotated) rectangle onto the canvas with simple
// alpha blending. Rotation is snapped to 0/90/180/270 so the bounding
// box math stays trivial (matches how the level builder places objects).
void Canvas::blendRect(const Gene& gene, float alpha) {
    float halfW = gene.w * 0.5f, halfH = gene.h * 0.5f;
    // For 90/270 the visual footprint is effectively swapped for squares;
    // triangles keep their own bounding box either way, which is good
    // enough for the fitness approximation.
    if (std::fmod(std::abs(gene.rotationDeg), 180.f) == 90.f) {
        std::swap(halfW, halfH);
    }

    int x0 = (int)std::floor(gene.cx - halfW);
    int y0 = (int)std::floor(gene.cy - halfH);
    int x1 = (int)std::ceil(gene.cx + halfW);
    int y1 = (int)std::ceil(gene.cy + halfH);

    x0 = std::clamp(x0, 0, w);
    x1 = std::clamp(x1, 0, w);
    y0 = std::clamp(y0, 0, h);
    y1 = std::clamp(y1, 0, h);

    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            bool inside = true;
            if (gene.shape == Gene::Shape::Triangle) {
                // crude triangle test: point must be below the diagonal
                float fx = (x - x0) / std::max(1.f, (float)(x1 - x0));
                float fy = (y - y0) / std::max(1.f, (float)(y1 - y0));
                inside = (fx + fy) <= 1.05f;
            }
            if (!inside) continue;
            size_t idx = ((size_t)y * w + x) * 3;
            px[idx + 0] = px[idx + 0] * (1 - alpha) + gene.r * alpha;
            px[idx + 1] = px[idx + 1] * (1 - alpha) + gene.g * alpha;
            px[idx + 2] = px[idx + 2] * (1 - alpha) + gene.b * alpha;
        }
    }
}

double Canvas::errorInRect(const Canvas& target, int x0, int y0, int x1, int y1) const {
    x0 = std::clamp(x0, 0, w); x1 = std::clamp(x1, 0, w);
    y0 = std::clamp(y0, 0, h); y1 = std::clamp(y1, 0, h);
    double err = 0.0;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            size_t idx = ((size_t)y * w + x) * 3;
            for (int c = 0; c < 3; c++) {
                double d = px[idx + c] - target.px[idx + c];
                err += d * d;
            }
        }
    }
    return err;
}

// --------------------------------------------------------- EvolutionEngine --

bool EvolutionEngine::loadTarget(const std::string& imagePath, int blocksWide, int blocksHigh,
                                  uint8_t bgR, uint8_t bgG, uint8_t bgB) {
    auto image = new cocos2d::CCImage();
    if (!image->initWithImageFile(imagePath.c_str())) {
        image->release();
        log::error("EvolveArt: failed to decode image at {}", imagePath);
        return false;
    }

    int srcW = image->getWidth();
    int srcH = image->getHeight();
    unsigned char* data = image->getData(); // RGBA8888
    bool hasAlpha = image->hasAlpha();
    int channels = hasAlpha ? 4 : 3;

    m_target.init(blocksWide, blocksHigh, bgR, bgG, bgB);

    // Simple nearest-neighbour downsample from source pixels into the
    // block grid. Good enough since we're heavily downsampling anyway.
    for (int by = 0; by < blocksHigh; by++) {
        for (int bx = 0; bx < blocksWide; bx++) {
            int sx = std::min(srcW - 1, bx * srcW / blocksWide);
            int sy = std::min(srcH - 1, by * srcH / blocksHigh);
            size_t sidx = ((size_t)sy * srcW + sx) * channels;
            float r = data[sidx + 0];
            float g = data[sidx + 1];
            float b = data[sidx + 2];
            float a = hasAlpha ? (data[sidx + 3] / 255.f) : 1.f;

            // GD's canvas is flipped vertically relative to image row order.
            int ty = (blocksHigh - 1 - by);
            size_t tidx = ((size_t)ty * blocksWide + bx) * 3;
            m_target.px[tidx + 0] = r * a + bgR * (1 - a);
            m_target.px[tidx + 1] = g * a + bgG * (1 - a);
            m_target.px[tidx + 2] = b * a + bgB * (1 - a);
        }
    }

    image->release();

    m_canvas.init(blocksWide, blocksHigh, bgR, bgG, bgB);
    m_history.clear();
    m_objectsPlaced = 0;
    m_currentError = totalError();
    m_initialError = m_currentError;
    return true;
}

double EvolutionEngine::totalError() const {
    return m_canvas.errorInRect(m_target, 0, 0, m_canvas.w, m_canvas.h);
}

static int randomObjectId(const std::vector<int>& objectIds, std::mt19937& rng) {
    if (objectIds.empty()) {
        return 1;
    }
    std::uniform_int_distribution<size_t> uobj(0, objectIds.size() - 1);
    return objectIds[uobj(rng)];
}

Gene EvolutionEngine::randomCandidate() {
    std::uniform_real_distribution<float> ux(0, (float)m_canvas.w);
    std::uniform_real_distribution<float> uy(0, (float)m_canvas.h);
    std::uniform_real_distribution<float> usize(1.f, std::max(2.f, m_canvas.w * 0.15f));
    std::uniform_int_distribution<int> urot(0, 3);
    std::uniform_int_distribution<int> ushape(0, 1);

    Gene g;
    g.shape = ushape(m_rng) == 0 ? Gene::Shape::Square : Gene::Shape::Triangle;
    g.objectId = randomObjectId(m_objectIds, m_rng);
    g.cx = ux(m_rng);
    g.cy = uy(m_rng);
    g.w = g.h = usize(m_rng);
    g.rotationDeg = (float)(urot(m_rng) * 90);

    int x = std::clamp((int)g.cx, 0, m_canvas.w - 1);
    int y = std::clamp((int)g.cy, 0, m_canvas.h - 1);
    size_t idx = ((size_t)y * m_canvas.w + x) * 3;
    g.r = (uint8_t)m_target.px[idx + 0];
    g.g = (uint8_t)m_target.px[idx + 1];
    g.b = (uint8_t)m_target.px[idx + 2];
    return g;
}

// Instead of a fully blind guess, bias half the population toward whatever
// region currently has the worst error, and colour it with the average
// target colour of that region. This is the trick that makes the image
// converge fast early on, then get progressively finer-detailed as objects
// shrink over time (object size distribution shrinks with objectsPlaced).
Gene EvolutionEngine::mutateFromErrorRegion() {
    std::uniform_int_distribution<int> ux(0, m_canvas.w - 1);
    std::uniform_int_distribution<int> uy(0, m_canvas.h - 1);

    int bestX = ux(m_rng), bestY = uy(m_rng);
    double bestErr = -1.0;
    // sample a handful of random points, keep the worst one -> cheap proxy
    // for "find the region that needs the most help"
    for (int i = 0; i < 12; i++) {
        int x = ux(m_rng), y = uy(m_rng);
        size_t idx = ((size_t)y * m_canvas.w + x) * 3;
        double d = 0;
        for (int c = 0; c < 3; c++) {
            double diff = m_canvas.px[idx + c] - m_target.px[idx + c];
            d += diff * diff;
        }
        if (d > bestErr) { bestErr = d; bestX = x; bestY = y; }
    }

    // Size shrinks as more objects are placed -> coarse-to-fine build-up.
    float progress = m_maxObjects > 0 ? (float)m_objectsPlaced / m_maxObjects : 0.f;
    float maxSize = std::max(1.5f, m_canvas.w * (0.22f * (1.f - progress) + 0.02f));
    std::uniform_real_distribution<float> usize(1.f, maxSize);
    std::uniform_real_distribution<float> jitter(-maxSize * 0.5f, maxSize * 0.5f);
    std::uniform_int_distribution<int> urot(0, 3);
    std::uniform_int_distribution<int> ushape(0, 1);

    Gene g;
    g.shape = ushape(m_rng) == 0 ? Gene::Shape::Square : Gene::Shape::Triangle;
    g.objectId = randomObjectId(m_objectIds, m_rng);
    g.cx = std::clamp(bestX + jitter(m_rng), 0.f, (float)m_canvas.w);
    g.cy = std::clamp(bestY + jitter(m_rng), 0.f, (float)m_canvas.h);
    g.w = g.h = usize(m_rng);
    g.rotationDeg = (float)(urot(m_rng) * 90);

    int x = std::clamp((int)g.cx, 0, m_canvas.w - 1);
    int y = std::clamp((int)g.cy, 0, m_canvas.h - 1);
    size_t idx = ((size_t)y * m_canvas.w + x) * 3;
    g.r = (uint8_t)m_target.px[idx + 0];
    g.g = (uint8_t)m_target.px[idx + 1];
    g.b = (uint8_t)m_target.px[idx + 2];
    return g;
}

int EvolutionEngine::step(int n, std::vector<Gene>& acceptedThisCall) {
    int accepted = 0;
    for (int gen = 0; gen < n; gen++) {
        if (finished()) break;

        Gene best;
        double bestErrAfter = 1e300;
        double bestErrBefore = 0;
        int bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
        bool found = false;

        for (int i = 0; i < m_populationSize; i++) {
            Gene cand = (i % 3 == 0) ? randomCandidate() : mutateFromErrorRegion();

            float halfW = cand.w * 0.5f, halfH = cand.h * 0.5f;
            if (std::fmod(std::abs(cand.rotationDeg), 180.f) == 90.f) std::swap(halfW, halfH);
            int x0 = (int)std::floor(cand.cx - halfW), x1 = (int)std::ceil(cand.cx + halfW);
            int y0 = (int)std::floor(cand.cy - halfH), y1 = (int)std::ceil(cand.cy + halfH);

            double errBefore = m_canvas.errorInRect(m_target, x0, y0, x1, y1);

            Canvas scratch = m_canvas; // cheap: only small regions matter, canvas is small
            scratch.blendRect(cand, 1.0f);
            double errAfter = scratch.errorInRect(m_target, x0, y0, x1, y1);

            if (errAfter < bestErrAfter) {
                bestErrAfter = errAfter;
                bestErrBefore = errBefore;
                best = cand;
                bx0 = x0; by0 = y0; bx1 = x1; by1 = y1;
                found = true;
            }
        }

        if (found && bestErrAfter < bestErrBefore) {
            m_canvas.blendRect(best, 1.0f);
            m_history.push_back(best);
            acceptedThisCall.push_back(best);
            m_objectsPlaced++;
            m_currentError -= (bestErrBefore - bestErrAfter);
            accepted++;
        }
    }
    return accepted;
}

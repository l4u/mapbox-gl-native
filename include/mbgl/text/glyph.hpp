#ifndef MBGL_TEXT_GLYPH
#define MBGL_TEXT_GLYPH

#include <mbgl/util/rect.hpp>

#include <cstdint>
#include <vector>
#include <map>

namespace mbgl {

typedef std::pair<uint16_t, uint16_t> GlyphRange;

// Note: this only works for the BMP
GlyphRange getGlyphRange(char32_t glyph);

struct GlyphMetrics {
    operator bool() const {
        return !(width == 0 && height == 0 && advance == 0);
    }

    // Glyph metrics.
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t left = 0;
    int32_t top = 0;
    uint32_t advance = 0;

};

struct Glyph {
    inline explicit Glyph() : rect(0, 0, 0, 0), metrics() {}
    inline explicit Glyph(const Rect<uint16_t> &rect,
                          const GlyphMetrics &metrics)
        : rect(rect), metrics(metrics) {}

    operator bool() const {
        return metrics || rect;
    }

    const Rect<uint16_t> rect;
    const GlyphMetrics metrics;
};

typedef std::map<uint32_t, Glyph> GlyphPositions;

class PositionedGlyph {
public:
    inline explicit PositionedGlyph(uint32_t glyph, uint32_t x, uint32_t y)
        : glyph(glyph), x(x), y(y) {}

    uint32_t glyph = 0;
    int32_t x = 0;
    int32_t y = 0;
};

typedef std::vector<PositionedGlyph> Shaping;
}

#endif

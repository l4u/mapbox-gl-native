#ifndef MBGL_STYLE_STYLE_PROPERTIES
#define MBGL_STYLE_STYLE_PROPERTIES

#include <mbgl/util/variant.hpp>
#include <mbgl/style/types.hpp>
#include <mbgl/style/function_properties.hpp>

#include <array>
#include <string>
#include <type_traits>
#include <memory>

namespace mbgl {

struct FillProperties {
    FillProperties() {}
    bool antialias = true;
    float opacity = 1.0f;
    Color fill_color = {{ 0, 0, 0, 1 }};
    Color stroke_color = {{ 0, 0, 0, -1 }};
    std::array<float, 2> translate = {{ 0, 0 }};
    TranslateAnchorType translateAnchor = TranslateAnchorType::Default;
    std::string image;

    inline bool isVisible() const {
        return opacity > 0 && (fill_color[3] > 0 || stroke_color[3] > 0);
    }
};

struct LineProperties {
    inline LineProperties() {}
    float opacity = 1.0f;
    Color color = {{ 0, 0, 0, 1 }};
    std::array<float, 2> translate = {{ 0, 0 }};
    TranslateAnchorType translateAnchor = TranslateAnchorType::Default;
    float width = 1;
    float offset = 0;
    float blur = 0;
    std::array<float, 2> dash_array = {{ 1, -1 }};
    std::string image;

    inline bool isVisible() const {
        return opacity > 0 && color[3] > 0 && width > 0;
    }
};

struct IconProperties {
    inline IconProperties() {}
    float opacity = 1.0f;
    float rotate = 0.0f;
    RotateAnchorType rotate_anchor = RotateAnchorType::Default;

    inline bool isVisible() const {
        return opacity > 0;
    }
};

struct TextProperties {
    inline TextProperties() {}
    float opacity = 1.0f;
    float size = 12.0f;
    Color color = {{ 0, 0, 0, 1 }};
    Color halo_color = {{ 1, 1, 1, 0.75 }};
    float halo_width = 0.25f;
    float halo_blur = 1.0f;

    inline bool isVisible() const {
        return opacity > 0 && (color[3] > 0 || halo_color[3] > 0) && size > 0;
    }
};

struct CompositeProperties {
    inline CompositeProperties() {}
    float opacity = 1.0f;

    inline bool isVisible() const {
        return opacity > 0;
    }
};

struct RasterProperties {
    inline RasterProperties() {}
    float opacity = 1.0f;
    float spin = 0.0f;
    std::array<float, 2> brightness = {{ 0, 1 }};
    float saturation = 0.0f;
    float contrast = 0.0f;
    float fade = 0.0f;

    inline bool isVisible() const {
        return opacity > 0;
    }
};

struct BackgroundProperties {
    inline BackgroundProperties() {}
    Color color = {{ 0, 0, 0, 1 }};
    std::string image;
};

typedef util::variant<
    FillProperties,
    LineProperties,
    IconProperties,
    TextProperties,
    CompositeProperties,
    RasterProperties,
    BackgroundProperties,
    std::false_type
> StyleProperties;

template <typename T>
const T &defaultStyleProperties();

}

#endif

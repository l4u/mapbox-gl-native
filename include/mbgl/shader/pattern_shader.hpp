#ifndef MBGL_SHADER_SHADER_PATTERN
#define MBGL_SHADER_SHADER_PATTERN

#include <mbgl/shader/shader.hpp>
#include <mbgl/util/mat4.hpp>

namespace mbgl {

class PatternShader : public Shader {
public:
    PatternShader();

    void bind(char *offset);

    void setOpacity(float opacity);
    void setPatternMatrix(const mat4& matrix);
    void setPatternTopLeft(const std::array<float, 2>& pattern_tl);
    void setPatternBottomRight(const std::array<float, 2>& pattern_br);
    void setMix(float mix);

private:
    int32_t a_pos = -1;

    float opacity = 1;
    int32_t u_opacity = -1;

    mat4 pattern_matrix = {{}};
    int32_t u_pattern_matrix = -1;

    std::array<float, 2> pattern_tl = {{}};
    int32_t u_pattern_tl = -1;

    std::array<float, 2> pattern_br = {{}};
    int32_t u_pattern_br = -1;

    float mix = 0;
    int32_t u_mix = -1;
};

}

#endif

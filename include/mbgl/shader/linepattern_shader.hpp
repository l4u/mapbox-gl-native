#ifndef MBGL_SHADER_SHADER_LINEPATTERN
#define MBGL_SHADER_SHADER_LINEPATTERN

#include <mbgl/shader/shader.hpp>

namespace mbgl {
    
class LinepatternShader : public Shader {
public:
    LinepatternShader();
    
    void bind(char *offset);
    
    void setPatternSize(const std::array<float, 2>& new_pattern_size);
    void setPatternTopLeft(const std::array<float, 2>& new_pattern_tl);
    void setPatternBottomRight(const std::array<float, 2>& new_pattern_br);
//    void setFade();
    void setExtrudeMatrix(const std::array<float, 16>& new_exmatrix);
    void setColor(const std::array<float, 4>& new_color);
    void setLineWidth(const std::array<float, 2>& new_linewidth);
    void setRatio(float new_ratio);
    void setGamma(float new_gamma);

private:
    int32_t a_pos = -1;
    int32_t a_linesofar = -1;
    std::array<float, 16> exmatrix = {{}};
    int32_t u_exmatrix = -1;
    
    std::array<float, 16> posmatrix = {{}};
    int32_t u_posmatrix = -1;

    float ratio = 0;
    int32_t u_ratio = -1;
    
    std::array<float, 4> color = {{}};
    int32_t u_color = -1;

//    u_point
    std::array<float, 2> linewidth = {{}};
    int32_t u_linewidth = -1;

    float gamma = 0.0f;
    int32_t u_gamma = -1;

    std::array<float, 2> pattern_size = {{}};
    int32_t u_pattern_size = -1;

    std::array<float, 2> pattern_tl = {{}};
    int32_t u_pattern_tl = -1;
    
    std::array<float, 2> pattern_br = {{}};
    int32_t u_pattern_br = -1;

//    u_fade
    
};
    
}

#endif

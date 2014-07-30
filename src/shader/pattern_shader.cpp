#include <mbgl/shader/pattern_shader.hpp>
#include <mbgl/shader/shaders.hpp>
#include <mbgl/platform/gl.hpp>

#include <cstdio>

using namespace mbgl;

PatternShader::PatternShader()
    : Shader(
        shaders[PATTERN_SHADER].vertex,
        shaders[PATTERN_SHADER].fragment
    ) {
    if (!valid) {
        fprintf(stderr, "invalid pattern shader\n");
        return;
    }

    a_pos = glGetAttribLocation(program, "a_pos");

    u_matrix = glGetUniformLocation(program, "u_matrix");
    u_pattern_matrix = glGetUniformLocation(program, "u_patternmatrix");
    u_opacity = glGetUniformLocation(program, "u_opacity");
    u_pattern_tl = glGetUniformLocation(program, "u_pattern_tl");
    u_pattern_br = glGetUniformLocation(program, "u_pattern_br");
    u_mix = glGetUniformLocation(program, "u_mix");

    // fprintf(stderr, "PatternShader:\n");
    // fprintf(stderr, "    - u_matrix: %d\n", u_matrix);
    // fprintf(stderr, "    - u_opacity: %d\n", u_opacity);
    // fprintf(stderr, "    - u_pattern_tl: %d\n", u_pattern_tl);
    // fprintf(stderr, "    - u_pattern_br: %d\n", u_pattern_br);
    // fprintf(stderr, "    - u_mix: %d\n", u_mix);
}

void PatternShader::bind(char *offset) {
    glEnableVertexAttribArray(a_pos);
    glVertexAttribPointer(a_pos, 2, GL_SHORT, false, 0, offset);
}

void PatternShader::setPatternMatrix(const mat4& new_matrix) {
    if (pattern_matrix != new_matrix) {
        glUniformMatrix3fv(u_pattern_matrix, 1, GL_FALSE, new_matrix.data());
        pattern_matrix = new_matrix;
    }
}

void PatternShader::setOpacity(float new_opacity) {
    if (opacity != new_opacity) {
        glUniform1f(u_opacity, new_opacity);
        opacity = new_opacity;
    }
}

void PatternShader::setPatternTopLeft(const std::array<float, 2>& new_pattern_tl) {
    if (pattern_tl != new_pattern_tl) {
        glUniform2fv(u_pattern_tl, 1, new_pattern_tl.data());
        pattern_tl = new_pattern_tl;
    }
}

void PatternShader::setPatternBottomRight(const std::array<float, 2>& new_pattern_br) {
    if (pattern_br != new_pattern_br) {
        glUniform2fv(u_pattern_br, 1, new_pattern_br.data());
        pattern_br = new_pattern_br;
    }
}

void PatternShader::setMix(float new_mix) {
    if (mix != new_mix) {
        glUniform1f(u_mix, new_mix);
        mix = new_mix;
    }
}

#include <mbgl/shader/linepattern_shader.hpp>
#include <mbgl/shader/shaders.hpp>
#include <mbgl/platform/gl.hpp>

#include <cstdio>

using namespace mbgl;

LinepatternShader::LinepatternShader()
: Shader(
         shaders[LINEPATTERN_SHADER].vertex,
         shaders[LINEPATTERN_SHADER].fragment
         ) {
    if (!valid) {
        fprintf(stderr, "invalid line pattern shader\n");
        return;
    }
    

    a_pos = glGetAttribLocation(program, "a_pos");
//    a_extrude = glGetAttribLocation(program, "a_extrude");
    a_linesofar = glGetAttribLocation(program, "a_linesofar");
    u_posmatrix = glGetUniformLocation(program, "u_posmatrix");
    u_exmatrix = glGetUniformLocation(program, "u_exmatrix");
    u_ratio = glGetUniformLocation(program, "u_ratio");
    u_color = glGetUniformLocation(program, "u_color");
//    u_point = glGetUniformLocation(program, "u_point");
    u_linewidth = glGetUniformLocation(program, "u_linewidth");
    u_gamma = glGetUniformLocation(program, "u_gamma");
    u_pattern_size = glGetUniformLocation(program, "u_pattern_size");
    u_pattern_tl = glGetUniformLocation(program, "u_pattern_tl");
    u_pattern_br = glGetUniformLocation(program, "u_pattern_br");
//    u_fade = glGetUniformLocation(program, "u_fade");

// do i need to figure out how to pass varying ?

//
//    // fprintf(stderr, "LinepatternShader:\n");
//    // fprintf(stderr, "    - u_linewidth: %d\n", u_linewidth);
//    // fprintf(stderr, "    - u_point: %d\n", u_point);
//    // fprintf(stderr, "    - u_gamma: %d\n", u_gamma);
//    // fprintf(stderr, "    - u_pattern_size: %d\n", u_pattern_size);
//    // fprintf(stderr, "    - u_pattern_tl: %d\n", u_pattern_tl);
//    // fprintf(stderr, "    - u_pattern_br: %d\n", u_pattern_br);
//    // fprintf(stderr, "    - u_fade: %d\n", u_fade);
//    //    and then some
}

void LinepatternShader::bind(char *offset) {
    glEnableVertexAttribArray(a_pos);
    glVertexAttribPointer(a_pos, 2, GL_SHORT, false, 8, offset);
//    ^ should the 4th here be 0 or 8?
}

void LinepatternShader::setPatternSize(const std::array<float, 2>& new_pattern_size) {
    if (pattern_size != new_pattern_size) {
        glUniform2fv(u_pattern_size, 1, new_pattern_size.data());
        pattern_size = new_pattern_size;
    }
}

void LinepatternShader::setPatternTopLeft(const std::array<float, 2>& new_pattern_tl) {
    if (pattern_tl != new_pattern_tl) {
        glUniform2fv(u_pattern_tl, 1, new_pattern_tl.data());
        pattern_tl = new_pattern_tl;
    }
}

void LinepatternShader::setPatternBottomRight(const std::array<float, 2>& new_pattern_br) {
    if (pattern_br != new_pattern_br) {
        glUniform2fv(u_pattern_br, 1, new_pattern_br.data());
        pattern_br = new_pattern_br;
    }
}

//void LinepatternShader::setFade() {
////    TODO??
//}

void LinepatternShader::setExtrudeMatrix(const std::array<float, 16>& new_exmatrix) {
    if (exmatrix != new_exmatrix) {
        glUniformMatrix4fv(u_exmatrix, 1, GL_FALSE, new_exmatrix.data());
        exmatrix = new_exmatrix;
    }
}

void LinepatternShader::setColor(const std::array<float, 4>& new_color) {
    if (color != new_color) {
        glUniform4fv(u_color, 1, new_color.data());
        color = new_color;
    }
}

void LinepatternShader::setLineWidth(const std::array<float, 2>& new_linewidth) {
    if (linewidth != new_linewidth) {
        glUniform2fv(u_linewidth, 1, new_linewidth.data());
        linewidth = new_linewidth;
    }
}

void LinepatternShader::setRatio(float new_ratio) {
    if (ratio != new_ratio) {
        glUniform1f(u_ratio, new_ratio);
        ratio = new_ratio;
    }
}

void LinepatternShader::setGamma(float new_gamma) {
    if (gamma != new_gamma) {
        glUniform1f(u_gamma, new_gamma);
        gamma = new_gamma;
    }
};

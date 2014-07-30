#include <mbgl/renderer/painter.hpp>
#include <mbgl/renderer/fill_bucket.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/style/style_layer.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/sprite.hpp>
#include <mbgl/geometry/sprite_atlas.hpp>
#include <mbgl/util/std.hpp>

using namespace mbgl;



void Painter::renderFill(FillBucket& bucket, const FillProperties& properties, const Tile::ID& id, const mat4 &vtxMatrix) {
    Color fill_color = properties.fill_color;
    fill_color[0] *= properties.opacity;
    fill_color[1] *= properties.opacity;
    fill_color[2] *= properties.opacity;
    fill_color[3] *= properties.opacity;

    Color stroke_color = properties.stroke_color;
    if (stroke_color[3] < 0) {
        stroke_color = fill_color;
    } else {
        stroke_color[0] *= properties.opacity;
        stroke_color[1] *= properties.opacity;
        stroke_color[2] *= properties.opacity;
        stroke_color[3] *= properties.opacity;
    }

    bool outline = properties.antialias && properties.stroke_color != properties.fill_color;
    bool fringeline = properties.antialias && properties.stroke_color == properties.fill_color;
    if (fringeline) {
        outline = true;
        stroke_color = fill_color;
    }


    // Because we're drawing top-to-bottom, and we update the stencil mask
    // below, we have to draw the outline first (!)
    if (outline && pass == Translucent) {
        useProgram(outlineShader->program);
        outlineShader->setMatrix(vtxMatrix);
        lineWidth(2.0f); // This is always fixed and does not depend on the pixelRatio!

        outlineShader->setColor(stroke_color);

        // Draw the entire line
        outlineShader->setWorld({{
            static_cast<float>(map.getState().getFramebufferWidth()),
            static_cast<float>(map.getState().getFramebufferHeight())
        }});
        glDepthRange(strata, 1.0f);
        bucket.drawVertices(*outlineShader);
    } else if (fringeline) {
        // // We're only drawing to the first seven bits (== support a maximum of
        // // 127 overlapping polygons in one place before we get rendering errors).
        // glStencilMask(0x3F);
        // glClear(GL_STENCIL_BUFFER_BIT);

        // // Draw front facing triangles. Wherever the 0x80 bit is 1, we are
        // // increasing the lower 7 bits by one if the triangle is a front-facing
        // // triangle. This means that all visible polygons should be in CCW
        // // orientation, while all holes (see below) are in CW orientation.
        // glStencilFunc(GL_EQUAL, 0x80, 0x80);

        // // When we do a nonzero fill, we count the number of times a pixel is
        // // covered by a counterclockwise polygon, and subtract the number of
        // // times it is "uncovered" by a clockwise polygon.
        // glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP);
    }

    // Only draw the fill when it's either opaque and we're drawing opaque
    // fragments or when it's translucent and we're drawing translucent
    // fragments
    if ((fill_color[3] >= 1.0f) == (pass == Opaque)) {
        const std::shared_ptr<Sprite> &sprite = map.getStyle()->sprite;
        if (properties.image.size() && sprite) {
            SpriteAtlas &spriteAtlas = *map.getSpriteAtlas();
            Rect<uint16_t> imagePos = spriteAtlas.getImage(properties.image, *sprite);
            float zoomFraction = map.getState().getZoomFraction();

            useProgram(patternShader->program);
            patternShader->setMatrix(vtxMatrix);
            patternShader->setPatternTopLeft({{
                float(imagePos.x) / spriteAtlas.getWidth(),
                float(imagePos.y) / spriteAtlas.getHeight(),
            }});
            patternShader->setPatternBottomRight({{
                float(imagePos.x + imagePos.w) / spriteAtlas.getWidth(),
                float(imagePos.y + imagePos.h) / spriteAtlas.getHeight(),
            }});
            patternShader->setMix(zoomFraction);
            patternShader->setOpacity(properties.opacity);

            float factor = 8.0 / std::pow(2, map.getState().getIntegerZoom() - id.z);

            mat4 matrix;
            matrix::identity(matrix);
            matrix::scale(matrix, matrix,
                          1 / (imagePos.w * factor),
                          1 / (imagePos.h * factor),
                          1);
            patternShader->setMatrix(matrix);

            spriteAtlas.bind(true);

            // Draw the actual triangles into the color & stencil buffer.
            glDepthRange(strata + strata_epsilon, 1.0f);
            bucket.drawElements(*patternShader);
        } else {
            // Draw filling rectangle.
            useProgram(plainShader->program);
            plainShader->setMatrix(vtxMatrix);
            plainShader->setColor(fill_color);

            // Draw the actual triangles into the color & stencil buffer.
            glDepthRange(strata + strata_epsilon, 1.0f);
            bucket.drawElements(*plainShader);
        }
    }

    // Because we're drawing top-to-bottom, and we update the stencil mask
    // below, we have to draw the outline first (!)
    if (fringeline && pass == Translucent) {
        useProgram(outlineShader->program);
        outlineShader->setMatrix(vtxMatrix);
        lineWidth(2.0f); // This is always fixed and does not depend on the pixelRatio!

        outlineShader->setColor(fill_color);

        // Draw the entire line
        outlineShader->setWorld({{
            static_cast<float>(map.getState().getFramebufferWidth()),
            static_cast<float>(map.getState().getFramebufferHeight())
        }});

        glDepthRange(strata + strata_epsilon, 1.0f);
        bucket.drawVertices(*outlineShader);
    }
}

void Painter::renderFill(FillBucket& bucket, std::shared_ptr<StyleLayer> layer_desc, const Tile::ID& id) {
    // Abort early.
    if (!bucket.hasData()) return;

    const FillProperties &properties = layer_desc->getProperties<FillProperties>();

    if (layer_desc->rasterize && layer_desc->rasterize->isEnabled(id.z)) {
        if (pass == Translucent) {
            const RasterizedProperties rasterize = layer_desc->rasterize->get(id.z);
            // Buffer value around the 0..4096 extent that will be drawn into the 256x256 pixel
            // texture. We later scale the texture so that the actual bounds will align with this
            // tile's bounds. The reason we do this is so that the
            if (!bucket.prerendered) {
                bucket.prerendered = std::make_unique<PrerenderedTexture>(rasterize);
                bucket.prerendered->bindFramebuffer();

                preparePrerender(*bucket.prerendered);

                const FillProperties modifiedProperties = [&]{
                    FillProperties modifiedProperties = properties;
                    modifiedProperties.opacity = 1;
                    return modifiedProperties;
                }();

                // When drawing the fill, we want to draw a buffer around too, so we
                // essentially downscale everyting, and then upscale it later when rendering.
                const int buffer = rasterize.buffer * 4096.0f;
                const mat4 vtxMatrix = [&]{
                    mat4 vtxMatrix;
                    matrix::ortho(vtxMatrix, -buffer, 4096 + buffer, -4096 - buffer, buffer, 0, 1);
                    matrix::translate(vtxMatrix, vtxMatrix, 0, -4096, 0);
                    return vtxMatrix;
                }();

                setOpaque();
                renderFill(bucket, modifiedProperties, id, vtxMatrix);

                setTranslucent();
                renderFill(bucket, modifiedProperties, id, vtxMatrix);

                if (rasterize.blur > 0) {
                    bucket.prerendered->blur(*this, rasterize.blur);
                }

                // RESET STATE
                bucket.prerendered->unbindFramebuffer();
                finishPrerender(*bucket.prerendered);
            }

            renderPrerenderedTexture(*bucket.prerendered, properties);
        }
    } else {
        const mat4 &vtxMatrix = translatedMatrix(properties.translate, id, properties.translateAnchor);
        renderFill(bucket, properties, id, vtxMatrix);
    }
}

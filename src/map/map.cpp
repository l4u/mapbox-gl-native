#include <mbgl/map/map.hpp>
#include <mbgl/map/source.hpp>
#include <mbgl/map/view.hpp>
#include <mbgl/platform/platform.hpp>
#include <mbgl/map/sprite.hpp>
#include <mbgl/util/transition.hpp>
#include <mbgl/util/time.hpp>
#include <mbgl/util/math.hpp>
#include <mbgl/util/clip_ids.hpp>
#include <mbgl/util/string.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/util/uv.hpp>
#include <mbgl/util/std.hpp>
#include <mbgl/style/style.hpp>
#include <mbgl/text/glyph_store.hpp>
#include <mbgl/geometry/glyph_atlas.hpp>
#include <mbgl/style/style_layer_group.hpp>
#include <mbgl/style/style_bucket.hpp>
#include <mbgl/util/texturepool.hpp>
#include <mbgl/geometry/sprite_atlas.hpp>

#include <algorithm>
#include <memory>
#include <iostream>

using namespace mbgl;

Map::Map(View& view)
    : view(view),
      transform(view),
      style(std::make_shared<Style>()),
      glyphAtlas(std::make_shared<GlyphAtlas>(1024, 1024)),
      glyphStore(std::make_shared<GlyphStore>()),
      spriteAtlas(std::make_shared<SpriteAtlas>(512, 512)),
      texturepool(std::make_shared<Texturepool>()),
      painter(*this),
      loop(std::make_shared<uv::loop>()) {

    view.initialize(this);

    // Make sure that we're doing an initial drawing in all cases.
    is_clean.clear();
    is_rendered.clear();
    is_swapped.test_and_set();
}

Map::~Map() {
    // Clear the style first before the rest of the constructor deletes members of this object.
    // This is required because members of the style reference the Map object in their destructors.
    style.reset();

    if (async) {
        stop();
    }
}

void Map::start() {
    // When starting map rendering in another thread, we perform async/continuously
    // updated rendering. Only in these cases, we attach the async handlers.
    async = true;

    // Setup async notifications
    async_terminate = new uv_async_t();
    uv_async_init(**loop, async_terminate, terminate);
    async_terminate->data = **loop;

    async_render = new uv_async_t();
    uv_async_init(**loop, async_render, render);
    async_render->data = this;

    async_cleanup = new uv_async_t();
    uv_async_init(**loop, async_cleanup, cleanup);
    async_cleanup->data = this;

    uv_thread_create(&thread, [](void *arg) {
        Map *map = static_cast<Map *>(arg);
        map->run();
    }, this);
}

void Map::stop() {
    if (async_terminate != nullptr) {
        uv_async_send(async_terminate);
    }

    uv_thread_join(&thread);

    // Run the event loop once to make sure our async delete handlers are called.
    uv_run(**loop, UV_RUN_ONCE);

    async = false;
}

void Map::delete_async(uv_handle_t *handle) {
    delete (uv_async_t *)handle;
}

void Map::run() {
    setup();
    prepare();
    uv_run(**loop, UV_RUN_DEFAULT);

    // If the map rendering wasn't started asynchronously, we perform one render
    // *after* all events have been processed.
    if (!async) {
        prepare();
        render();
    }
}

void Map::rerender() {
    // We only send render events if we want to continuously update the map
    // (== async rendering).
    if (async && async_render != nullptr) {
        uv_async_send(async_render);
    }
}

void Map::update() {
    is_clean.clear();
    rerender();
}

bool Map::needsSwap() {
    return is_swapped.test_and_set() == false;
}

void Map::swapped() {
    is_rendered.clear();
    rerender();
}

void Map::cleanup() {
    if (async_cleanup != nullptr) {
        uv_async_send(async_cleanup);
    }
}

void Map::cleanup(uv_async_t *async) {
    Map *map = static_cast<Map *>(async->data);

    map->view.make_active();
    map->painter.cleanup();
}

void Map::render(uv_async_t *async) {
    Map *map = static_cast<Map *>(async->data);


    if (map->state.hasSize()) {
        if (map->is_rendered.test_and_set() == false) {
            map->prepare();
            if (map->is_clean.test_and_set() == false) {
                map->render();
                map->is_swapped.clear();
                map->view.swap();
            } else {
                // We set the rendered flag in the test above, so we have to reset it
                // now that we're not actually rendering because the map is clean.
                map->is_rendered.clear();
            }
        }
    }
}

void Map::terminate(uv_async_t *async) {
    // Closes all open handles on the loop. This means that the loop will automatically terminate.
    uv_loop_t *loop = static_cast<uv_loop_t *>(async->data);
    uv_walk(loop, [](uv_handle_t *handle, void */*arg*/) {
        if (!uv_is_closing(handle)) {
            uv_close(handle, NULL);
        }
    }, NULL);
}

#pragma mark - Setup

void Map::setup() {
    view.make_active();

    painter.setup();
}

void Map::setStyleJSON(std::string newStyleJSON) {
    styleJSON.swap(newStyleJSON);
    sprite.reset();
    style->loadJSON((const uint8_t *)styleJSON.c_str());
    glyphStore->setURL(style->glyph_url);
    update();
}

std::string Map::getStyleJSON() const {
    return styleJSON;
}

void Map::setAccessToken(std::string access_token) {
    accessToken.swap(access_token);
}

std::string Map::getAccessToken() const {
    return accessToken;
}

std::shared_ptr<Sprite> Map::getSprite() {
    const float pixelRatio = state.getPixelRatio();
    const std::string &sprite_url = style->getSpriteURL();
    if (!sprite || sprite->pixelRatio != pixelRatio) {
        sprite = Sprite::Create(sprite_url, pixelRatio);
    }

    return sprite;
}

#pragma mark - View

void Map::setCenter(const LatLng& center) {
    transform.setCenter(center);
    update();
}

LatLng Map::getCenter() const {
    return transform.getCenter();
}

void Map::setZoom(double zoom) {
    transform.setZoom(zoom);
    update();
}

double Map::getZoom() const {
    return transform.getZoom();
}

void Map::setBearing(double degrees) {
    transform.setBearing(degrees);
    update();
}

double Map::getBearing() const {
    return transform.getBearing();
}

#pragma mark - Transitions

void Map::panBy(const Point& delta, double duration) {
    transform.panBy(delta, duration * 1_second);
    update();
}

void Map::panTo(const LatLng& latLng, double duration) {
    transform.panTo(latLng, duration * 1_second);
    update();
}

void Map::zoomTo(double zoom, double duration) {
    transform.zoomTo(zoom, transform.getCenter(), duration * 1_second);
    update();
}

void Map::zoomTo(double zoom, const LatLng& around, double duration) {
    transform.zoomTo(zoom, around, duration * 1_second);
    update();
}

void Map::rotateTo(double bearing, double duration) {
    transform.rotateTo(bearing, transform.getCenter(), duration * 1_second);
    update();
}

void Map::rotateTo(double bearing, const LatLng& around, double duration) {
    transform.rotateTo(bearing, around, duration * 1_second);
    update();
}

void Map::easeTo(const LatLng& center, double zoom, double bearing, double duration) {
    transform.easeTo(center, zoom, bearing, duration * 1_second);
    update();
}

void Map::flyTo(const LatLng& center, double zoom, double bearing, double duration) {
    transform.flyTo(center, zoom, bearing, duration * 1_second);
    update();
}

void Map::cancelTransitions() {
    transform.cancelTransitions();
    update();
}

void Map::startPanning() {
    transform.startPanning();
    update();
}

void Map::stopPanning() {
    transform.stopPanning();
    update();
}

void Map::startScaling() {
    transform.startScaling();
    update();
}

void Map::stopScaling() {
    transform.stopScaling();
    update();
}

void Map::startRotating() {
    transform.startRotating();
    update();
}

void Map::stopRotating() {
    transform.stopRotating();
    update();
}

#pragma mark - Size

void Map::resize(uint16_t width, uint16_t height, float ratio) {
    resize(width, height, ratio, width * ratio, height * ratio);
}

void Map::resize(uint16_t width, uint16_t height, float ratio, uint16_t fb_width, uint16_t fb_height) {
    if (transform.resize(width, height, ratio, fb_width, fb_height)) {
        update();
    }
}

#pragma mark - Constraints

double Map::getMinZoom() const {
    return transform.getMinZoom();
}

double Map::getMaxZoom() const {
    return transform.getMaxZoom();
}

bool Map::canRotate() {
    return transform.canRotate();
}

#pragma mark - Projection

Point Map::project(const LatLng& latlng) const {
    return transform.locationPoint(latlng);
}

LatLng Map::unproject(const Point& point) const {
    return transform.pointLocation(point);
}

#pragma mark - Toggles

void Map::setDebug(bool value) {
    debug = value;
    painter.setDebug(debug);
    update();
}

void Map::toggleDebug() {
    setDebug(!debug);
}

bool Map::getDebug() const {
    return debug;
}

void Map::setAppliedClasses(const std::vector<std::string> &classes) {
    style->setAppliedClasses(classes);
    if (style->hasTransitions()) {
        update();
    }
}


void Map::toggleClass(const std::string &name) {
    style->toggleClass(name);
    if (style->hasTransitions()) {
        update();
    }
}

const std::vector<std::string> &Map::getAppliedClasses() const {
   return style->getAppliedClasses();
}

void Map::setDefaultTransitionDuration(uint64_t duration_milliseconds) {
    style->setDefaultTransitionDuration(duration_milliseconds);
}

void Map::updateSources() {
    // First, disable all existing sources.
    for (const std::shared_ptr<StyleSource> &source : activeSources) {
        source->enabled = false;
    }

    // Then, reenable all of those that we actually use when drawing this layer.
    updateSources(style->layers);

    // Then, construct or destroy the actual source object, depending on enabled state.
    for (const std::shared_ptr<StyleSource> &style_source : activeSources) {
        if (style_source->enabled) {
            if (!style_source->source) {
                style_source->source = std::make_shared<Source>(style_source->info, getAccessToken());
            }
        } else {
            style_source->source.reset();
        }
    }

    // Finally, remove all sources that are disabled.
    util::erase_if(activeSources, [](std::shared_ptr<StyleSource> source){
        return !source->enabled;
    });
}

const std::set<std::shared_ptr<StyleSource>> Map::getActiveSources() const {
    return activeSources;
}

void Map::updateSources(const std::shared_ptr<StyleLayerGroup> &group) {
    if (!group) {
        return;
    }
    for (const std::shared_ptr<StyleLayer> &layer : group->layers) {
        if (!layer) continue;
        if (layer->bucket) {
            if (layer->bucket->style_source) {
                (*activeSources.emplace(layer->bucket->style_source).first)->enabled = true;
            }
        } else if (layer->layers) {
            updateSources(layer->layers);
        }
    }
}

void Map::updateTiles() {
    for (const std::shared_ptr<StyleSource> &source : getActiveSources()) {
        source->source->update(*this);
    }
}

void Map::updateRenderState() {
    std::forward_list<Tile::ID> ids;

    for (const std::shared_ptr<StyleSource> &source : getActiveSources()) {
        ids.splice_after(ids.before_begin(), source->source->getIDs());
        source->source->updateMatrices(painter.projMatrix, state);
    }

    const std::map<Tile::ID, ClipID> clipIDs = computeClipIDs(ids);

    for (const std::shared_ptr<StyleSource> &source : getActiveSources()) {
        source->source->updateClipIDs(clipIDs);
    }
}

void Map::prepare() {
    view.make_active();

    // Update transform transitions.
    animationTime = util::now();
    if (transform.needsTransition()) {
        transform.updateTransitions(animationTime);
    }

    const TransformState oldState = state;
    state = transform.currentState();

    bool pixelRatioChanged = oldState.getPixelRatio() != state.getPixelRatio();
    bool dimensionsChanged = oldState.getFramebufferWidth() != state.getFramebufferWidth() ||
                             oldState.getFramebufferHeight() != state.getFramebufferHeight();

    if (pixelRatioChanged || dimensionsChanged) {
        painter.clearFramebuffers();
    }

    animationTime = util::now();
    updateSources();
    style->updateProperties(state.getNormalizedZoom(), animationTime);

    // Allow the sprite atlas to potentially pull new sprite images if needed.
    spriteAtlas->resize(state.getPixelRatio());
    spriteAtlas->update(*getSprite());

    updateTiles();
}

void Map::render() {
#if defined(DEBUG)
    std::vector<std::string> debug;
#endif
    painter.clear();

    painter.resetFramebuffer();

    painter.resize();

    painter.changeMatrix();

    updateRenderState();

    painter.drawClippingMasks(getActiveSources());

    // Actually render the layers
    if (debug::renderTree) { std::cout << "{" << std::endl; indent++; }
    renderLayers(style->layers);
    if (debug::renderTree) { std::cout << "}" << std::endl; indent--; }

    // Finalize the rendering, e.g. by calling debug render calls per tile.
    // This guarantees that we have at least one function per tile called.
    // When only rendering layers via the stylesheet, it's possible that we don't
    // ever visit a tile during rendering.
    for (const std::shared_ptr<StyleSource> &source : getActiveSources()) {
        source->source->finishRender(painter);
    }

    // Schedule another rerender when we definitely need a next frame.
    if (transform.needsTransition() || style->hasTransitions()) {
        update();
    }

    glFlush();
}

void Map::renderLayers(std::shared_ptr<StyleLayerGroup> group) {
    if (!group) {
        // Make sure that we actually do have a layer group.
        return;
    }

    // TODO: Correctly compute the number of layers recursively beforehand.
    float strata_thickness = 1.0f / (group->layers.size() + 1);

    // - FIRST PASS ------------------------------------------------------------
    // Render everything top-to-bottom by using reverse iterators. Render opaque
    // objects first.

    if (debug::renderTree) {
        std::cout << std::string(indent++ * 4, ' ') << "OPAQUE {" << std::endl;
    }
    int i = 0;
    for (auto it = group->layers.rbegin(), end = group->layers.rend(); it != end; ++it, ++i) {
        painter.setOpaque();
        painter.setStrata(i * strata_thickness);
        renderLayer(*it, Opaque);
    }
    if (debug::renderTree) {
        std::cout << std::string(--indent * 4, ' ') << "}" << std::endl;
    }

    // - SECOND PASS -----------------------------------------------------------
    // Make a second pass, rendering translucent objects. This time, we render
    // bottom-to-top.
    if (debug::renderTree) {
        std::cout << std::string(indent++ * 4, ' ') << "TRANSLUCENT {" << std::endl;
    }
    --i;
    for (auto it = group->layers.begin(), end = group->layers.end(); it != end; ++it, --i) {
        painter.setTranslucent();
        painter.setStrata(i * strata_thickness);
        renderLayer(*it, Translucent);
    }
    if (debug::renderTree) {
        std::cout << std::string(--indent * 4, ' ') << "}" << std::endl;
    }
}

void Map::renderLayer(std::shared_ptr<StyleLayer> layer_desc, RenderPass pass) {
    if (layer_desc->layers) {
        // This is a layer group. We render them during our translucent render pass.
        if (pass == Translucent) {
            const CompositeProperties &properties = layer_desc->getProperties<CompositeProperties>();
            if (properties.isVisible()) {
                gl::group group(std::string("group: ") + layer_desc->id);

                if (debug::renderTree) {
                    std::cout << std::string(indent++ * 4, ' ') << "+ " << layer_desc->id
                              << " (Composite) {" << std::endl;
                }

                painter.pushFramebuffer();

                renderLayers(layer_desc->layers);

                GLuint texture = painter.popFramebuffer();

                // Render the previous texture onto the screen.
                painter.drawComposite(texture, properties);

                if (debug::renderTree) {
                    std::cout << std::string(--indent * 4, ' ') << "}" << std::endl;
                }
            }
        }
    } else if (layer_desc->type == StyleLayerType::Background) {
        // This layer defines the background color.
    } else {
        // This is a singular layer.
        if (!layer_desc->bucket) {
            fprintf(stderr, "[WARNING] layer '%s' is missing bucket\n", layer_desc->id.c_str());
            return;
        }

        if (!layer_desc->bucket->style_source) {
            fprintf(stderr, "[WARNING] can't find source for layer '%s'\n", layer_desc->id.c_str());
            return;
        }

        StyleSource &style_source = *layer_desc->bucket->style_source;

        // Skip this layer if there is no data.
        if (!style_source.source) {
            return;
        }

        // Skip this layer if it's outside the range of min/maxzoom.
        // This may occur when there /is/ a bucket created for this layer, but the min/max-zoom
        // is set to a fractional value, or value that is larger than the source maxzoom.
        const double zoom = state.getZoom();
        if (layer_desc->bucket->min_zoom > zoom ||
            layer_desc->bucket->max_zoom <= zoom) {
            return;
        }

        // Abort early if we can already deduce from the bucket type that
        // we're not going to render anything anyway during this pass.
        switch (layer_desc->type) {
            case StyleLayerType::Fill:
                if (!layer_desc->getProperties<FillProperties>().isVisible()) return;
                break;
            case StyleLayerType::Line:
                if (pass == Opaque) return;
                if (!layer_desc->getProperties<LineProperties>().isVisible()) return;
                break;
            case StyleLayerType::Symbol:
                if (pass == Opaque) return;
                if (!layer_desc->getProperties<SymbolProperties>().isVisible()) return;
                break;
            case StyleLayerType::Raster:
                if (pass == Translucent) return;
                if (!layer_desc->getProperties<RasterProperties>().isVisible()) return;
                break;
            default:
                break;
        }

        if (debug::renderTree) {
            std::cout << std::string(indent * 4, ' ') << "- " << layer_desc->id << " ("
                      << layer_desc->type << ")" << std::endl;
        }

        style_source.source->render(painter, layer_desc);
    }
}

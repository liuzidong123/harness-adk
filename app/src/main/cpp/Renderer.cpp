#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <android/imagedecoder.h>
#include <cstdlib>

#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"

#define PRINT_GL_STRING(s) {aout << #s": "<< glGetString(s) << std::endl;}

#define PRINT_GL_STRING_AS_LIST(s) { \
std::istringstream extensionStream((const char *) glGetString(s));\
std::vector<std::string> extensionList(\
        std::istream_iterator<std::string>{extensionStream},\
        std::istream_iterator<std::string>());\
aout << #s":\n";\
for (auto& extension: extensionList) {\
    aout << extension << "\n";\
}\
aout << std::endl;\
}

#define CORNFLOWER_BLUE 100 / 255.f, 149 / 255.f, 237 / 255.f, 1

static const char *vertex = R"vertex(#version 300 es
in vec3 inPosition;
in vec2 inUV;

out vec2 fragUV;

uniform mat4 uProjection;

void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4(inPosition, 1.0);
}
)vertex";

static const char *fragment = R"fragment(#version 300 es
precision mediump float;

in vec2 fragUV;

uniform sampler2D uTexture;

out vec4 outColor;

void main() {
    outColor = texture(uTexture, fragUV);
}
)fragment";

static constexpr float kProjectionHalfHeight = 2.f;
static constexpr float kProjectionNearPlane = -1.f;
static constexpr float kProjectionFarPlane = 1.f;

// Snake grid layout constants
static constexpr float kGridLeft = 0.13f;
static constexpr float kGridBottom = 0.14f;
static constexpr float kCellSize = 0.037f;

// Speed levels: 1=slow … 4=fast
static constexpr float kMoveIntervals[4] = {0.30f, 0.20f, 0.12f, 0.07f};

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::render() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastFrameTime_).count();
    lastFrameTime_ = now;

    updateRenderArea();

    // Game projection
    if (shaderNeedsNewProjectionMatrix_) {
        float projectionMatrix[16] = {0};
        Utility::buildOrthographicMatrix(
                projectionMatrix,
                kProjectionHalfHeight,
                float(width_) / height_,
                kProjectionNearPlane,
                kProjectionFarPlane);
        shader_->setProjectionMatrix(projectionMatrix);
        shaderNeedsNewProjectionMatrix_ = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    // Game models
    for (const auto &model: models_) {
        shader_->drawModel(model);
    }

    // UI projection mapping [0,1] normalized coords to NDC [-1,1]
    float uiProj[16] = {0};
    uiProj[0] = 2.0f;
    uiProj[5] = 2.0f;
    uiProj[10] = -1.0f;
    uiProj[12] = -1.0f;
    uiProj[13] = -1.0f;
    uiProj[15] = 1.0f;
    shader_->setProjectionMatrix(uiProj);

    // Snake game — always rendered behind UI, only updates during PLAYING
    if (gameState_ == GameState::PLAYING) {
        updateSnake(dt);
    }
    if (!snakeSegments_.empty()) {
        renderSnake();
    }

    // UI buttons and labels
    for (size_t i = 0; i < buttons_.size(); i++) {
        if (buttons_[i].visibleIn == gameState_) {
            shader_->drawModel(uiModels_[i]);
        }
    }

    // Speed controls in MENU
    if (gameState_ == GameState::MENU) {
        shader_->drawModel(*speedDownModel_);
        shader_->drawModel(*speedLabelModel_);
        shader_->drawModel(*speedUpModel_);
    }

    // Score in PLAYING
    if (gameState_ == GameState::PLAYING && scoreModel_) {
        shader_->drawModel(*scoreModel_);
    }

    shaderNeedsNewProjectionMatrix_ = true;

    auto swapResult = eglSwapBuffers(display_, surface_);
    assert(swapResult == EGL_TRUE);
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint numConfigs;
    eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);

    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + numConfigs,
            [&display](const EGLConfig &config) {
                EGLint red, green, blue, depth;
                if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth)) {

                    aout << "Found config with " << red << ", " << green << ", " << blue << ", "
                         << depth << std::endl;
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    aout << "Found " << numConfigs << " configs" << std::endl;
    aout << "Chose " << config << std::endl;

    EGLint format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);

    auto madeCurrent = eglMakeCurrent(display, surface, surface, context);
    assert(madeCurrent);

    display_ = display;
    surface_ = surface;
    context_ = context;

    eglQuerySurface(display_, surface_, EGL_WIDTH, &width_);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height_);

    PRINT_GL_STRING(GL_VENDOR);
    PRINT_GL_STRING(GL_RENDERER);
    PRINT_GL_STRING(GL_VERSION);
    PRINT_GL_STRING_AS_LIST(GL_EXTENSIONS);

    shader_ = std::unique_ptr<Shader>(
            Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection"));
    assert(shader_);

    shader_->activate();

    glClearColor(CORNFLOWER_BLUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    createModels();
    createUI();
    createSnakeAssets();
    createSpeedControls();
    initSnake();

    // Initialize audio engine
    audioEngine_ = std::unique_ptr<AudioEngine>(new AudioEngine());
    audioEngine_->init(app_);
}

void Renderer::updateRenderArea() {
    EGLint width;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);

    EGLint height;
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);

    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
        shaderNeedsNewProjectionMatrix_ = true;
    }
}

void Renderer::createModels() {
    std::vector<Vertex> vertices = {
            Vertex(Vector3{1, 1, 0}, Vector2{1, 0}),
            Vertex(Vector3{-1, 1, 0}, Vector2{0, 0}),
            Vertex(Vector3{-1, -1, 0}, Vector2{0, 1}),
            Vertex(Vector3{1, -1, 0}, Vector2{1, 1})
    };
    std::vector<Index> indices = {
            0, 1, 2, 0, 2, 3
    };

    auto assetManager = app_->activity->assetManager;
    auto spAndroidRobotTexture = TextureAsset::loadAsset(assetManager, "android_robot.png");

    models_.emplace_back(vertices, indices, spAndroidRobotTexture);
}

void Renderer::createUI() {
    int fontScale = std::max(2, std::min(4, width_ / 960));

    buttons_ = {
        // MENU
        {0.5f, 0.50f, 0.3f, 0.10f, GameState::MENU, GameState::PLAYING},
        // PLAYING
        {0.15f, 0.08f, 0.14f, 0.07f, GameState::PLAYING, GameState::PAUSED},
        {0.85f, 0.08f, 0.14f, 0.07f, GameState::PLAYING, GameState::GAME_OVER},
        // PAUSED
        {0.5f, 0.36f, 0.3f, 0.08f, GameState::PAUSED, GameState::PLAYING},
        {0.5f, 0.48f, 0.3f, 0.08f, GameState::PAUSED, GameState::MENU},
        {0.5f, 0.60f, 0.3f, 0.08f, GameState::PAUSED, GameState::GAME_OVER},
        // GAME_OVER
        {0.5f, 0.42f, 0.3f, 0.08f, GameState::GAME_OVER, GameState::PLAYING},
        {0.5f, 0.54f, 0.3f, 0.08f, GameState::GAME_OVER, GameState::MENU},
        // State labels
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::MENU, GameState::MENU},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::PLAYING, GameState::PLAYING},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::PAUSED, GameState::PAUSED},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::GAME_OVER, GameState::GAME_OVER},
    };

    struct ButtonStyle { uint32_t color; const char* label; };
    ButtonStyle styles[] = {
        {0x4CAF50FF, "START"},
        {0x9E9E9EFF, "PAUSE"},
        {0xF44336FF, "STOP"},
        {0xFFC107FF, "RESUME"},
        {0x2196F3FF, "MENU"},
        {0xF44336FF, "STOP"},
        {0x4CAF50FF, "AGAIN"},
        {0xF44336FF, "MENU"},
        {0x333333FF, "SNAKESHOT"},
        {0x333333FF, "PLAYING"},
        {0x333333FF, "PAUSED"},
        {0x333333FF, "GAME OVER"},
    };

    for (size_t i = 0; i < buttons_.size(); i++) {
        const auto &btn = buttons_[i];
        float hw = btn.w / 2.0f;
        float hh = btn.h / 2.0f;
        float l = btn.x - hw, r = btn.x + hw;
        float b = btn.y - hh, t = btn.y + hh;

        // UV mapping: pixel data is top-down, OpenGL expects bottom-up, so V is flipped
        std::vector<Vertex> verts = {
            Vertex(Vector3{r, t, 0}, Vector2{1, 0}),
            Vertex(Vector3{l, t, 0}, Vector2{0, 0}),
            Vertex(Vector3{l, b, 0}, Vector2{0, 1}),
            Vertex(Vector3{r, b, 0}, Vector2{1, 1}),
        };
        std::vector<Index> idx = {0, 1, 2, 0, 2, 3};

        auto s = styles[i];
        auto c = s.color;
        auto spTex = TextureAsset::createText(
                (c >> 24) & 0xFF, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, s.label, fontScale);
        uiModels_.emplace_back(verts, idx, spTex);
    }
}

void Renderer::createSnakeAssets() {
    uint8_t headR = 0x4C, headG = 0xAF, headB = 0x50; // green
    spSnakeHeadTex_ = TextureAsset::createColor(headR, headG, headB, 255);
    uint8_t bodyR = 0x38, bodyG = 0x8E, bodyB = 0x3C; // darker green
    spSnakeBodyTex_ = TextureAsset::createColor(bodyR, bodyG, bodyB, 255);
    uint8_t foodR = 0xF4, foodG = 0x43, foodB = 0x36; // red
    spSnakeFoodTex_ = TextureAsset::createColor(foodR, foodG, foodB, 255);
    uint8_t bgR = 0x22, bgG = 0x22, bgB = 0x22; // dark grey
    spGridBgTex_ = TextureAsset::createColor(bgR, bgG, bgB, 200);
}

void Renderer::createSpeedControls() {
    speedLevel_ = 2;
    int fontScale = std::max(2, std::min(4, width_ / 960));

    // Pre-build 4 speed label textures: "SPEED 1" … "SPEED 4"
    for (int i = 0; i < 4; i++) {
        char txt[16];
        txt[0] = 'S'; txt[1] = 'P'; txt[2] = 'E'; txt[3] = 'E'; txt[4] = 'D'; txt[5] = ' ';
        txt[6] = '0' + char(i + 1); txt[7] = 0;
        spSpeedLabels_[i] = TextureAsset::createText(0x33, 0x33, 0x33, 0xFF, txt, fontScale);
    }

    // Helper: build a quad model from center/size
    auto makeQuad = [&](float cx, float cy, float w, float h,
                        const std::shared_ptr<TextureAsset> &tex) -> Model {
        float hw = w / 2.0f, hh = h / 2.0f;
        std::vector<Vertex> verts = {
            Vertex(Vector3{cx + hw, cy + hh, 0}, Vector2{1, 0}),
            Vertex(Vector3{cx - hw, cy + hh, 0}, Vector2{0, 0}),
            Vertex(Vector3{cx - hw, cy - hh, 0}, Vector2{0, 1}),
            Vertex(Vector3{cx + hw, cy - hh, 0}, Vector2{1, 1}),
        };
        std::vector<Index> idx = {0, 1, 2, 0, 2, 3};
        return Model(verts, idx, tex);
    };

    auto spMinus = TextureAsset::createText(0x44, 0x44, 0x44, 0xFF, "-", fontScale);
    auto spPlus  = TextureAsset::createText(0x44, 0x44, 0x44, 0xFF, "+", fontScale);

    speedDownModel_  = std::unique_ptr<Model>(new Model(makeQuad(0.28f, 0.32f, 0.08f, 0.08f, spMinus)));
    speedLabelModel_ = std::unique_ptr<Model>(new Model(makeQuad(0.50f, 0.32f, 0.18f, 0.08f, spSpeedLabels_[speedLevel_ - 1])));
    speedUpModel_    = std::unique_ptr<Model>(new Model(makeQuad(0.72f, 0.32f, 0.08f, 0.08f, spPlus)));
}

void Renderer::updateSpeedLabel() {
    char txt[16];
    int n = speedLevel_;
    txt[0] = 'S'; txt[1] = 'P'; txt[2] = 'E'; txt[3] = 'E'; txt[4] = 'D'; txt[5] = ' ';
    txt[6] = '0' + char(n); txt[7] = 0;
    int fontScale = std::max(2, std::min(4, width_ / 960));
    auto newTex = TextureAsset::createText(0x33, 0x33, 0x33, 0xFF, txt, fontScale);

    float cx = 0.50f, cy = 0.32f, hw = 0.09f, hh = 0.04f;
    std::vector<Vertex> verts = {
        Vertex(Vector3{cx + hw, cy + hh, 0}, Vector2{1, 0}),
        Vertex(Vector3{cx - hw, cy + hh, 0}, Vector2{0, 0}),
        Vertex(Vector3{cx - hw, cy - hh, 0}, Vector2{0, 1}),
        Vertex(Vector3{cx + hw, cy - hh, 0}, Vector2{1, 1}),
    };
    std::vector<Index> idx = {0, 1, 2, 0, 2, 3};
    speedLabelModel_ = std::unique_ptr<Model>(new Model(std::move(verts), std::move(idx), newTex));
}

void Renderer::initSnake() {
    snakeSegments_.clear();
    int mid = gridSize_ / 2;
    snakeSegments_.push_back({mid, mid});
    snakeSegments_.push_back({mid - 1, mid});
    snakeSegments_.push_back({mid - 2, mid});
    snakeDir_ = SnakeDir::RIGHT;
    nextDir_ = SnakeDir::RIGHT;
    moveTimer_ = 0;
    moveInterval_ = kMoveIntervals[speedLevel_ - 1];
    score_ = 0;
    updateScoreLabel();
    gameOver_ = false;
    spawnFood();
}

void Renderer::spawnFood() {
    // Find a random empty cell
    std::vector<GridPos> empty;
    empty.reserve(gridSize_ * gridSize_);
    for (int x = 0; x < gridSize_; x++) {
        for (int y = 0; y < gridSize_; y++) {
            bool occupied = false;
            for (const auto &seg : snakeSegments_) {
                if (seg.first == x && seg.second == y) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied) {
                empty.push_back({x, y});
            }
        }
    }
    if (!empty.empty()) {
        foodPos_ = empty[rand() % empty.size()];
    }
}

void Renderer::updateSnake(float dt) {
    if (gameOver_) return;

    moveTimer_ += dt;
    if (moveTimer_ >= moveInterval_) {
        moveTimer_ -= moveInterval_;

        // Apply queued direction (prevent 180-degree reversal)
        if ((nextDir_ == SnakeDir::UP && snakeDir_ != SnakeDir::DOWN) ||
            (nextDir_ == SnakeDir::DOWN && snakeDir_ != SnakeDir::UP) ||
            (nextDir_ == SnakeDir::LEFT && snakeDir_ != SnakeDir::RIGHT) ||
            (nextDir_ == SnakeDir::RIGHT && snakeDir_ != SnakeDir::LEFT)) {
            snakeDir_ = nextDir_;
        }

        auto head = snakeSegments_.front();
        switch (snakeDir_) {
            case SnakeDir::UP:    head.second++; break;
            case SnakeDir::DOWN:  head.second--; break;
            case SnakeDir::LEFT:  head.first--; break;
            case SnakeDir::RIGHT: head.first++; break;
        }

        // Wall collision → score -1, reverse direction, no game over
        if (head.first < 0 || head.first >= gridSize_ ||
            head.second < 0 || head.second >= gridSize_) {
            score_--;
            updateScoreLabel();
            switch (snakeDir_) {
                case SnakeDir::UP:    snakeDir_ = SnakeDir::DOWN;  break;
                case SnakeDir::DOWN:  snakeDir_ = SnakeDir::UP;    break;
                case SnakeDir::LEFT:  snakeDir_ = SnakeDir::RIGHT; break;
                case SnakeDir::RIGHT: snakeDir_ = SnakeDir::LEFT;  break;
            }
            nextDir_ = snakeDir_;
            return;
        }

        // Self collision (check against current body before adding new head)
        for (const auto &seg : snakeSegments_) {
            if (seg.first == head.first && seg.second == head.second) {
                score_ = 0;
                updateScoreLabel();
                gameState_ = GameState::GAME_OVER;
                if (audioEngine_) audioEngine_->playGameOver();
                return;
            }
        }

        snakeSegments_.push_front(head);

        // Food check
        if (head.first == foodPos_.first && head.second == foodPos_.second) {
            score_++;
            updateScoreLabel();
            if (audioEngine_) audioEngine_->playEat();
            spawnFood();
        } else {
            snakeSegments_.pop_back();
        }
    }
}

float Renderer::gridToUIX(int gx) const {
    return kGridLeft + (gx + 0.5f) * kCellSize;
}

float Renderer::gridToUIY(int gy) const {
    return kGridBottom + (gy + 0.5f) * kCellSize;
}

Model Renderer::makeCellModel(int gx, int gy, const std::shared_ptr<TextureAsset> &tex, bool centered) const {
    float cx = gridToUIX(gx);
    float cy = gridToUIY(gy);
    float halfSize;
    if (centered) {
        halfSize = kCellSize * 0.45f;
    } else {
        halfSize = kCellSize * 0.4f;
    }

    std::vector<Vertex> verts = {
        Vertex(Vector3{cx + halfSize, cy + halfSize, 0}, Vector2{1, 1}),
        Vertex(Vector3{cx - halfSize, cy + halfSize, 0}, Vector2{0, 1}),
        Vertex(Vector3{cx - halfSize, cy - halfSize, 0}, Vector2{0, 0}),
        Vertex(Vector3{cx + halfSize, cy - halfSize, 0}, Vector2{1, 0}),
    };
    std::vector<Index> idx = {0, 1, 2, 0, 2, 3};
    return Model(verts, idx, tex);
}

void Renderer::renderSnake() {
    // Grid background
    float bgLeft = kGridLeft;
    float bgRight = kGridLeft + gridSize_ * kCellSize;
    float bgBottom = kGridBottom;
    float bgTop = kGridBottom + gridSize_ * kCellSize;

    std::vector<Vertex> bgVerts = {
        Vertex(Vector3{bgRight, bgTop, 0}, Vector2{1, 1}),
        Vertex(Vector3{bgLeft, bgTop, 0}, Vector2{0, 1}),
        Vertex(Vector3{bgLeft, bgBottom, 0}, Vector2{0, 0}),
        Vertex(Vector3{bgRight, bgBottom, 0}, Vector2{1, 0}),
    };
    std::vector<Index> bgIdx = {0, 1, 2, 0, 2, 3};
    Model bgModel(bgVerts, bgIdx, spGridBgTex_);
    shader_->drawModel(bgModel);

    // Food
    auto foodModel = makeCellModel(foodPos_.first, foodPos_.second, spSnakeFoodTex_, true);
    shader_->drawModel(foodModel);

    // Snake segments
    for (size_t i = 0; i < snakeSegments_.size(); i++) {
        auto &seg = snakeSegments_[i];
        auto tex = (i == 0) ? spSnakeHeadTex_ : spSnakeBodyTex_;
        auto model = makeCellModel(seg.first, seg.second, tex, false);
        shader_->drawModel(model);
    }
}

void Renderer::updateScoreLabel() {
    int fontScale = std::max(2, std::min(4, width_ / 960));
    char buf[32];
    int n = score_;
    buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':'; buf[6] = ' ';
    if (n < 0) { buf[7] = '-'; n = -n; } else { buf[7] = ' '; }
    int pos = 8;
    if (n >= 100) { buf[pos++] = '0' + (n / 100) % 10; }
    if (n >= 10)  { buf[pos++] = '0' + (n / 10) % 10; }
    buf[pos++] = '0' + (n % 10);
    buf[pos] = 0;

    auto newTex = TextureAsset::createText(0, 0, 0, 0, buf, fontScale);
    float cx = 0.08f, cy = 0.92f;
    float hw = 0.14f, hh = 0.05f;
    std::vector<Vertex> verts = {
        Vertex(Vector3{cx + hw, cy + hh, 0}, Vector2{1, 0}),
        Vertex(Vector3{cx - hw, cy + hh, 0}, Vector2{0, 0}),
        Vertex(Vector3{cx - hw, cy - hh, 0}, Vector2{0, 1}),
        Vertex(Vector3{cx + hw, cy - hh, 0}, Vector2{1, 1}),
    };
    std::vector<Index> idx = {0, 1, 2, 0, 2, 3};
    scoreModel_ = std::unique_ptr<Model>(new Model(std::move(verts), std::move(idx), newTex));
}

void Renderer::handleInput() {
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        return;
    }

    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                handleButtonDown(x / float(width_), 1.0f - y / float(height_));
                break;

            case AMOTION_EVENT_ACTION_CANCEL:
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                break;
            default:
                break;
        }
    }
    android_app_clear_motion_events(inputBuffer);

    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        switch (keyEvent.action) {
            case AKEY_EVENT_ACTION_DOWN:
                break;
            case AKEY_EVENT_ACTION_UP:
                break;
            case AKEY_EVENT_ACTION_MULTIPLE:
                break;
            default:
                break;
        }
    }
    android_app_clear_key_events(inputBuffer);
}

bool Renderer::hitTest(const ButtonDef &btn, float nx, float ny) const {
    float hw = btn.w / 2.0f;
    float hh = btn.h / 2.0f;
    return nx >= btn.x - hw && nx <= btn.x + hw &&
           ny >= btn.y - hh && ny <= btn.y + hh;
}

bool Renderer::handleButtonDown(float nx, float ny) {
    // Speed controls in MENU
    if (gameState_ == GameState::MENU) {
        // "-" at (0.28, 0.32, 0.08, 0.08)
        if (nx >= 0.24f && nx <= 0.32f && ny >= 0.28f && ny <= 0.36f) {
            speedLevel_ = std::max(1, speedLevel_ - 1);
            moveInterval_ = kMoveIntervals[speedLevel_ - 1];
            updateSpeedLabel();
            return true;
        }
        // "+" at (0.72, 0.32, 0.08, 0.08)
        if (nx >= 0.68f && nx <= 0.76f && ny >= 0.28f && ny <= 0.36f) {
            speedLevel_ = std::min(4, speedLevel_ + 1);
            moveInterval_ = kMoveIntervals[speedLevel_ - 1];
            updateSpeedLabel();
            return true;
        }
    }

    // Check button hits
    for (const auto &btn : buttons_) {
        if (btn.visibleIn == gameState_ && hitTest(btn, nx, ny)) {
            // Play sound effects based on state transitions
            if (btn.visibleIn == GameState::MENU && btn.targetState == GameState::PLAYING) {
                if (audioEngine_) audioEngine_->playStart();
            } else if (btn.visibleIn == GameState::PLAYING && btn.targetState == GameState::PAUSED) {
                if (audioEngine_) audioEngine_->playPause();
            } else if (btn.visibleIn == GameState::PLAYING && btn.targetState == GameState::GAME_OVER) {
                if (audioEngine_) audioEngine_->playGameOver();
            }

            if (btn.targetState == GameState::PLAYING &&
                (btn.visibleIn == GameState::MENU || btn.visibleIn == GameState::GAME_OVER)) {
                initSnake();
            }
            gameState_ = btn.targetState;
            return true;
        }
    }

    // During PLAYING, tap on grid area → direction change relative to snake head
    if (gameState_ == GameState::PLAYING && !snakeSegments_.empty()) {
        auto head = snakeSegments_.front();
        float headX = gridToUIX(head.first);
        float headY = gridToUIY(head.second);
        float dx = nx - headX;
        float dy = ny - headY;
        if (std::abs(dx) > std::abs(dy)) {
            nextDir_ = dx > 0 ? SnakeDir::RIGHT : SnakeDir::LEFT;
        } else {
            nextDir_ = dy > 0 ? SnakeDir::UP : SnakeDir::DOWN;
        }
        return true;
    }

    return false;
}

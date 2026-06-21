#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>
#include <cstdint>
#include <deque>
#include <chrono>
#include <utility>

#include "Model.h"
#include "Shader.h"

struct android_app;

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

struct ButtonDef {
    float x, y, w, h;
    GameState visibleIn;
    GameState targetState;
};

using GridPos = std::pair<int,int>;

class Renderer {
public:
    inline Renderer(android_app *pApp) :
            app_(pApp),
            display_(EGL_NO_DISPLAY),
            surface_(EGL_NO_SURFACE),
            context_(EGL_NO_CONTEXT),
            width_(0),
            height_(0),
            shaderNeedsNewProjectionMatrix_(true),
            gameState_(GameState::MENU),
            lastFrameTime_(std::chrono::steady_clock::now()) {
        initRenderer();
    }

    virtual ~Renderer();

    void handleInput();
    void render();

private:
    void initRenderer();
    void updateRenderArea();
    void createModels();
    void createUI();
    void createSnakeAssets();

    bool hitTest(const ButtonDef &btn, float nx, float ny) const;
    bool handleButtonDown(float nx, float ny);

    // Snake game
    void initSnake();
    void spawnFood();
    void updateSnake(float dt);
    void renderSnake();

    float gridToUIX(int gx) const;
    float gridToUIY(int gy) const;
    Model makeCellModel(int gx, int gy, const std::shared_ptr<TextureAsset> &tex, bool centered) const;

    // Speed control
    void createSpeedControls();
    void updateSpeedLabel();
    int speedLevel_;
    std::shared_ptr<TextureAsset> spSpeedLabels_[4];
    std::unique_ptr<Model> speedDownModel_;
    std::unique_ptr<Model> speedUpModel_;
    std::unique_ptr<Model> speedLabelModel_;

    enum class SnakeDir { UP, DOWN, LEFT, RIGHT };

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;
    GameState gameState_;

    std::unique_ptr<Shader> shader_;
    std::vector<Model> models_;
    std::vector<Model> uiModels_;
    std::vector<ButtonDef> buttons_;

    // Snake state
    std::deque<GridPos> snakeSegments_;
    SnakeDir snakeDir_;
    SnakeDir nextDir_;
    GridPos foodPos_;
    float moveTimer_;
    float moveInterval_;
    static constexpr int gridSize_ = 20;
    bool gameOver_;

    // Snake assets
    std::shared_ptr<TextureAsset> spSnakeHeadTex_;
    std::shared_ptr<TextureAsset> spSnakeBodyTex_;
    std::shared_ptr<TextureAsset> spSnakeFoodTex_;
    std::shared_ptr<TextureAsset> spGridBgTex_;

    // Timing
    std::chrono::steady_clock::time_point lastFrameTime_;
};

#endif

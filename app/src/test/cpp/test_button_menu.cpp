#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <deque>
#include <utility>
#include <cstdint>
#include <memory>
#include <algorithm>

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

using GridPos = std::pair<int, int>;

static constexpr float kGridLeft = 0.13f;
static constexpr float kGridBottom = 0.14f;
static constexpr float kCellSize = 0.037f;
static constexpr float kMoveIntervals[4] = {0.30f, 0.20f, 0.12f, 0.07f};

// ============================================================
// White-box unit: hitTest
// ============================================================
static bool hitTest(const ButtonDef &btn, float nx, float ny) {
    float hw = btn.w / 2.0f;
    float hh = btn.h / 2.0f;
    return nx >= btn.x - hw && nx <= btn.x + hw &&
           ny >= btn.y - hh && ny <= btn.y + hh;
}

TEST(HitTest, CenterHit) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_TRUE(hitTest(btn, 0.5f, 0.5f));
}

TEST(HitTest, LeftEdge) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_TRUE(hitTest(btn, 0.35f, 0.5f));
}

TEST(HitTest, RightEdge) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_TRUE(hitTest(btn, 0.65f, 0.5f));
}

TEST(HitTest, BottomEdge) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_TRUE(hitTest(btn, 0.5f, 0.45f));
}

TEST(HitTest, TopEdge) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_TRUE(hitTest(btn, 0.5f, 0.55f));
}

TEST(HitTest, LeftOutside) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_FALSE(hitTest(btn, 0.349f, 0.5f));
}

TEST(HitTest, RightOutside) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_FALSE(hitTest(btn, 0.651f, 0.5f));
}

TEST(HitTest, BottomOutside) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_FALSE(hitTest(btn, 0.5f, 0.449f));
}

TEST(HitTest, TopOutside) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_FALSE(hitTest(btn, 0.5f, 0.551f));
}

TEST(HitTest, FarMiss) {
    ButtonDef btn{0.5f, 0.5f, 0.3f, 0.1f};
    EXPECT_FALSE(hitTest(btn, 0.0f, 0.0f));
}

TEST(HitTest, NarrowButtonCenter) {
    ButtonDef btn{0.15f, 0.08f, 0.14f, 0.07f};
    EXPECT_TRUE(hitTest(btn, 0.15f, 0.08f));
}

TEST(HitTest, NarrowButtonEdge) {
    ButtonDef btn{0.15f, 0.08f, 0.14f, 0.07f};
    EXPECT_TRUE(hitTest(btn, 0.22f, 0.08f));
}

TEST(HitTest, NarrowButtonOutside) {
    ButtonDef btn{0.15f, 0.08f, 0.14f, 0.07f};
    EXPECT_FALSE(hitTest(btn, 0.23f, 0.08f));
}

// ============================================================
// White-box unit: handleButtonDown — state machine transitions
// ============================================================
enum class SnakeDir { UP, DOWN, LEFT, RIGHT };
struct GameStateFixture {
    GameState state;
    std::vector<ButtonDef> buttons;
    std::deque<GridPos> snakeSegments;
    int speedLevel;
    float moveInterval;
    // direction input state
    SnakeDir snakeDir, nextDir;

    // Constants
    static constexpr int gridSize = 20;

    GameStateFixture() : state(GameState::MENU), speedLevel(2),
        moveInterval(kMoveIntervals[1]), snakeDir(SnakeDir::RIGHT),
        nextDir(SnakeDir::RIGHT) {
        buttons = {
            {0.5f, 0.50f, 0.3f, 0.10f, GameState::MENU, GameState::PLAYING},
            {0.15f, 0.08f, 0.14f, 0.07f, GameState::PLAYING, GameState::PAUSED},
            {0.85f, 0.08f, 0.14f, 0.07f, GameState::PLAYING, GameState::GAME_OVER},
            {0.5f, 0.36f, 0.3f, 0.08f, GameState::PAUSED, GameState::PLAYING},
            {0.5f, 0.48f, 0.3f, 0.08f, GameState::PAUSED, GameState::MENU},
            {0.5f, 0.60f, 0.3f, 0.08f, GameState::PAUSED, GameState::GAME_OVER},
            {0.5f, 0.42f, 0.3f, 0.08f, GameState::GAME_OVER, GameState::PLAYING},
            {0.5f, 0.54f, 0.3f, 0.08f, GameState::GAME_OVER, GameState::MENU},
            {0.5f, 0.88f, 0.3f, 0.07f, GameState::MENU, GameState::MENU},
            {0.5f, 0.88f, 0.3f, 0.07f, GameState::PLAYING, GameState::PLAYING},
            {0.5f, 0.88f, 0.3f, 0.07f, GameState::PAUSED, GameState::PAUSED},
            {0.5f, 0.88f, 0.3f, 0.07f, GameState::GAME_OVER, GameState::GAME_OVER},
        };
        initSnake();
    }

    void initSnake() {
        snakeSegments.clear();
        int mid = gridSize / 2;
        snakeSegments.push_back({mid, mid});
        snakeSegments.push_back({mid - 1, mid});
        snakeSegments.push_back({mid - 2, mid});
    }

    float gridToUIX(int gx) const {
        return kGridLeft + (gx + 0.5f) * kCellSize;
    }
    float gridToUIY(int gy) const {
        return kGridBottom + (gy + 0.5f) * kCellSize;
    }

    bool handleButtonDown(float nx, float ny) {
        if (state == GameState::MENU) {
            if (nx >= 0.24f && nx <= 0.32f && ny >= 0.28f && ny <= 0.36f) {
                speedLevel = std::max(1, speedLevel - 1);
                moveInterval = kMoveIntervals[speedLevel - 1];
                return true;
            }
            if (nx >= 0.68f && nx <= 0.76f && ny >= 0.28f && ny <= 0.36f) {
                speedLevel = std::min(4, speedLevel + 1);
                moveInterval = kMoveIntervals[speedLevel - 1];
                return true;
            }
        }
        for (const auto &btn : buttons) {
            if (btn.visibleIn == state && hitTest(btn, nx, ny)) {
                if (btn.targetState == GameState::PLAYING &&
                    (btn.visibleIn == GameState::MENU || btn.visibleIn == GameState::GAME_OVER)) {
                    initSnake();
                }
                state = btn.targetState;
                return true;
            }
        }
        if (state == GameState::PLAYING && !snakeSegments.empty()) {
            auto head = snakeSegments.front();
            float headX = gridToUIX(head.first);
            float headY = gridToUIY(head.second);
            float dx = nx - headX;
            float dy = ny - headY;
            if (std::abs(dx) > std::abs(dy)) {
                nextDir = dx > 0 ? SnakeDir::RIGHT : SnakeDir::LEFT;
            } else {
                nextDir = dy > 0 ? SnakeDir::UP : SnakeDir::DOWN;
            }
            return true;
        }
        return false;
    }

};

TEST(StateMachine, MenuStartToPlaying) {
    GameStateFixture f;
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.5f));
    EXPECT_EQ(f.state, GameState::PLAYING);
    EXPECT_EQ(f.snakeSegments.size(), 3);
}

TEST(StateMachine, PlayingPauseToPaused) {
    GameStateFixture f;
    f.state = GameState::PLAYING;
    EXPECT_TRUE(f.handleButtonDown(0.15f, 0.08f));
    EXPECT_EQ(f.state, GameState::PAUSED);
}

TEST(StateMachine, PlayingStopToGameOver) {
    GameStateFixture f;
    f.state = GameState::PLAYING;
    EXPECT_TRUE(f.handleButtonDown(0.85f, 0.08f));
    EXPECT_EQ(f.state, GameState::GAME_OVER);
}

TEST(StateMachine, PausedResumeToPlaying) {
    GameStateFixture f;
    f.state = GameState::PAUSED;
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.36f));
    EXPECT_EQ(f.state, GameState::PLAYING);
}

TEST(StateMachine, PausedMenuToMenu) {
    GameStateFixture f;
    f.state = GameState::PAUSED;
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.48f));
    EXPECT_EQ(f.state, GameState::MENU);
}

TEST(StateMachine, PausedStopToGameOver) {
    GameStateFixture f;
    f.state = GameState::PAUSED;
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.60f));
    EXPECT_EQ(f.state, GameState::GAME_OVER);
}

TEST(StateMachine, GameOverAgainToPlaying) {
    GameStateFixture f;
    f.state = GameState::GAME_OVER;
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.42f));
    EXPECT_EQ(f.state, GameState::PLAYING);
    EXPECT_EQ(f.snakeSegments.size(), 3);
}

TEST(StateMachine, GameOverMenuToMenu) {
    GameStateFixture f;
    f.state = GameState::GAME_OVER;
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.54f));
    EXPECT_EQ(f.state, GameState::MENU);
}

TEST(StateMachine, TapOutsideButtonsNoChange) {
    GameStateFixture f;
    f.state = GameState::MENU;
    EXPECT_FALSE(f.handleButtonDown(0.10f, 0.10f));
    EXPECT_EQ(f.state, GameState::MENU);
}

TEST(StateMachine, InitSnakeOnMenuStartToPlaying) {
    GameStateFixture f;
    f.snakeSegments.push_back({99, 99}); // corrupt state
    EXPECT_TRUE(f.handleButtonDown(0.5f, 0.5f));
    EXPECT_EQ(f.state, GameState::PLAYING);
    EXPECT_EQ(f.snakeSegments.size(), 3);
    EXPECT_EQ(f.snakeSegments[0], GridPos(10, 10));
}

// ============================================================
// Speed control
// ============================================================
TEST(SpeedControl, Increment3to4) {
    GameStateFixture f;
    f.speedLevel = 3;
    EXPECT_TRUE(f.handleButtonDown(0.72f, 0.32f));
    EXPECT_EQ(f.speedLevel, 4);
    EXPECT_FLOAT_EQ(f.moveInterval, 0.07f);
}

TEST(SpeedControl, Increment2to3) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_TRUE(f.handleButtonDown(0.72f, 0.32f));
    EXPECT_EQ(f.speedLevel, 3);
    EXPECT_FLOAT_EQ(f.moveInterval, 0.12f);
}

TEST(SpeedControl, Decrement2to1) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_TRUE(f.handleButtonDown(0.28f, 0.32f));
    EXPECT_EQ(f.speedLevel, 1);
    EXPECT_FLOAT_EQ(f.moveInterval, 0.30f);
}

TEST(SpeedControl, Decrement3to2) {
    GameStateFixture f;
    f.speedLevel = 3;
    EXPECT_TRUE(f.handleButtonDown(0.28f, 0.32f));
    EXPECT_EQ(f.speedLevel, 2);
    EXPECT_FLOAT_EQ(f.moveInterval, 0.20f);
}

TEST(SpeedControl, ClampMin) {
    GameStateFixture f;
    f.speedLevel = 1;
    EXPECT_TRUE(f.handleButtonDown(0.28f, 0.32f));
    EXPECT_EQ(f.speedLevel, 1);
    EXPECT_FLOAT_EQ(f.moveInterval, 0.30f);
}

TEST(SpeedControl, ClampMax) {
    GameStateFixture f;
    f.speedLevel = 4;
    EXPECT_TRUE(f.handleButtonDown(0.72f, 0.32f));
    EXPECT_EQ(f.speedLevel, 4);
    EXPECT_FLOAT_EQ(f.moveInterval, 0.07f);
}

TEST(SpeedControl, HitMinusLeftEdge) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_TRUE(f.handleButtonDown(0.24f, 0.32f));
    EXPECT_EQ(f.speedLevel, 1);
}

TEST(SpeedControl, MissMinusLeftOutside) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_FALSE(f.handleButtonDown(0.239f, 0.32f));
    EXPECT_EQ(f.speedLevel, 2);
}

TEST(SpeedControl, HitPlusRightEdge) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_TRUE(f.handleButtonDown(0.76f, 0.32f));
    EXPECT_EQ(f.speedLevel, 3);
}

TEST(SpeedControl, MissPlusRightOutside) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_FALSE(f.handleButtonDown(0.761f, 0.32f));
    EXPECT_EQ(f.speedLevel, 2);
}

TEST(SpeedControl, HitMinusBottomEdge) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_TRUE(f.handleButtonDown(0.28f, 0.28f));
    EXPECT_EQ(f.speedLevel, 1);
}

TEST(SpeedControl, MissMinusBottomOutside) {
    GameStateFixture f;
    f.speedLevel = 2;
    EXPECT_FALSE(f.handleButtonDown(0.28f, 0.279f));
    EXPECT_EQ(f.speedLevel, 2);
}

// ============================================================
// Grid coordinate mapping
// ============================================================
static float gridToUIX(int gx) {
    return kGridLeft + (gx + 0.5f) * kCellSize;
}

static float gridToUIY(int gy) {
    return kGridBottom + (gy + 0.5f) * kCellSize;
}

TEST(GridCoord, Origin) {
    EXPECT_FLOAT_EQ(gridToUIX(0), 0.1485f);
    EXPECT_FLOAT_EQ(gridToUIY(0), 0.1585f);
}

TEST(GridCoord, Midpoint) {
    EXPECT_FLOAT_EQ(gridToUIX(10), 0.5185f);
    EXPECT_FLOAT_EQ(gridToUIY(10), 0.5285f);
}

TEST(GridCoord, Endpoint) {
    EXPECT_FLOAT_EQ(gridToUIX(19), 0.8515f);
    EXPECT_FLOAT_EQ(gridToUIY(19), 0.8615f);
}

// ============================================================
// Font scaling
// ============================================================
static int calcFontScale(int width) {
    return std::max(2, std::min(4, (width - 800) / 160));
}

TEST(FontScale, SmallScreen) {
    EXPECT_EQ(calcFontScale(720), 2);
    EXPECT_EQ(calcFontScale(960), 2);
}

TEST(FontScale, MediumScreen) {
    EXPECT_EQ(calcFontScale(1280), 3);
}

TEST(FontScale, LargeScreenBoundary) {
    EXPECT_EQ(calcFontScale(1440), 4);
    EXPECT_EQ(calcFontScale(1920), 4);
}

// ============================================================
// Visibility filtering
// ============================================================
static int countVisibleButtons(const std::vector<ButtonDef> &buttons, GameState state) {
    int count = 0;
    for (const auto &btn : buttons) {
        if (btn.visibleIn == state) count++;
    }
    return count;
}

TEST(VisibilityFilter, MenuState) {
    std::vector<ButtonDef> buttons = {
        {0.5f, 0.50f, 0.3f, 0.10f, GameState::MENU, GameState::PLAYING},
        {0.15f, 0.08f, 0.14f, 0.07f, GameState::PLAYING, GameState::PAUSED},
        {0.85f, 0.08f, 0.14f, 0.07f, GameState::PLAYING, GameState::GAME_OVER},
        {0.5f, 0.36f, 0.3f, 0.08f, GameState::PAUSED, GameState::PLAYING},
        {0.5f, 0.48f, 0.3f, 0.08f, GameState::PAUSED, GameState::MENU},
        {0.5f, 0.60f, 0.3f, 0.08f, GameState::PAUSED, GameState::GAME_OVER},
        {0.5f, 0.42f, 0.3f, 0.08f, GameState::GAME_OVER, GameState::PLAYING},
        {0.5f, 0.54f, 0.3f, 0.08f, GameState::GAME_OVER, GameState::MENU},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::MENU, GameState::MENU},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::PLAYING, GameState::PLAYING},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::PAUSED, GameState::PAUSED},
        {0.5f, 0.88f, 0.3f, 0.07f, GameState::GAME_OVER, GameState::GAME_OVER},
    };
    EXPECT_EQ(countVisibleButtons(buttons, GameState::MENU), 2);
    EXPECT_EQ(countVisibleButtons(buttons, GameState::PLAYING), 3);
    EXPECT_EQ(countVisibleButtons(buttons, GameState::PAUSED), 4);
    EXPECT_EQ(countVisibleButtons(buttons, GameState::GAME_OVER), 3);
}

// ============================================================
// Direction input
// ============================================================
static constexpr float kHeadUIX = 0.13f + (10 + 0.5f) * 0.037f; // 0.5185
static constexpr float kHeadUIY = 0.14f + (10 + 0.5f) * 0.037f; // 0.5285

static SnakeDir calcDirection(float nx, float ny, float headX, float headY) {
    float dx = nx - headX;
    float dy = ny - headY;
    if (std::abs(dx) > std::abs(dy)) {
        return dx > 0 ? SnakeDir::RIGHT : SnakeDir::LEFT;
    } else {
        return dy > 0 ? SnakeDir::UP : SnakeDir::DOWN;
    }
}

TEST(DirectionInput, TapRight) {
    EXPECT_EQ(calcDirection(0.70f, kHeadUIY, kHeadUIX, kHeadUIY), SnakeDir::RIGHT);
}

TEST(DirectionInput, TapLeft) {
    EXPECT_EQ(calcDirection(0.30f, kHeadUIY, kHeadUIX, kHeadUIY), SnakeDir::LEFT);
}

TEST(DirectionInput, TapUp) {
    EXPECT_EQ(calcDirection(kHeadUIX, 0.70f, kHeadUIX, kHeadUIY), SnakeDir::UP);
}

TEST(DirectionInput, TapDown) {
    EXPECT_EQ(calcDirection(kHeadUIX, 0.30f, kHeadUIX, kHeadUIY), SnakeDir::DOWN);
}

TEST(DirectionInput, HorizontalPreferredDxGtDy) {
    EXPECT_EQ(calcDirection(0.60f, 0.55f, kHeadUIX, kHeadUIY), SnakeDir::RIGHT);
}

TEST(DirectionInput, VerticalPreferredDyGtDx) {
    EXPECT_EQ(calcDirection(0.52f, 0.60f, kHeadUIX, kHeadUIY), SnakeDir::UP);
}

TEST(DirectionInput, HorizontalEqualFavorsX) {
    EXPECT_EQ(calcDirection(0.60f, 0.60f, kHeadUIX, kHeadUIY), SnakeDir::RIGHT);
}

// ============================================================
// Speed level intervals constant correctness
// ============================================================
TEST(SpeedConstants, IntervalsMatchSpec) {
    EXPECT_FLOAT_EQ(kMoveIntervals[0], 0.30f);
    EXPECT_FLOAT_EQ(kMoveIntervals[1], 0.20f);
    EXPECT_FLOAT_EQ(kMoveIntervals[2], 0.12f);
    EXPECT_FLOAT_EQ(kMoveIntervals[3], 0.07f);
}

// ============================================================
// Init snake state correctness
// ============================================================
TEST(InitSnake, SegmentsCount) {
    GameStateFixture f;
    EXPECT_EQ(f.snakeSegments.size(), 3);
}

TEST(InitSnake, HeadPosition) {
    GameStateFixture f;
    EXPECT_EQ(f.snakeSegments[0], GridPos(10, 10));
    EXPECT_EQ(f.snakeSegments[1], GridPos(9, 10));
    EXPECT_EQ(f.snakeSegments[2], GridPos(8, 10));
}

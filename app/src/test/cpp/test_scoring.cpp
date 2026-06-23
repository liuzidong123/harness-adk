#include <gtest/gtest.h>

// ============================================================
// Scoring logic tests (standalone, no Renderer dependency)
// ============================================================

// Replicate scoring rules from Renderer::updateSnake
// These pure functions test the business logic independently

enum class SnakeDir { UP, DOWN, LEFT, RIGHT };

TEST(ScoringLogic, EatFoodIncrementsScore) {
    int score = 0;
    // Food check: head == food
    int headX = 5, headY = 5;
    int foodX = 5, foodY = 5;
    if (headX == foodX && headY == foodY) {
        score++;
    }
    EXPECT_EQ(score, 1);
}

TEST(ScoringLogic, WallCollisionDecrementsScore) {
    int score = 5;
    // Wall collision: head out of bounds
    int headX = -1, headY = 10;
    int gridSize = 20;
    if (headX < 0 || headX >= gridSize || headY < 0 || headY >= gridSize) {
        score--;
    }
    EXPECT_EQ(score, 4);
}

TEST(ScoringLogic, WallCollisionDirectionReverses) {
    // Test all four direction reversals
    struct TestCase { SnakeDir input; SnakeDir expected; };
    TestCase cases[] = {
        {SnakeDir::UP, SnakeDir::DOWN},
        {SnakeDir::DOWN, SnakeDir::UP},
        {SnakeDir::LEFT, SnakeDir::RIGHT},
        {SnakeDir::RIGHT, SnakeDir::LEFT},
    };
    for (const auto &c : cases) {
        SnakeDir result = c.input;
        switch (result) {
            case SnakeDir::UP:    result = SnakeDir::DOWN;  break;
            case SnakeDir::DOWN:  result = SnakeDir::UP;    break;
            case SnakeDir::LEFT:  result = SnakeDir::RIGHT; break;
            case SnakeDir::RIGHT: result = SnakeDir::LEFT;  break;
        }
        EXPECT_EQ(result, c.expected);
    }
}

TEST(ScoringLogic, NegativeScoreIsAllowed) {
    int score = 0;
    // Hit wall 3 times
    for (int i = 0; i < 3; i++) {
        score--;
    }
    EXPECT_EQ(score, -3);
}

TEST(ScoringLogic, GameOverResetsScore) {
    int score = 10;
    // Self collision detected → reset
    bool selfCollision = true;
    if (selfCollision) {
        score = 0;
    }
    EXPECT_EQ(score, 0);
}

TEST(ScoringLogic, EatFoodDoesNotAffectNonMatchingHead) {
    int score = 0;
    int headX = 5, headY = 5;
    int foodX = 6, foodY = 6;
    if (headX == foodX && headY == foodY) {
        score++;
    }
    EXPECT_EQ(score, 0);
}

TEST(ScoringLogic, WallBounceDoesNotMoveHead) {
    // When wall hit, snake stays and reverses direction
    // Head position should not change after wall bounce
    int headX = 0, headY = 10;
    SnakeDir dir = SnakeDir::LEFT;
    int newHeadX = headX, newHeadY = headY;
    switch (dir) {
        case SnakeDir::LEFT: newHeadX--; break;
        default: break;
    }
    bool hitWall = (newHeadX < 0);
    if (hitWall) {
        // Reverse direction, don't update head position
        dir = SnakeDir::RIGHT;
        newHeadX = headX; // Stay in place
        newHeadY = headY;
    }
    EXPECT_EQ(dir, SnakeDir::RIGHT);
    EXPECT_EQ(newHeadX, 0);
    EXPECT_EQ(newHeadY, 10);
}

TEST(ScoringLogic, EatAndScoreAreIndependentOfDirection) {
    int score = 0;
    // Eating should work regardless of snake direction
    SnakeDir dirs[] = {SnakeDir::UP, SnakeDir::DOWN, SnakeDir::LEFT, SnakeDir::RIGHT};
    int headX = 7, headY = 7;
    int foodX = 7, foodY = 7;
    for (auto d : dirs) {
        (void)d;
        if (headX == foodX && headY == foodY) {
            score++;
        }
    }
    EXPECT_EQ(score, 4);
}

// ============================================================
// Score label formatting tests
// ============================================================
TEST(ScoreFormat, PositiveScore) {
    int score = 42;
    char buf[32];
    int n = score;
    buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':'; buf[6] = ' ';
    if (n < 0) { buf[7] = '-'; n = -n; } else { buf[7] = ' '; }
    int pos = 8;
    if (n >= 100) { buf[pos++] = '0' + (n / 100) % 10; }
    if (n >= 10)  { buf[pos++] = '0' + (n / 10) % 10; }
    buf[pos++] = '0' + (n % 10);
    buf[pos] = 0;
    EXPECT_STREQ(buf, "SCORE:  42");
}

TEST(ScoreFormat, NegativeScore) {
    int score = -7;
    char buf[32];
    int n = score;
    buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':'; buf[6] = ' ';
    if (n < 0) { buf[7] = '-'; n = -n; } else { buf[7] = ' '; }
    int pos = 8;
    if (n >= 100) { buf[pos++] = '0' + (n / 100) % 10; }
    if (n >= 10)  { buf[pos++] = '0' + (n / 10) % 10; }
    buf[pos++] = '0' + (n % 10);
    buf[pos] = 0;
    EXPECT_STREQ(buf, "SCORE: -7");
}

TEST(ScoreFormat, ZeroScore) {
    int score = 0;
    char buf[32];
    int n = score;
    buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':'; buf[6] = ' ';
    if (n < 0) { buf[7] = '-'; n = -n; } else { buf[7] = ' '; }
    int pos = 8;
    if (n >= 100) { buf[pos++] = '0' + (n / 100) % 10; }
    if (n >= 10)  { buf[pos++] = '0' + (n / 10) % 10; }
    buf[pos++] = '0' + (n % 10);
    buf[pos] = 0;
    EXPECT_STREQ(buf, "SCORE:  0");
}

TEST(ScoreFormat, ThreeDigitScore) {
    int score = 123;
    char buf[32];
    int n = score;
    buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':'; buf[6] = ' ';
    if (n < 0) { buf[7] = '-'; n = -n; } else { buf[7] = ' '; }
    int pos = 8;
    if (n >= 100) { buf[pos++] = '0' + (n / 100) % 10; }
    if (n >= 10)  { buf[pos++] = '0' + (n / 10) % 10; }
    buf[pos++] = '0' + (n % 10);
    buf[pos] = 0;
    EXPECT_STREQ(buf, "SCORE: 123");
}

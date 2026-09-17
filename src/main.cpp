// 3D Snake game using raylib.
#include "raylib.h"
#include <deque>
#include <cstdlib>
#include <ctime>
#include <string>

namespace {

constexpr int kGridSize = 16;          // playable area is kGridSize x kGridSize cells
constexpr float kCellSize = 1.0f;
constexpr float kMoveIntervalSec = 0.15f;

enum class Direction { Up, Down, Left, Right };

struct Cell {
    int x;
    int z;
    bool operator==(const Cell& o) const { return x == o.x && z == o.z; }
};

std::deque<Cell> g_snake;
Direction g_direction = Direction::Right;
Direction g_pendingDirection = Direction::Right;
Cell g_food{0, 0};
bool g_gameOver = false;
int g_score = 0;
float g_moveTimer = 0.0f;

bool isOpposite(Direction a, Direction b) {
    return (a == Direction::Up && b == Direction::Down) ||
           (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) ||
           (a == Direction::Right && b == Direction::Left);
}

void placeFood() {
    while (true) {
        Cell candidate{GetRandomValue(0, kGridSize - 1), GetRandomValue(0, kGridSize - 1)};
        bool onSnake = false;
        for (const auto& seg : g_snake) {
            if (seg == candidate) { onSnake = true; break; }
        }
        if (!onSnake) {
            g_food = candidate;
            return;
        }
    }
}

void resetGame() {
    g_snake.clear();
    int mid = kGridSize / 2;
    g_snake.push_back({mid, mid});
    g_snake.push_back({mid - 1, mid});
    g_snake.push_back({mid - 2, mid});
    g_direction = Direction::Right;
    g_pendingDirection = Direction::Right;
    g_score = 0;
    g_gameOver = false;
    g_moveTimer = 0.0f;
    placeFood();
}

// Converts a grid cell to world-space coordinates centered on the board.
Vector3 cellToWorld(const Cell& cell, float y) {
    const float half = kGridSize * kCellSize / 2.0f;
    return {-half + (cell.x + 0.5f) * kCellSize, y,
             -half + (cell.z + 0.5f) * kCellSize};
}

void handleInput() {
    if (IsKeyPressed(KEY_UP))    g_pendingDirection = Direction::Up;
    if (IsKeyPressed(KEY_DOWN))  g_pendingDirection = Direction::Down;
    if (IsKeyPressed(KEY_LEFT))  g_pendingDirection = Direction::Left;
    if (IsKeyPressed(KEY_RIGHT)) g_pendingDirection = Direction::Right;
    if (IsKeyPressed(KEY_R))     resetGame();
}

void advanceSnake() {
    if (!isOpposite(g_pendingDirection, g_direction)) {
        g_direction = g_pendingDirection;
    }

    Cell head = g_snake.front();
    switch (g_direction) {
        case Direction::Up:    head.z -= 1; break;
        case Direction::Down:  head.z += 1; break;
        case Direction::Left:  head.x -= 1; break;
        case Direction::Right: head.x += 1; break;
    }

    if (head.x < 0 || head.x >= kGridSize || head.z < 0 || head.z >= kGridSize) {
        g_gameOver = true;
        return;
    }
    for (const auto& seg : g_snake) {
        if (seg == head) {
            g_gameOver = true;
            return;
        }
    }

    g_snake.push_front(head);
    if (head == g_food) {
        g_score += 10;
        placeFood();
    } else {
        g_snake.pop_back();
    }
}

void update(float dt) {
    handleInput();
    if (g_gameOver) return;

    g_moveTimer += dt;
    if (g_moveTimer >= kMoveIntervalSec) {
        g_moveTimer -= kMoveIntervalSec;
        advanceSnake();
    }
}

void draw(const Camera3D& camera) {
    BeginDrawing();
    ClearBackground(Color{12, 12, 20, 255});

    BeginMode3D(camera);
    DrawGrid(kGridSize, kCellSize);

    for (size_t i = 0; i < g_snake.size(); ++i) {
        Vector3 pos = cellToWorld(g_snake[i], kCellSize / 2.0f);
        Color color = (i == 0) ? Color{50, 230, 76, 255} : Color{38, 166, 51, 255};
        DrawCube(pos, kCellSize * 0.9f, kCellSize * 0.9f, kCellSize * 0.9f, color);
        DrawCubeWires(pos, kCellSize * 0.9f, kCellSize * 0.9f, kCellSize * 0.9f, Color{0, 0, 0, 90});
    }

    Vector3 foodPos = cellToWorld(g_food, kCellSize / 2.0f);
    DrawSphere(foodPos, kCellSize * 0.4f, Color{230, 51, 51, 255});

    EndMode3D();

    DrawText(TextFormat("Score: %d", g_score), 10, 10, 24, RAYWHITE);
    if (g_gameOver) {
        const char* msg = "GAME OVER - press R to restart";
        int width = MeasureText(msg, 28);
        DrawText(msg, GetScreenWidth() / 2 - width / 2, GetScreenHeight() / 2 - 14, 28, RAYWHITE);
    }

    EndDrawing();
}

}  // namespace

int main() {
    SetRandomSeed(static_cast<unsigned>(time(nullptr)));

    const int screenWidth = 900;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "3D Snake");
    SetTargetFPS(60);

    Camera3D camera{};
    camera.position = {0.0f, 18.0f, 16.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    resetGame();

    while (!WindowShouldClose()) {
        update(GetFrameTime());
        draw(camera);
    }

    CloseWindow();
    return 0;
}

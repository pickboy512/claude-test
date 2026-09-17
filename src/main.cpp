// 3D Snake game using raylib.
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <deque>
#include <cstdlib>
#include <ctime>
#include <cmath>

namespace {

constexpr int kGridSize = 16;          // playable area is kGridSize x kGridSize cells
constexpr float kCellSize = 1.0f;
constexpr float kMoveIntervalSec = 0.15f;
constexpr float kBoardHalf = kGridSize * kCellSize / 2.0f;

enum class Direction { Up, Down, Left, Right };

struct Cell {
    int x;
    int z;
    bool operator==(const Cell& o) const { return x == o.x && z == o.z; }
};

std::deque<Cell> g_snake;
std::deque<Cell> g_prevSnake;   // snake layout before the most recent grid step, for interpolation
Direction g_direction = Direction::Right;
Direction g_pendingDirection = Direction::Right;
Cell g_food{0, 0};
bool g_gameOver = false;
int g_score = 0;
float g_moveTimer = 0.0f;
Model g_floorModel{};

bool isOpposite(Direction a, Direction b) {
    return (a == Direction::Up && b == Direction::Down) ||
           (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) ||
           (a == Direction::Right && b == Direction::Left);
}

Vector3 directionVector(Direction d) {
    switch (d) {
        case Direction::Up:    return {0.0f, 0.0f, -1.0f};
        case Direction::Down:  return {0.0f, 0.0f, 1.0f};
        case Direction::Left:  return {-1.0f, 0.0f, 0.0f};
        case Direction::Right: return {1.0f, 0.0f, 0.0f};
    }
    return {1.0f, 0.0f, 0.0f};
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
    g_prevSnake = g_snake;
    g_direction = Direction::Right;
    g_pendingDirection = Direction::Right;
    g_score = 0;
    g_gameOver = false;
    g_moveTimer = 0.0f;
    placeFood();
}

// Converts a grid cell to world-space coordinates centered on the board.
Vector3 cellToWorld(const Cell& cell, float y) {
    return {-kBoardHalf + (cell.x + 0.5f) * kCellSize, y,
             -kBoardHalf + (cell.z + 0.5f) * kCellSize};
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
        g_prevSnake = g_snake;
        advanceSnake();
    }
}

// A world-space position for snake segment `index`, smoothly interpolated
// between its previous grid cell and its current one so movement glides
// instead of snapping once per tick.
Vector3 segmentRenderPos(size_t index, float alpha, float y) {
    Vector3 curr = cellToWorld(g_snake[index], y);
    if (index >= g_prevSnake.size()) return curr;  // freshly grown segment, no prior spot
    Vector3 prev = cellToWorld(g_prevSnake[index], y);
    return Vector3Lerp(prev, curr, alpha);
}

// Draws a box with a distinct brightness per face, faking directional
// lighting cheaply without a custom shader so shapes read as solid 3D
// forms instead of flat silhouettes.
void DrawCubeShaded(Vector3 position, float width, float height, float length, Color color) {
    float x = position.x;
    float y = position.y;
    float z = position.z;
    float w = width / 2.0f;
    float h = height / 2.0f;
    float l = length / 2.0f;

    auto tint = [&](float factor) {
        return Color{
            static_cast<unsigned char>(Clamp(color.r * factor, 0.0f, 255.0f)),
            static_cast<unsigned char>(Clamp(color.g * factor, 0.0f, 255.0f)),
            static_cast<unsigned char>(Clamp(color.b * factor, 0.0f, 255.0f)),
            color.a};
    };

    rlBegin(RL_TRIANGLES);

        Color c = tint(0.95f); rlColor4ub(c.r, c.g, c.b, c.a);  // front (+z)
        rlVertex3f(x - w, y - h, z + l); rlVertex3f(x + w, y - h, z + l); rlVertex3f(x - w, y + h, z + l);
        rlVertex3f(x + w, y - h, z + l); rlVertex3f(x + w, y + h, z + l); rlVertex3f(x - w, y + h, z + l);

        c = tint(0.6f); rlColor4ub(c.r, c.g, c.b, c.a);  // back (-z)
        rlVertex3f(x - w, y - h, z - l); rlVertex3f(x - w, y + h, z - l); rlVertex3f(x + w, y - h, z - l);
        rlVertex3f(x + w, y - h, z - l); rlVertex3f(x - w, y + h, z - l); rlVertex3f(x + w, y + h, z - l);

        c = tint(1.0f); rlColor4ub(c.r, c.g, c.b, c.a);  // top (+y)
        rlVertex3f(x - w, y + h, z - l); rlVertex3f(x - w, y + h, z + l); rlVertex3f(x + w, y + h, z + l);
        rlVertex3f(x - w, y + h, z - l); rlVertex3f(x + w, y + h, z + l); rlVertex3f(x + w, y + h, z - l);

        c = tint(0.45f); rlColor4ub(c.r, c.g, c.b, c.a);  // bottom (-y)
        rlVertex3f(x - w, y - h, z - l); rlVertex3f(x + w, y - h, z - l); rlVertex3f(x + w, y - h, z + l);
        rlVertex3f(x - w, y - h, z - l); rlVertex3f(x + w, y - h, z + l); rlVertex3f(x - w, y - h, z + l);

        c = tint(0.8f); rlColor4ub(c.r, c.g, c.b, c.a);  // right (+x)
        rlVertex3f(x + w, y - h, z - l); rlVertex3f(x + w, y + h, z - l); rlVertex3f(x + w, y + h, z + l);
        rlVertex3f(x + w, y - h, z - l); rlVertex3f(x + w, y + h, z + l); rlVertex3f(x + w, y - h, z + l);

        c = tint(0.7f); rlColor4ub(c.r, c.g, c.b, c.a);  // left (-x)
        rlVertex3f(x - w, y - h, z - l); rlVertex3f(x - w, y + h, z + l); rlVertex3f(x - w, y + h, z - l);
        rlVertex3f(x - w, y - h, z - l); rlVertex3f(x - w, y - h, z + l); rlVertex3f(x - w, y + h, z + l);

    rlEnd();
}

void drawGroundShadow(Vector3 worldPos, float radius) {
    DrawCylinder({worldPos.x, 0.012f, worldPos.z}, radius, radius, 0.02f, 16, Color{0, 0, 0, 80});
}

void drawBoundaryWalls() {
    constexpr float wallHeight = 1.0f;
    constexpr float wallThickness = 0.15f;
    const float span = kGridSize * kCellSize + wallThickness * 2.0f;
    Color wallColor{90, 170, 255, 65};

    DrawCube({0.0f, wallHeight / 2.0f, -kBoardHalf - wallThickness / 2.0f}, span, wallHeight, wallThickness, wallColor);
    DrawCube({0.0f, wallHeight / 2.0f, kBoardHalf + wallThickness / 2.0f}, span, wallHeight, wallThickness, wallColor);
    DrawCube({-kBoardHalf - wallThickness / 2.0f, wallHeight / 2.0f, 0.0f}, wallThickness, wallHeight, span, wallColor);
    DrawCube({kBoardHalf + wallThickness / 2.0f, wallHeight / 2.0f, 0.0f}, wallThickness, wallHeight, span, wallColor);
}

void drawBackgroundGradient() {
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(),
                            Color{18, 20, 34, 255}, Color{8, 9, 16, 255});
}

void drawSnakeAndFood(float alpha) {
    size_t count = g_snake.size();
    for (size_t i = 0; i < count; ++i) {
        // Taper the body from a wide head to a slim tail for a snake-like silhouette.
        float t = (count > 1) ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.0f;
        float size = Lerp(kCellSize * 0.9f, kCellSize * 0.55f, t);
        Vector3 pos = segmentRenderPos(i, alpha, size / 2.0f);
        Color color = (i == 0) ? Color{60, 235, 90, 255} : Color{35, 160, 55, 255};

        drawGroundShadow(pos, size * 0.55f);
        DrawCubeShaded(pos, size, size, size, color);

        if (i == 0) {
            Vector3 fwd = directionVector(g_direction);
            Vector3 right{-fwd.z, 0.0f, fwd.x};
            float eyeForward = size * 0.4f;
            float eyeSide = size * 0.28f;
            float eyeY = pos.y + size * 0.15f;
            Vector3 eye1 = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, eyeForward), Vector3Scale(right, eyeSide)));
            Vector3 eye2 = Vector3Add(pos, Vector3Add(Vector3Scale(fwd, eyeForward), Vector3Scale(right, -eyeSide)));
            eye1.y = eyeY;
            eye2.y = eyeY;
            DrawSphere(eye1, size * 0.09f, BLACK);
            DrawSphere(eye2, size * 0.09f, BLACK);
        }
    }

    float t = static_cast<float>(GetTime());
    float bob = sinf(t * 3.0f) * 0.12f;
    float pulse = 1.0f + sinf(t * 4.0f) * 0.06f;
    Vector3 foodPos = cellToWorld(g_food, kCellSize * 0.4f + bob);
    float foodRadius = kCellSize * 0.36f * pulse;

    drawGroundShadow({foodPos.x, 0.0f, foodPos.z}, kCellSize * 0.4f);
    DrawSphereEx(foodPos, foodRadius * 1.7f, 10, 10, Color{235, 70, 70, 45});
    DrawSphereEx(foodPos, foodRadius, 20, 20, Color{235, 60, 60, 255});
}

void draw(Camera3D& camera, float alpha) {
    BeginDrawing();
    drawBackgroundGradient();

    // Let the camera drift gently toward the (interpolated) snake head so the
    // view feels alive without ever snapping.
    Vector3 headPos = segmentRenderPos(0, alpha, 0.0f);
    Vector3 desiredTarget = Vector3Lerp({0.0f, 0.0f, 0.0f}, headPos, 0.35f);
    camera.target = Vector3Lerp(camera.target, desiredTarget, 0.08f);

    BeginMode3D(camera);
    DrawModel(g_floorModel, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

    drawSnakeAndFood(alpha);
    drawBoundaryWalls();

    EndMode3D();

    DrawText(TextFormat("Score: %d", g_score), 10, 10, 24, RAYWHITE);
    if (g_gameOver) {
        const char* msg = "GAME OVER - press R to restart";
        int width = MeasureText(msg, 28);
        DrawText(msg, GetScreenWidth() / 2 - width / 2, GetScreenHeight() / 2 - 14, 28, RAYWHITE);
    }

    EndDrawing();
}

void loadFloorModel() {
    Image checker = GenImageChecked(512, 512, kGridSize, kGridSize,
                                     Color{40, 44, 58, 255}, Color{52, 57, 74, 255});
    Texture2D texture = LoadTextureFromImage(checker);
    UnloadImage(checker);
    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);

    Mesh plane = GenMeshPlane(kGridSize * kCellSize, kGridSize * kCellSize, 1, 1);
    g_floorModel = LoadModelFromMesh(plane);
    g_floorModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
}

}  // namespace

int main() {
    SetRandomSeed(static_cast<unsigned>(time(nullptr)));

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    const int screenWidth = 900;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "3D Snake");
    SetTargetFPS(60);

    loadFloorModel();

    Camera3D camera{};
    camera.position = {0.0f, 18.0f, 16.0f};
    camera.target = {0.0f, 0.0f, 0.0f};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    resetGame();

    while (!WindowShouldClose()) {
        update(GetFrameTime());
        float alpha = Clamp(g_moveTimer / kMoveIntervalSec, 0.0f, 1.0f);
        draw(camera, alpha);
    }

    UnloadModel(g_floorModel);
    CloseWindow();
    return 0;
}

// 3D Snake game using OpenGL/GLUT.
#include <GL/freeglut.h>
#include <GL/glu.h>
#include <cmath>
#include <deque>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

namespace {

constexpr int kGridSize = 16;          // playable area is kGridSize x kGridSize cells
constexpr float kCellSize = 1.0f;
constexpr int kMoveIntervalMs = 150;

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

float g_camAngle = 35.0f;   // orbit angle around the board
float g_camHeight = 18.0f;
float g_camDistance = 22.0f;

void placeFood() {
    while (true) {
        Cell candidate{std::rand() % kGridSize, std::rand() % kGridSize};
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
    placeFood();
}

void drawCube(float x, float y, float z, float size, const float color[3]) {
    glPushMatrix();
    glTranslatef(x, y, z);
    GLfloat matDiffuse[] = {color[0], color[1], color[2], 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glutSolidCube(size);
    glPopMatrix();
}

void drawFloor() {
    const float half = kGridSize * kCellSize / 2.0f;
    GLfloat matDiffuse[] = {0.15f, 0.15f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-half, 0.0f, -half);
    glVertex3f(-half, 0.0f, half);
    glVertex3f(half, 0.0f, half);
    glVertex3f(half, 0.0f, -half);
    glEnd();

    // grid lines for readability
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_LINES);
    for (int i = 0; i <= kGridSize; ++i) {
        float offset = -half + i * kCellSize;
        glVertex3f(offset, 0.01f, -half);
        glVertex3f(offset, 0.01f, half);
        glVertex3f(-half, 0.01f, offset);
        glVertex3f(half, 0.01f, offset);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// Converts a grid cell to world-space coordinates centered on the board.
void cellToWorld(const Cell& cell, float& outX, float& outZ) {
    const float half = kGridSize * kCellSize / 2.0f;
    outX = -half + (cell.x + 0.5f) * kCellSize;
    outZ = -half + (cell.z + 0.5f) * kCellSize;
}

void updateCamera() {
    const float boardCenter = 0.0f;
    float rad = g_camAngle * 3.14159265f / 180.0f;
    float eyeX = boardCenter + g_camDistance * sinf(rad);
    float eyeZ = boardCenter + g_camDistance * cosf(rad);
    gluLookAt(eyeX, g_camHeight, eyeZ,
              boardCenter, 0.0f, boardCenter,
              0.0f, 1.0f, 0.0f);
}

void renderBitmapString(float x, float y, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}

void drawHud() {
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);

    std::ostringstream scoreText;
    scoreText << "Score: " << g_score;
    renderBitmapString(10.0f, height - 25.0f, scoreText.str());

    if (g_gameOver) {
        renderBitmapString(width / 2.0f - 70.0f, height / 2.0f,
                            "GAME OVER - press R to restart");
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void display() {
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    updateCamera();

    GLfloat lightPos[] = {0.0f, 25.0f, 0.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    drawFloor();

    const float headColor[3] = {0.2f, 0.9f, 0.3f};
    const float bodyColor[3] = {0.15f, 0.65f, 0.2f};
    const float foodColor[3] = {0.9f, 0.2f, 0.2f};

    for (size_t i = 0; i < g_snake.size(); ++i) {
        float wx, wz;
        cellToWorld(g_snake[i], wx, wz);
        drawCube(wx, kCellSize / 2.0f, wz, kCellSize * 0.9f,
                 i == 0 ? headColor : bodyColor);
    }

    float foodX, foodZ;
    cellToWorld(g_food, foodX, foodZ);
    glPushMatrix();
    glTranslatef(foodX, kCellSize / 2.0f, foodZ);
    GLfloat matDiffuse[] = {foodColor[0], foodColor[1], foodColor[2], 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiffuse);
    glutSolidSphere(kCellSize * 0.4, 20, 20);
    glPopMatrix();

    drawHud();

    glutSwapBuffers();
}

void reshape(int width, int height) {
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, static_cast<double>(width) / height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

bool isOpposite(Direction a, Direction b) {
    return (a == Direction::Up && b == Direction::Down) ||
           (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) ||
           (a == Direction::Right && b == Direction::Left);
}

void advanceSnake() {
    if (g_gameOver) return;

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

void timer(int /*value*/) {
    advanceSnake();
    glutPostRedisplay();
    glutTimerFunc(kMoveIntervalMs, timer, 0);
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 'r':
        case 'R':
            resetGame();
            break;
        case 27:  // Esc
            std::exit(0);
        default:
            break;
    }
}

void specialKeys(int key, int, int) {
    switch (key) {
        case GLUT_KEY_UP:    g_pendingDirection = Direction::Up;    break;
        case GLUT_KEY_DOWN:  g_pendingDirection = Direction::Down;  break;
        case GLUT_KEY_LEFT:  g_pendingDirection = Direction::Left;  break;
        case GLUT_KEY_RIGHT: g_pendingDirection = Direction::Right; break;
        default: break;
    }
}

void initGl() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    GLfloat ambient[] = {0.35f, 0.35f, 0.35f, 1.0f};
    GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}

}  // namespace

int main(int argc, char** argv) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(900, 700);
    glutCreateWindow("3D Snake");

    initGl();
    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(kMoveIntervalMs, timer, 0);

    glutMainLoop();
    return 0;
}

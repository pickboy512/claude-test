# claude-test

## 3D Snake

A simple 3D Snake game written in C++ with OpenGL/GLUT. The snake and food
are rendered as shaded cubes/sphere on a grid, viewed from a fixed
perspective camera.

### Build (Linux)

Install dependencies (Debian/Ubuntu):

```
sudo apt-get install build-essential cmake freeglut3-dev libglu1-mesa-dev
```

Build:

```
mkdir -p build && cd build
cmake ..
make
```

Run:

```
./snake3d
```

### Controls

- Arrow keys: change direction
- `R`: restart after game over
- `Esc`: quit

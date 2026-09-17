# claude-test

## 3D Snake

A simple 3D Snake game written in C++ with [raylib](https://www.raylib.com/).
The snake and food are rendered as cubes/a sphere on a grid, viewed from a
fixed perspective camera.

### Build (macOS, incl. Apple Silicon)

Install dependencies:

```
brew install cmake raylib
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

### Build (Linux)

Install raylib per the [raylib wiki](https://github.com/raysan5/raylib/wiki)
for your distro (or build it from source), then build the same way as above:

```
mkdir -p build && cd build
cmake ..
make
./snake3d
```

### Controls

- Arrow keys: change direction
- `R`: restart after game over
- `Esc`: quit

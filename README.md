# claude-test

## Yasuo Practice Tool

A 2D top-down League of Legends-style practice tool written in C++ with
[raylib](https://www.raylib.com/). You play a fixed champion, Yasuo, and can
practice two things at once:

- **Last-hitting**: a lane of allied and enemy minions spawns in waves and
  fights itself automatically; right-click an enemy minion to attack it, and
  you're credited with CS/gold only if your own hit lands the kill.
- **Combos**: a practice dummy with regenerating HP sits off to the side for
  drilling ability combos. Its last combo's total damage is shown once it
  fully regenerates.

Abilities follow the shape of Yasuo's real kit:

- **Passive - Way of the Wanderer**: moving fills a Flow gauge; at full Flow
  your next hit taken is shielded.
- **Q - Steel Tempest**: a skillshot. Landing three casts within a few
  seconds knocks the target airborne instead of just damaging it.
- **W - Wind Wall**: a defensive cooldown that shields you (no damage, like
  the real spell).
- **E - Sweeping Blade**: a dash that damages what it passes through; killing
  something with it fully refunds the cooldown.
- **R - Last Breath**: only does something if an airborne enemy is in range —
  dashes to it and deals AoE damage around the impact point.

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
./practice_tool
```

### Build (Linux)

Install raylib per the [raylib wiki](https://github.com/raysan5/raylib/wiki)
for your distro (or build it from source), then build the same way as above:

```
mkdir -p build && cd build
cmake ..
make
./practice_tool
```

### Controls

- Right mouse button: move, or attack-move onto a minion/dummy under the cursor
- `Q` / `W` / `E` / `R`: cast abilities toward the cursor
- `S`: stop (clear current move/attack order)
- `Backspace`: reset the practice session (CS, gold, dummy, minions, cooldowns)
- `Esc`: quit

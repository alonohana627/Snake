# Game rules (`game`)

`snake::game` holds the rules and the state, and nothing else: no windows, keys,
pixels, colors, sounds or time. It's told to **turn** and to **tick**; everything
else reads its state. Files: [grid.hpp](../include/snake/game/grid.hpp),
[snake.hpp](../include/snake/game/snake.hpp), [game.hpp](../include/snake/game/game.hpp).

## The board (`grid.hpp`)

- 20x20 cells (`kBoardWidth`, `kBoardHeight`). `Cell{x, y}`, (0, 0) is the top-left,
  y grows downward - the same as on screen.
- `Direction` is Up, Down, Left, Right. `neighbor(cell, direction)` is the next
  cell that way; `opposite(direction)` the reverse.
- `wrap(cell, w, h)` brings a cell that left the board back on the other side.
  (C++'s `%` keeps the sign, so `-1 % 20` is `-1`; the helper adds the size once
  more to get 19.) `isInside(cell, w, h)` checks the edges.

## The snake (`Snake`)

A chain of cells, head first, stored in a `std::deque<Cell>` (cheap to add at the
front and drop at the back - exactly what moving does).

**Moving** (`stepTo(newHead)`): push the new head at the front, drop the tail at
the back - unless the snake is growing, then the tail stays for that step.
`grow(n)` adds `n` such steps.

The snake knows nothing about walls or the board: `nextHead()` says where the head
*would* go, and the game decides where it actually goes (wrapped, or a crash).

### Turning

A plain "current direction" loses fast key presses: pressing Down then Left within
one tick would keep only Left. So turns are **queued**:

- `turn(d)` appends `d` to a queue of up to 3 turns (`kMaxQueuedTurns`).
- Each step uses (and removes) the first queued turn.
- A turn is ignored if it's the same as, or the opposite of, the direction the
  snake will have when that turn is used (the last queued turn, or the current
  direction). So heading Right, `Up, Left` is a tight U-turn over two steps, but
  `Left` alone (straight back into itself) is ignored.

`direction()` is the direction of the last step - what the snake visibly does (the
head sprite faces this way).

## The game (`Game`)

### A tick

```
next = snake.nextHead()
if walls are Solid and next is off the board:  crash
snake.stepTo(wrap(next))                         (Wrap: back in on the other side)
if the head is on the body:                      crash
event Moved
if the head is on the food:
    snake grows by growthOf(type), score += pointsOf(type)
    best = max(best, score), event Ate(type), new food
```

**Crash** = `startOver()` + event `Crashed`: a fresh length-5 snake in the middle
(10, 10) heading right, score 0, new food. The best score is kept. There's no
"game over" screen: the round restarts at once.

### Food

Three types, picked with weights 6 : 3 : 1 (`kFoodWeights` in game.cpp):

| Type | Chance | Grows by (`growthOf`) | Points (`pointsOf`) |
|---|---|---|---|
| Small | 60% | 1 | 1 |
| Medium | 30% | 2 | 2 |
| Large | 10% | 3 | 3 |

**Where it appears** (`spawnFood`): a random free cell, every free cell equally
likely - but preferably at least 3 moves from the head (`kFoodMinDistance`,
counting wrap-around), so it never pops up right in front of the snake. If no cell
is that far, any free cell; if the snake fills the board, no food.

**Randomness** comes from a `std::mt19937` seeded in the constructor. The same seed
gives the same game (handy for tests); the app passes a random seed.

### Walls

`setWalls(Walls::Wrap | Walls::Solid)`, from the WALLS setting. Takes effect on the
next tick. With Solid, the check happens *before* moving, so the snake never leaves
the board.

### Score

- `score()`: points eaten since the last start over.
- `bestScore()`: the highest score so far. `setBestScore()` carries one over from
  an earlier run (loaded from `snake.ini`).

### Events

`tick()` records what happened in `m_events`; `takeEvents()` hands them over and
clears the list:

| Event | When |
|---|---|
| `Moved` | Every tick the snake moved |
| `Ate` (+ food type) | It ate; comes after that tick's `Moved` |
| `Crashed` | It hit itself or a solid wall. Alone: no `Moved` that tick |

The game doesn't know who listens. Today `app` uses them for sounds and to decide
when to save.

## Testing it

The game needs nothing but the standard library, so a test is plain C++:

```cpp
#include "snake/game/game.hpp"
using namespace snake::game;

Game game(1);                      // fixed seed: same food every run
game.setWalls(Walls::Solid);
for (int i = 0; i < 9; ++i) game.tick();   // head (10,10) -> (19,10)
game.tick();                                // into the right wall
// game.takeEvents().back().type == EventType::Crashed, score() == 0
```

Link it against `snake::game` only (see [build.md](build.md#tests)).

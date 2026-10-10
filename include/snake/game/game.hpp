#pragma once

#include "snake/game/grid.hpp"
#include "snake/game/snake.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace snake::game {

// The board size, in cells.
inline constexpr int kBoardWidth = 20;
inline constexpr int kBoardHeight = 20;

// The kinds of food. Bigger food is rarer, worth more, and makes the snake longer.
enum class FoodType {
    Small,
    Medium,
    Large,
};

// How many cells the snake grows when it eats this food.
constexpr int growthOf(FoodType type)
{
    switch (type) {
    case FoodType::Small:  return 1;
    case FoodType::Medium: return 2;
    case FoodType::Large:  return 3;
    }
    return 1;
}

// How many points this food is worth.
constexpr int pointsOf(FoodType type)
{
    switch (type) {
    case FoodType::Small:  return 1;
    case FoodType::Medium: return 2;
    case FoodType::Large:  return 3;
    }
    return 1;
}

struct Food {
    Cell cell;
    FoodType type = FoodType::Small;
};

// What happens at the board's edges.
enum class Walls {
    Wrap,  // leaving one side brings the snake back on the other
    Solid, // running into an edge is a crash
};

// Something that happened during a tick. The game only reports it; whoever shows
// the game decides what it looks or sounds like (e.g. a sound when food is eaten).
enum class EventType {
    Moved,   // the snake moved one cell (every tick, unless it crashed)
    Ate,     // it ate food
    Crashed, // it ran into itself (or a solid wall), and the game started over
};

struct Event {
    EventType type = EventType::Moved;
    FoodType food = FoodType::Small; // Ate only: what was eaten
};

// The game's rules and state - nothing else. It knows nothing about windows,
// keys, pixels, colors or time: it's told to turn and to tick, and others read
// its state to show it.
//
// Rules:
// - The snake moves one cell per tick. At the board's edges it wraps around, or,
//   with solid walls, crashes.
// - It must not run into itself (or a solid wall). If it does, it starts over from the start.
// - There's always one piece of food on a free cell (unless the board is full).
//   Small is the most common, Large the rarest (see kFoodWeights in game.cpp).
//   Eating it makes the snake growthOf(type) cells longer, adds pointsOf(type)
//   to the score, and new food appears.
// - Score = points from the food eaten since the last start over.
class Game {
public:
    // A length-5 snake in the middle of the board, heading right, and food.
    // `seed` drives where food appears and which type: the same seed gives the same game.
    explicit Game(std::uint32_t seed);

    // Ask the snake to turn. It happens on an upcoming tick.
    void turn(Direction direction);

    // Advance by exactly one tick: the snake moves one cell. If it ran into
    // itself, the game starts over; if it reached the food, it eats it.
    // How often to tick is up to the caller (it sets the snake's speed).
    void tick();

    // Back to the start: a fresh snake, a zero score and new food. The best score is kept.
    void startOver();

    // Wrap (the default) or Solid. Takes effect from the next tick.
    void setWalls(Walls walls) { m_walls = walls; }
    Walls walls() const { return m_walls; }

    const Snake& snake() const { return m_snake; }

    // The food on the board. Empty only when the snake fills the whole board.
    std::optional<Food> food() const { return m_food; }

    // Points from the food eaten since the last start over.
    int score() const { return m_score; }

    // The highest score so far (including earlier runs, if setBestScore was given one).
    int bestScore() const { return m_bestScore; }

    // Carry over the best score from an earlier run (e.g. loaded from a file).
    void setBestScore(int best) { m_bestScore = std::max(best, m_score); }

    // What happened in the ticks since the last call, oldest first. Reading them clears them.
    std::vector<Event> takeEvents();

private:
    void spawnFood();

    std::mt19937 m_random; // random number generator for food positions and types
    Snake m_snake;
    std::optional<Food> m_food;
    int m_score = 0;
    int m_bestScore = 0;
    Walls m_walls = Walls::Wrap;
    std::vector<Event> m_events;
};

} // namespace snake::game

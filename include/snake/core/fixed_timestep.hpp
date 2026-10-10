#pragma once

namespace snake::core {

// Turns real time into a whole number of fixed-length steps, so logic that runs
// once per step behaves the same at any frame rate. Call advance() once per frame.
class FixedTimestep {
public:
    // stepSeconds: the length of one step.
    // maxFrameSeconds: a longer frame (e.g. the loop was stalled) only counts as this
    // long, so we don't try to catch up on hundreds of steps at once.
    FixedTimestep(double stepSeconds, double maxFrameSeconds);

    // Start (or restart) measuring from `now`. Time before it never turns into steps.
    void reset(double now);

    // Adds the time since the last call and returns how many steps are due:
    // 0, 1 or several, depending on frame rate vs step rate. Leftover time
    // (less than one step) carries over to the next call.
    int advance(double now);

private:
    double m_stepSeconds;
    double m_maxFrameSeconds;
    double m_previousTime = 0.0;
    double m_unsimulatedTime = 0.0; // real time that hasn't been turned into steps yet
};

} // namespace snake::core

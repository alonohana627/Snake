#include "snake/core/fixed_timestep.hpp"

#include <algorithm>

namespace snake::core {

FixedTimestep::FixedTimestep(double stepSeconds, double maxFrameSeconds)
    : m_stepSeconds(stepSeconds)
    , m_maxFrameSeconds(maxFrameSeconds)
{
}

void FixedTimestep::reset(double now)
{
    m_previousTime = now;
}

int FixedTimestep::advance(double now)
{
    m_unsimulatedTime += std::min(now - m_previousTime, m_maxFrameSeconds);
    m_previousTime = now;

    int steps = 0;
    while (m_unsimulatedTime >= m_stepSeconds) {
        m_unsimulatedTime -= m_stepSeconds;
        ++steps;
    }
    return steps;
}

} // namespace snake::core

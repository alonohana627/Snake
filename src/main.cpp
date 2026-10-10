#include "snake/app/application.hpp"

#include <cstdio>
#include <exception>
#include <print>

int main()
{
    // Print immediately, even when stdout is a pipe (some terminals) rather than a console.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    try {
        snake::app::Application app;
        app.run();
    } catch (const std::exception& error) {
        std::println(stderr, "Error: {}", error.what());
        return 1;
    }

    return 0;
}

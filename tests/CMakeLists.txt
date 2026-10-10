# Test suite entry point. Enabled with -DSNAKE_BUILD_TESTS=ON
# (or the "*-tests" presets / the --tests flag of the build scripts).
#
# No tests yet. To add one, link just the component under test - e.g. the game
# needs no window or GPU:
#   add_executable(snake_tests game_test.cpp)
#   snake_set_warnings(snake_tests)
#   target_link_libraries(snake_tests PRIVATE snake::game)
#   add_test(NAME snake_tests COMMAND snake_tests)

# Third-party dependencies (git submodules).

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/glfw/CMakeLists.txt)
    message(FATAL_ERROR "deps/glfw is empty. Run: git submodule update --init")
endif()

# GLFW: window + input. We only want the library, not its docs or install rules.
# (Its examples and tests are already off when it's built as part of another project.)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

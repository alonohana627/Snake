# Third-party dependencies (git submodules).

foreach(dep IN ITEMS glfw/CMakeLists.txt miniaudio/miniaudio.c)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dep})
        message(FATAL_ERROR "deps/ is missing ${dep}. Run: git submodule update --init")
    endif()
endforeach()

# GLFW: window + input. We only want the library, not its docs or install rules.
# (Its examples and tests are already off when it's built as part of another project.)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(glfw)

# miniaudio: sound output. It's one .c file (plus its header), so we build it
# ourselves instead of using its CMakeLists, which also builds Vorbis/Opus support.
# We play sounds we generate in code, so no file decoding/encoding/resource manager.
# The MA_NO_* flags are PUBLIC: they change miniaudio's structs, so our code must
# see the same ones when it includes miniaudio.h.
add_library(miniaudio STATIC miniaudio/miniaudio.c)
target_include_directories(miniaudio SYSTEM PUBLIC miniaudio)
target_compile_definitions(miniaudio PUBLIC
    MA_NO_DECODING
    MA_NO_ENCODING
    MA_NO_GENERATION
    MA_NO_RESOURCE_MANAGER
)
find_package(Threads REQUIRED)
target_link_libraries(miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX)
    target_link_libraries(miniaudio PUBLIC m)
endif()

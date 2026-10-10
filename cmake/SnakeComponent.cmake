# snake_add_component(<name> <sources>...)
#
# Builds one component (src/<name>/) as a static library "snake_<name>", usable
# as "snake::<name>". Its headers live in include/snake/<name>/.
# What a component may use is whatever it links: e.g. game links only snake::core,
# so it can't reach GLFW or Vulkan.
function(snake_add_component name)
    add_library(snake_${name} STATIC ${ARGN})
    add_library(snake::${name} ALIAS snake_${name})
    target_include_directories(snake_${name} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    snake_set_warnings(snake_${name})
endfunction()

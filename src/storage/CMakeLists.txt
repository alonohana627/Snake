# Files on disk: where the user's files go, reading and safely writing them, and a
# small INI format. Knows nothing about the game - app decides what to store.
snake_add_component(storage
    files.cpp
    ini.cpp
)

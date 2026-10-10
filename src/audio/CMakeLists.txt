# Sound: an 8-bit style synthesizer (notes -> samples) and a player that sends
# samples to the speakers. miniaudio stays private: nothing else sees it.
snake_add_component(audio
    player.cpp
    synth.cpp
)
target_link_libraries(snake_audio
    PRIVATE miniaudio
)

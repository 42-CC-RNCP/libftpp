add_executable(perlin_noise_visualization
    perlin_noise_visualization.cpp
)

target_include_directories(perlin_noise_visualization
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

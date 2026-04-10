cmake_minimum_required(VERSION 3.20)
project(Testing)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(OpenGL_Engine)
add_executable(app
    Main.cpp
    ShadowLabUI.cpp
)
target_link_libraries(app PRIVATE engine)

target_compile_definitions(app PRIVATE
    ENGINE_ASSET_ROOT="${CMAKE_SOURCE_DIR}"
)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders
        $<TARGET_FILE_DIR:app>/Shaders

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Environment
        $<TARGET_FILE_DIR:app>/Environment

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Textures
        $<TARGET_FILE_DIR:app>/Textures

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Models
        $<TARGET_FILE_DIR:app>/Models
    
)

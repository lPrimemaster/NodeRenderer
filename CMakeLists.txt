cmake_minimum_required (VERSION 3.8)
project(CBG CXX)
set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(glfw)

# Stop being nasty
file(GLOB IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/imgui/*.h
)
file(GLOB WINDOWS_SRC
    ${CMAKE_SOURCE_DIR}/src/windows/*.cpp
    ${CMAKE_SOURCE_DIR}/src/windows/*.h
    ${CMAKE_SOURCE_DIR}/src/windows/*.inl
)

add_subdirectory(muparser)

add_executable(cbg64
    glad/src/glad.c
    # glad/include/glad.h

    ${IMGUI_SRC}
    ${WINDOWS_SRC}

    src/log/logger.h
    src/log/logger.cpp

    src/render/renderer.h
    src/render/renderer.cpp

    src/math/vector.h
    src/math/vector.cpp

    src/main.cpp
)

target_include_directories(cbg64 PRIVATE glad/include glfw/include)
target_link_libraries(cbg64 glfw ${GLFW_LIBRARIES} muparser)

add_custom_target(copy-runtime-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/shader
    DEPENDS cbg64
)

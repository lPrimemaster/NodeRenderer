cmake_minimum_required (VERSION 3.8)
project(CBG CXX)
set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(glfw)

find_package (Python REQUIRED COMPONENTS Interpreter Development)

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
file(GLOB NODES_SRC
    ${CMAKE_SOURCE_DIR}/render/nodes/*.h
)

add_subdirectory(muparser)

add_subdirectory(tinyobjloader)

add_subdirectory(bvh)

add_executable(cbg64
    glad/src/glad.c
    # glad/include/glad.h

    ${IMGUI_SRC}
    ${WINDOWS_SRC}
    ${NODES_SRC}

    src/log/logger.h
    src/log/logger.cpp

    src/render/renderer.h
    src/render/renderer.cpp

    src/math/vector.h
    src/math/vector.cpp

    src/util/objloader.h
    src/util/objloader.cpp

    src/util/audio.h
    src/util/audio.cpp

    src/python/loader.h
    src/python/loader.cpp

    src/util/base64.h
    src/util/base64.cpp

    src/util/raycaster/bvh.h
    src/util/raycaster/bvh.cpp

    src/main.cpp
)

target_include_directories(cbg64 PRIVATE glad/include glfw/include ${Python_INCLUDE_DIRS})
target_link_libraries(cbg64 glfw ${GLFW_LIBRARIES} muparser tinyobjloader bvh ${Python_LIBRARIES} Winmm.lib)

add_custom_target(copy-runtime-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/shader
    DEPENDS cbg64
)

add_custom_target(copy-runtime-files-py ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/python/scripts ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/scripts
    DEPENDS cbg64
)

add_custom_target(copy-runtime-files-res ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/res
    DEPENDS cbg64
)

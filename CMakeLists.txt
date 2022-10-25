cmake_minimum_required (VERSION 3.8)
project(NodeRenderer VERSION 1.0.3 LANGUAGES CXX)
set(NodeRenderer_VERSION_SUFFIX "alpha")

message("Setting version: ${CMAKE_PROJECT_VERSION}-${NodeRenderer_VERSION_SUFFIX}")
configure_file(src/config/version.h.in version.h)

set(CMAKE_CXX_STANDARD 20)

option(INSTALLER_BUILD "Set if this is a shipping build." OFF)
option(DEV_BUILD "Set if this is a development build." ON)
# TODO: Make this set the console on or off

if(DEV_BUILD)
    add_definitions(-DDEV_BUILD)
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_subdirectory(glfw EXCLUDE_FROM_ALL)
add_subdirectory(muparser EXCLUDE_FROM_ALL)
add_subdirectory(tinyobjloader EXCLUDE_FROM_ALL)
add_subdirectory(bvh EXCLUDE_FROM_ALL)
add_subdirectory(glad EXCLUDE_FROM_ALL)

set(KISSFFT_PKGCONFIG OFF CACHE BOOL "Build pkg-config files" FORCE)
set(KISSFFT_TEST OFF CACHE BOOL "Build kissfft tests" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "Build kissfft command-line tools" FORCE)
add_subdirectory(kissfft EXCLUDE_FROM_ALL)

# Stop being nasty
file(GLOB IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/imgui/*.h
)
file(GLOB WINDOWS_SRC
    ${CMAKE_SOURCE_DIR}/src/windows/*.cpp
    ${CMAKE_SOURCE_DIR}/src/windows/*.h
    ${CMAKE_SOURCE_DIR}/src/windows/*.inl
)

set(IMPLOT_SRC
    ${CMAKE_SOURCE_DIR}/implot/implot.h
    ${CMAKE_SOURCE_DIR}/implot/implot_internal.h
    ${CMAKE_SOURCE_DIR}/implot/implot.cpp
    ${CMAKE_SOURCE_DIR}/implot/implot_items.cpp
)

# EXE Icon
set(winrc res/image/resources.rc)
set_property(SOURCE winrc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/res/image/nr.ico)
set_property(SOURCE winrc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/res/image/newfile.png)
set_property(SOURCE winrc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/res/image/loadfile.png)
set_property(SOURCE winrc APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/res/image/savefile.png)

add_executable(nr64
    ${winrc}
    ${IMGUI_SRC}
    ${IMPLOT_SRC}
    ${WINDOWS_SRC}
    ${NODES_SRC}

    src/log/logger.h
    src/log/logger.cpp

    src/render/renderer.h
    src/render/renderer.cpp

    src/render/raymarch_renderer.h
    src/render/raymarch_renderer.cpp
    src/render/raster_renderer.h
    src/render/raster_renderer.cpp

    src/math/vector.h
    src/math/vector.cpp

    src/util/updateclient.h
    src/util/updateclient.cpp

    src/util/objloader.h
    src/util/objloader.cpp

    src/util/audio.h
    src/util/audio.cpp

    src/util/base64.h
    src/util/base64.cpp

    src/util/raycaster/bvh.h
    src/util/raycaster/bvh.cpp

    src/util/misc.inl

    src/main.cpp
)

if(INSTALLER_BUILD)
    set_target_properties(nr64 PROPERTIES LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" /SUBSYSTEM:CONSOLE")
endif()


target_include_directories(nr64 PRIVATE glad/include glfw/include imgui ${Python_INCLUDE_DIRS} glad ${PROJECT_BINARY_DIR})
target_link_libraries(nr64 glad glfw ${GLFW_LIBRARIES} muparser tinyobjloader bvh ${Python_LIBRARIES} Winmm.lib WinHttp.lib kissfft) #msvcrt.lib)

add_custom_target(copy-runtime-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/shader
    DEPENDS nr64
)

add_custom_target(copy-runtime-files-res ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/res
    DEPENDS nr64
)

# Create an automated installer with CPack


# set(CMAKE_INSTALL_PREFIX "C:/Users/cesar/OneDrive/Desktop/NodeRenderer") # Debug only
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${IRUNTIME})
# cmake_path(GET CMAKE_RUNTIME_OUTPUT_DIRECTORY PARENT_PATH MY_PARENT_DIR)
# message(${MY_PARENT_DIR})
install(TARGETS nr64 RUNTIME DESTINATION . COMPONENT Core)

# Being nasty again...
file(GLOB SHADERS_DIR
    ${CMAKE_SOURCE_DIR}/src/shader/*.vs
    ${CMAKE_SOURCE_DIR}/src/shader/*.fs
)

file(GLOB EXAMPLES_DIR
    ${CMAKE_SOURCE_DIR}/examples/*.b64
)

file(GLOB DEPS_DIR
    ${CMAKE_SOURCE_DIR}/bin/Release/kissfft-float.dll
)

install(FILES ${SHADERS_DIR}  DESTINATION shader         COMPONENT Core)
install(FILES ${DEPS_DIR}     DESTINATION .              COMPONENT Core)
install(FILES ${EXAMPLES_DIR} DESTINATION saves/examples COMPONENT Examples)


set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_NAME "NodeRenderer")
set(CPACK_PACKAGE_VENDOR "Primemaster")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "NodeRenderer - Render anything visually.")
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Installer)
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/res/image\\\\nr.ico")
set(CPACK_NSIS_MUI_ICON ${CMAKE_SOURCE_DIR}/res/image/nr.ico)
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}-${NodeRenderer_VERSION_SUFFIX}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY NodeRenderer)
set(CPACK_NSIS_MODIFY_PATH OFF)
set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)


set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
CreateShortCut \\\"$DESKTOP\\\\Node Renderer.lnk\\\" \\\"$INSTDIR\\\\nr64.exe\\\"
")

set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
Delete \\\"$DESKTOP\\\\Node Renderer.lnk\\\"
")

include(CPack)

cpack_add_component(Core
                    DISPLAY_NAME Core
                    DESCRIPTION "The main application. This is required to run the renderer with all the basic functionalities."
                    REQUIRED)

cpack_add_component(Examples
                    DISPLAY_NAME Examples
                    DESCRIPTION "Installs example saves. Recommended for new users."
                    DEPENDS Core)
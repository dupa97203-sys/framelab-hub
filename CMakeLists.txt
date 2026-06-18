cmake_minimum_required(VERSION 3.15)
project(framelab_hub CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pobranie biblioteki GLFW ze źródła (zip jest szybszy niż klonowanie git)
include(FetchContent)
FetchContent_Declare(
  glfw
  URL https://github.com/glfw/glfw/archive/refs/tags/3.3.10.zip
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Ścieżka do zaimportowanych plików ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Główna aplikacja
add_executable(framelab_hub main.cpp ${IMGUI_SOURCES})

target_include_directories(framelab_hub PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Linkowanie z GLFW i OpenGL
target_link_libraries(framelab_hub PRIVATE glfw)

if(UNIX AND NOT APPLE)
    # Dla Linuksa - standardowe X11 i OpenGL
    target_link_libraries(framelab_hub PRIVATE GL dl X11 Xrandr Xinerama Xcursor Xi pthread)
elseif(APPLE)
    # Dla macOS - Frameworki systemowe Cocoa i OpenGL
    target_link_libraries(framelab_hub PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(WIN32)
    # Dla Windows - OpenGL32 i DWM
    target_link_libraries(framelab_hub PRIVATE opengl32 dwmapi)
endif()

cmake_minimum_required(VERSION 3.20)
project(LearnVulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# MSVC 默认按系统 ANSI 代码页（中文 Windows 上是 GBK）读取源文件，
# 会把 UTF-8 的中文注释读成乱码，甚至因为某个字节恰好是 0x5C('\') 而报奇怪的语法错误。
# /utf-8 同时指定「源文件字符集」和「执行字符集」为 UTF-8，彻底避免这个问题。
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---------------------------------------------------------------------------
# Vulkan SDK
# ---------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

# 着色器编译规则（内含 glslc 查找）
include(cmake/Shaders.cmake)

message(STATUS "Vulkan include : ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan library : ${Vulkan_LIBRARIES}")
message(STATUS "glslc          : ${GLSLC_EXECUTABLE}")

# ---------------------------------------------------------------------------
# 第三方库：GLFW（窗口）+ GLM（线性代数），首次 configure 时自动下载
# ---------------------------------------------------------------------------
include(FetchContent)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11      OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(glfw glm)

# ---------------------------------------------------------------------------
# 本体
# ---------------------------------------------------------------------------
add_executable(learn_vulkan src/main.cpp)

if(TARGET Vulkan::Vulkan)
    target_link_libraries(learn_vulkan PRIVATE Vulkan::Vulkan)
else()
    target_include_directories(learn_vulkan PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(learn_vulkan PRIVATE ${Vulkan_LIBRARIES})
endif()

target_link_libraries(learn_vulkan PRIVATE glfw glm::glm)

# 着色器产物目录，运行时用 SHADER_DIR 定位 .spv，避免受工作目录影响
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_definitions(learn_vulkan PRIVATE SHADER_DIR="${SHADER_OUTPUT_DIR}")

# 当前需要的着色器（后续章节逐个加进来）
add_shaders(learn_vulkan
    # shader.vert
    # shader.frag
)

# 调试时的工作目录 = 可执行文件所在目录
set_target_properties(learn_vulkan PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:learn_vulkan>")

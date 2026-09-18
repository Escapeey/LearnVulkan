cmake_minimum_required(VERSION 3.20)
project(LearnVulkan LANGUAGES CXX)

# CMake 4.x 移除了对 cmake_minimum_required(VERSION < 3.5) 的兼容，
# 而 GLM 1.0.1 这类较老的依赖仍声明旧版本号，直接 add_subdirectory 会硬报错。
# 这个变量让 CMake 4.x 放行它们；CMake 3.x 不认识它，当普通缓存变量忽略，无副作用。
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING
    "Allow dependencies declaring an old cmake_minimum_required (needed on CMake 4.x)")

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
# 第三方库：GLFW（窗口）+ GLM（线性代数）
#
# 依赖获取按优先级自动选择（详见 third_party/README.md）：
#   1) -DGLFW_LOCAL_DIR=<路径>     显式指定本地源码目录
#   2) third_party/glfw 已存在      用本地源码
#   3) 从 GLFW_GIT_URL 拉取         默认 GitHub，国内可换镜像
#
# GitHub 拉不动时的两种救法（都不用改本文件）：
#   配代理:  git config --global http.proxy http://127.0.0.1:7890
#   换镜像:  cmake -S . -B build -DGLFW_GIT_URL=<镜像地址> -DGLM_GIT_URL=<镜像地址>
# ---------------------------------------------------------------------------
include(FetchContent)

# 默认用 Gitee 官方镜像（"Gitee 极速下载"，mirrors 组，会跟上游同步）。
#
# 原因：实测这台机器上 github.com:443 连不上（TCP 层 21 秒超时），而 gitee.com 正常。
# 想换回 GitHub 或别的源，用 -DGLFW_GIT_URL=... / -DGLM_GIT_URL=... 覆盖即可。
set(GLFW_GIT_URL "https://gitee.com/mirrors/glfw.git" CACHE STRING "GLFW 仓库地址")
set(GLFW_GIT_TAG "3.4"                                CACHE STRING "GLFW 版本")
set(GLM_GIT_URL  "https://gitee.com/mirrors/glm.git"  CACHE STRING "GLM 仓库地址")
set(GLM_GIT_TAG  "1.0.1"                              CACHE STRING "GLM 版本")

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

function(acquire_dependency NAME GIT_URL GIT_TAG)
    set(local_var "${NAME}_LOCAL_DIR")
    set(local_dir "")

    if(DEFINED ${local_var} AND NOT "${${local_var}}" STREQUAL "")
        set(local_dir "${${local_var}}")
    elseif(EXISTS "${THIRD_PARTY_DIR}/${NAME}/CMakeLists.txt")
        set(local_dir "${THIRD_PARTY_DIR}/${NAME}")
    endif()

    if(NOT "${local_dir}" STREQUAL "")
        message(STATUS "依赖 ${NAME}: 使用本地源码 ${local_dir}")
        add_subdirectory("${local_dir}" "${CMAKE_BINARY_DIR}/_deps/${NAME}-build")
    else()
        message(STATUS "依赖 ${NAME}: 从 ${GIT_URL} 拉取 (tag ${GIT_TAG})")
        FetchContent_Declare(${NAME}
            GIT_REPOSITORY "${GIT_URL}"
            GIT_TAG        "${GIT_TAG}"
            GIT_SHALLOW    TRUE)
        FetchContent_MakeAvailable(${NAME})
    endif()
endfunction()

# GLFW 的构建选项必须在它被加入之前设为 CACHE，否则会被 GLFW 自己的 option() 覆盖
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11      OFF CACHE BOOL "" FORCE)

acquire_dependency(glfw "${GLFW_GIT_URL}" "${GLFW_GIT_TAG}")
acquire_dependency(glm  "${GLM_GIT_URL}"  "${GLM_GIT_TAG}")

# ---------------------------------------------------------------------------
# 目标
#
#   env_check      D1 环境自检工具（独立于教程，永久保留）
#   learn_vulkan   教程主程序，跟着章节一路长大
# ---------------------------------------------------------------------------

# 两个目标共用的链接与调试设置
function(configure_vulkan_target TARGET)
    if(TARGET Vulkan::Vulkan)
        target_link_libraries(${TARGET} PRIVATE Vulkan::Vulkan)
    else()
        target_include_directories(${TARGET} PRIVATE ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(${TARGET} PRIVATE ${Vulkan_LIBRARIES})
    endif()

    target_link_libraries(${TARGET} PRIVATE glfw glm::glm)

    # 调试时的工作目录 = 可执行文件所在目录
    set_target_properties(${TARGET} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET}>")
endfunction()

# --- D1 环境自检 -----------------------------------------------------------
add_executable(env_check src/env_check.cpp)
configure_vulkan_target(env_check)

# --- 教程主程序 ------------------------------------------------------------
add_executable(learn_vulkan src/hello_triangle.cpp)
configure_vulkan_target(learn_vulkan)

# 着色器产物目录，运行时用 SHADER_DIR 定位 .spv，避免受工作目录影响
set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_definitions(learn_vulkan PRIVATE SHADER_DIR="${SHADER_OUTPUT_DIR}")

# 着色器：GLSL -> SPIR-V。每次构建都会重新编译，产物在 build/shaders/
#
# 为什么 D1 阶段就接上（而不是等到 D6）：
#   这样你第一次跑 build.ps1 就会顺带验证 glslc 链路是否通 —— 路径、环境变量、
#   自定义命令都对不对，一次暴露。否则要拖到 D6 才发现问题，前面几章白跑。
add_shaders(learn_vulkan
    shader.vert
    shader.frag
)

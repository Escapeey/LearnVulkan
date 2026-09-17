# GLSL -> SPIR-V 的构建规则。
#
# 用法（在 CMakeLists.txt 里）：
#     add_shaders(learn_vulkan shader.vert shader.frag)
#
# 输入是 shaders/ 目录下的文件名，产物是 <build>/shaders/<名字>.spv。
# 运行时代码通过编译期宏 SHADER_DIR 定位这个目录。

find_program(GLSLC_EXECUTABLE
    NAMES glslc glslc.exe
    HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VK_SDK_PATH}/Bin")
mark_as_advanced(GLSLC_EXECUTABLE)

function(add_shaders TARGET)
    if(NOT ARGN)
        return()
    endif()

    if(NOT GLSLC_EXECUTABLE)
        message(WARNING
            "找不到 glslc。请确认已安装 Vulkan SDK 并重启终端（VULKAN_SDK 环境变量必须生效）。"
            " 着色器将不会被编译，运行时会加载 .spv 失败。")
        return()
    endif()

    set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(spv_files "")

    foreach(shader IN LISTS ARGN)
        set(src "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}")
        set(spv "${output_dir}/${shader}.spv")

        add_custom_command(
            OUTPUT  "${spv}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
            COMMAND "${GLSLC_EXECUTABLE}" "${src}" -o "${spv}"
            DEPENDS "${src}"
            COMMENT "glslc: ${shader} -> ${shader}.spv"
            VERBATIM)

        list(APPEND spv_files "${spv}")
    endforeach()

    add_custom_target(${TARGET}_shaders DEPENDS ${spv_files})
    add_dependencies(${TARGET} ${TARGET}_shaders)
endfunction()

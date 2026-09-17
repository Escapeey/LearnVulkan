#version 450

// ---------------------------------------------------------------------------
//  D6：顶点着色器（Hello Triangle）
//
//  与 OpenGL 的 GLSL 相比，这里有三处必须注意的差异：
//
//  1. 版本号是 450（Vulkan 用的 GLSL 方言），不是 330 core
//  2. 所有输入/输出都必须用 layout(location = N) 显式编号 ——
//     Vulkan 不提供 glGetAttribLocation / glBindAttribLocation 这类按名字查找的机制
//  3. gl_VertexID 在 Vulkan 里叫 gl_VertexIndex，而且是「顶点索引」不是「顶点序号」
//     （索引缓冲章节会体现差别）
// ---------------------------------------------------------------------------

// 输出给片元着色器。location = 0 必须与 shader.frag 里的 in 编号一致
// —— 靠编号匹配，不是靠名字。这是 Vulkan GLSL 最重要的规则。
layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2( 0.0, -0.5),
    vec2( 0.5,  0.5),
    vec2(-0.5,  0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),   // 红
    vec3(0.0, 1.0, 0.0),   // 绿
    vec3(0.0, 0.0, 1.0)    // 蓝
);

void main() {
    // gl_Position 是内建输出，屏幕空间的裁剪坐标（NDC 的四分量形式）
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

    fragColor = colors[gl_VertexIndex];
}

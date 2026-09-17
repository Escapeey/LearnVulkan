#version 450

// ---------------------------------------------------------------------------
//  D6：片元着色器（Hello Triangle）
//
//  与 OpenGL 的两处关键差异：
//
//  1. 没有 gl_FragColor。输出必须自己声明，并用 layout(location = 0) 指定颜色附件编号
//     （OpenGL 里对应的是 glBindFragDataLocation）
//  2. 从顶点着色器接收的变量，编号必须与 shader.vert 的 out 完全一致
//     —— 名字可以完全不同，编号一样就能连上
// ---------------------------------------------------------------------------

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}

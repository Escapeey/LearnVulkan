# shaders/

GLSL 源码放这里。CMake 会在编译时调用 Vulkan SDK 的 `glslc` 把每个文件编译成
SPIR-V，产物在 `build/shaders/<名字>.spv`。

## 为什么 Vulkan 要预编译着色器？

OpenGL 把 GLSL 源码直接交给驱动，**驱动在运行时编译**。这带来两个问题：

1. 首次使用某个 shader 时会卡顿（驱动现场编译），游戏里表现为掉帧。
2. 每个显卡厂商的 GLSL 编译器实现不同，同一份代码在不同驱动上行为可能不一致。

Vulkan 改成：**你离线编译成 SPIR-V 字节码**，驱动只做最后的机器码翻译。
所以你必须自己跑 `glslc` —— 这一步 CMake 已经帮你接好了。

## 加载方式

运行时代码通过编译期宏 `SHADER_DIR` 定位 .spv 目录（见 `CMakeLists.txt`），
所以你不必关心当前工作目录是什么。范例：

```cpp
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(std::string(SHADER_DIR) + "/" + filename, std::ios::ate | std::ios::binary);
    // ...
}
```

调用：`readFile("shader.vert.spv")`

## 新增着色器

在根目录 `CMakeLists.txt` 里把文件名加进 `add_shaders(...)`：

```cmake
add_shaders(learn_vulkan
    shader.vert
    shader.frag
)
```

## GLSL 写 Vulkan 着色器的两个坑

1. **必须显式指定 `layout(location = N)`**，不能像 OpenGL 那样靠 `glGetAttribLocation` 查。
2. 顶点着色器的输出变量用 `layout(location = 0) out vec3 fragColor;`，
   片元着色器用 `layout(location = 0) in vec3 fragColor;` 对应 —— 靠**编号**匹配，靠**名字**不行。

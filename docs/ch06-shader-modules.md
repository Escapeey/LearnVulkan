# D6 讲义：图形管线简介 + 着色器模块

> 教程章节：[图形管线简介](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Introduction) ·
> [着色器模块](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules)
>
> 前置：D5 完成（`swapChainImageViews` 已就绪）
> 自检题：`docs/ch06-check.md`

---

## 0. 本单元你要亲手写的

| 函数 | 难度 | 说明 |
|---|---|---|
| `readFile()` | ★★ | 读二进制 `.spv`，注意 4 字节对齐 |
| `createShaderModule()` | ★★ | `VkShaderModule` 只是字节码容器 |
| `createGraphicsPipeline()` 的**开头部分** | ★★ | 两个 `VkPipelineShaderStageCreateInfo` |
| 更新 `cleanup()` | ★ | `vkDestroyShaderModule` |

> **本单元不创建完整管线。** 只做"着色器阶段"这一小块。
> `VkPipelineLayout`（D8）和 `vkCreateGraphicsPipelines`（D10）还在后面。

---

## 1. 全景：图形管线的阶段划分

```
        ┌─────────────────────────────────────────┐
顶点数据 →│ 输入装配 (Input Assembler)              │  ← 固定功能
        ├─────────────────────────────────────────┤
        │ 顶点着色器 (Vertex Shader)              │  ← ★可编程
        ├─────────────────────────────────────────┤
        │ 细分 / 几何着色器（本教程不用）          │  ← ★可编程（可选）
        ├─────────────────────────────────────────┤
        │ 光栅化 (Rasterization)                  │  ← 固定功能
        ├─────────────────────────────────────────┤
        │ 片元着色器 (Fragment Shader)            │  ← ★可编程
        ├─────────────────────────────────────────┤
        │ 颜色混合 (Color Blending)               │  ← 固定功能
        └─────────────────────────────────────────┘
```

### 对照 OpenGL

你在 OpenGL 里其实**早就用过这条管线**，只是固定功能部分是通过全局状态设置的：

| 管线阶段 | OpenGL 里你怎么设 | Vulkan 里在哪设 |
|---|---|---|
| 输入装配 | `glDrawElements` 的模式参数 | `VkPipelineInputAssemblyStateCreateInfo`（D7） |
| 顶点格式 | `glVertexAttribPointer` | `VkPipelineVertexInputStateCreateInfo`（D7） |
| 视口/裁剪 | `glViewport` / `glEnable(GL_CULL_FACE)` | `VkPipelineViewportStateCreateInfo` / `VkPipelineRasterizationStateCreateInfo`（D7） |
| 多重采样 | `glEnable(GL_MULTISAMPLE)` | `VkPipelineMultisampleStateCreateInfo`（D8） |
| 深度/模板 | `glEnable(GL_DEPTH_TEST)` + `glDepthFunc` | `VkPipelineDepthStencilStateCreateInfo`（D8） |
| 颜色混合 | `glEnable(GL_BLEND)` + `glBlendFunc` | `VkPipelineColorBlendStateCreateInfo`（D8） |
| **着色器** | `glCreateShader` / `glCompileShader` / `glLinkProgram` | **`VkShaderModule` + `vkCreateGraphicsPipelines`（本单元 + D10）** |

> **关键认知**：OpenGL 里那些 `glEnable` / `glBlendFunc` 调用，在 Vulkan 里**全部变成了
> `VkGraphicsPipelineCreateInfo` 里的结构体字段**。这就是我在 `docs/opengl-to-vulkan.md`
> 里说的"不可变管线"——所有状态在创建时固化，之后不可改。
>
> **代价**：改一个状态就要重建整条管线（或用 dynamic state）。
> **收益**：绘制时没有状态切换开销，驱动可以提前把所有东西编译好。

---

## 2. ⭐ 为什么 Vulkan 要预编译成 SPIR-V

### OpenGL 的做法（你熟悉的）

```cpp
glShaderSource(vs, 1, &src, nullptr);
glCompileShader(vs);                 // ← 驱动在【运行时】把 GLSL 编译成机器码
glAttachShader(program, vs);
glLinkProgram(program);
```

两个问题：

1. **首帧卡顿**：第一次用某个着色器时驱动现场编译。游戏里表现为进新场景掉帧。
2. **厂商差异**：NVIDIA / AMD / Intel 的 GLSL 编译器是各自实现的。同一份 GLSL，在一家能过、在另一家报错，或者行为微妙不同——这是 OpenGL 时代最经典的跨平台噩梦。

### Vulkan 的做法

**你离线把 GLSL 编译成 SPIR-V**，驱动只做最后的机器码翻译。

> **SPIR-V 不是机器码**，它是一种标准化的二进制中间表示。跨厂商一致性正是来自"大家都从同一份 SPIR-V 出发"。

| | OpenGL | Vulkan |
|---|---|---|
| 你交付给驱动的 | GLSL 源码（文本） | **SPIR-V 字节码（二进制）** |
| 谁编译 | 驱动，运行时 | 你，构建时（`glslc`） |
| 首帧卡顿 | 有 | 无（编译在构建期完成） |
| 跨厂商一致性 | 差 | 好 |
| 工具链 | 无 | `glslc` / `glslangValidator` |

> **和你的 GLEW/GLAD 经验对照**：这两件事的模式是一样的——**OpenGL 让驱动做的事，Vulkan 让你自己做**。
> GLEW/GLAD 负责"找函数地址"，`glslc` 负责"编译着色器"。
> 区别是 GLEW/GLAD 有自动生成工具，`glslc` 你要自己在构建系统里接。

### 本工程已经接好了

`cmake/Shaders.cmake` 定义了 `add_shaders()`，根 `CMakeLists.txt` 里已经列上了这两个文件：

```cmake
add_shaders(learn_vulkan
    shader.vert
    shader.frag
)
```

**每次构建都会自动执行**（等价于）：

```bash
glslc shaders/shader.vert -o build/shaders/shader.vert.spv
glslc shaders/shader.frag -o build/shaders/shader.frag.spv
```

产物在 `build/shaders/`。运行时代码通过编译期宏 `SHADER_DIR` 定位它，所以**你不用关心当前工作目录**：

```cpp
std::ifstream file(std::string(SHADER_DIR) + "/" + filename, std::ios::ate | std::ios::binary);
```

> 💡 **额外好处**：因为着色器现在从第一次构建就开始编译，**你 D1 跑 `build.ps1` 时就会顺带验证 `glslc` 链路是通的**。
> 如果 `glslc` 没找到或路径不对，你会在 D1 就发现，而不是拖到 D6。

---

## 3. Vulkan GLSL 与 OpenGL GLSL 的差异

你已经会写 GLSL，但 **Vulkan 用的是不同的方言**。这五处差异必须记住：

| # | OpenGL GLSL | Vulkan GLSL | 说明 |
|---|---|---|---|
| 1 | `#version 330 core` | `#version 450` | 版本号不同，且 Vulkan 没有 `core`/`compatibility` 之分 |
| 2 | 输入可用 `glGetAttribLocation` 按名字查 | **必须显式 `layout(location = N)`** | Vulkan 没有按名字查找的机制，全靠编号 |
| 3 | 顶点序号是 `gl_VertexID` | **`gl_VertexIndex`** | 命名不同。而且它语义是"索引"——D14 用索引缓冲时你会看到区别 |
| 4 | 片元输出用 `gl_FragColor` | **必须显式声明 `layout(location = 0) out vec4 outColor;`** | 相当于 OpenGL 的 `glBindFragDataLocation` |
| 5 | 顶点→片元传值靠名字匹配 | **靠 `location` 编号匹配** | **名字可以完全不同，编号一样就能连上** |

### 第 5 点最容易出错，举例说明

```glsl
// shader.vert
layout(location = 0) out vec3 fragColor;
```
```glsl
// shader.frag
layout(location = 0) in vec3 whateverName;   // ← 名字不同完全没问题
```

**只要编号一致就连得上。** 反过来，编号不一致时——
`shader.frag` 读到的会是未定义的值，而**编译期和管线创建期都可能不报错**。

> 这是 Vulkan 里很典型的一类问题：**接口不匹配不一定有人告诉你，只能靠画面不对来发现。**
> 所以我在讲义里反复强调"验证层是老师，但它不是万能的"——它检查的是 API 使用规范，
> 不是你的渲染逻辑正确性。

---

## 4. `VkShaderModule`：它不是"着色器程序"

### 对照 OpenGL

| OpenGL | Vulkan |
|---|---|
| `glCreateShader` + `glShaderSource` + `glCompileShader` | `vkCreateShaderModule`（加载已编译好的 SPIR-V） |
| `glAttachShader` + `glLinkProgram` | **`vkCreateGraphicsPipelines`**（D10） |
| **4 个概念** | **2 个概念** |

**`VkShaderModule` 只是 SPIR-V 字节码的容器**，里面没有"程序"的概念。真正的"程序"在创建图形管线时才形成——因为**管线需要知道所有固定功能状态，才能把着色器编译成最终形式**。

### 关键推论：可以立刻销毁

```cpp
VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

// ... 用它们填 VkPipelineShaderStageCreateInfo ...

vkDestroyShaderModule(device, vertShaderModule, nullptr);   // ← 管线建好后立刻就能销毁
vkDestroyShaderModule(device, fragShaderModule, nullptr);
```

**为什么？** 因为 `vkCreateGraphicsPipelines` 已经把字节码消化掉了。模块本身不再需要存活。

> 教程在《结论》章节（D10）会提这一点。**现在先建立这个认知**：Vulkan 里的"生命周期最短化"
> 是一种常见优化——用完就销毁，不用等到程序结束。

### `createShaderModule()` 的两个坑

```cpp
VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());   // ← 坑 1
    // ...
}
```

**坑 1：`pCode` 是 `const uint32_t*`，不是 `const char*`**

SPIR-V 要求 4 字节对齐，所以要用 `reinterpret_cast`。`std::vector<char>` 的分配满足 `max_align_t` 对齐要求，所以安全。

**坑 2：`codeSize` 必须是 4 的倍数**

如果 `.spv` 文件损坏或被截断，`codeSize` 不是 4 的倍数，`vkCreateShaderModule` 会返回 `VK_ERROR_INVALID_SHADER_NV` 之类的错误或直接崩溃。**`glslc` 生成的 `.spv` 一定满足，但你要知道这个约束存在。**

### `VkPipelineShaderStageCreateInfo`

```cpp
VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
vertShaderStageInfo.module = vertShaderModule;
vertShaderStageInfo.pName  = "main";    // ← 坑 3
```

**坑 3：`pName` 必须传 `"main"`，传 `nullptr` 会崩溃**

`pName` 是**入口函数名**。因为 SPIR-V 理论上可以有多个入口点，所以必须指定。GLSL 的入口函数习惯叫 `main`，所以这里是 `"main"`。

> 还有个字段 `pSpecializationInfo`，用于"着色器常量特化"——可以在创建管线时把着色器里的
> `layout(constant_id = 0) const int X = 1;` 替换成别的值，避免为每个变体重新编译。
> 本教程不用，但知道它的存在有助于你以后理解引擎的着色器变体管理。

---

## 5. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 崩溃在 `vkCreateShaderModule` | `pCode` 没用 `reinterpret_cast<const uint32_t*>` | 见第 4 节坑 1 |
| 2 | 崩溃在 `vkCreateGraphicsPipelines`（D10） | `pName` 传了 `nullptr` | 传 `"main"` |
| 3 | 运行时打不开 `.spv` | `glslc` 没跑（SDK 没装 / `SHADER_DIR` 不对） | 检查 `build/shaders/` 有没有 `.spv` |
| 4 | 画面颜色不对但没报错 | `location` 编号不匹配 | 见第 3 节第 5 点 |
| 5 | `glslc` 报 `#version` 相关错误 | 忘了 Vulkan 用 `#version 450`，或写了 `core` | 见第 3 节 |
| 6 | 退出时报 `VkShaderModule` 泄漏 | 忘了销毁（或销毁了两次） | 见第 4 节"可以立刻销毁" |
| 7 | 顶点着色器编译报 `gl_VertexID` 未定义 | Vulkan 里叫 `gl_VertexIndex` | 见第 3 节第 3 点 |

---

## 6. 🔬 建议你自己做的三个观察实验

**这些实验没有"预期结果"，两种结果都有信息量**——目的是让你摸清 `glslc` 和验证层的边界在哪。

1. **把 `shader.vert` 的 `#version 450` 改成 `#version 330 core`**，重新构建，看 `glslc` 报什么。
2. **把 `shader.frag` 的 `layout(location = 0) in vec3 fragColor;` 改成 `location = 1`**，重新构建并运行。观察：
   - `glslc` 报错了吗？
   - 管线创建报错了吗？
   - 验证层说什么了吗？
   - 画面变成什么样了？
3. **把 `shader.vert` 的 `gl_VertexIndex` 改成 `gl_VertexID`**，看 `glslc` 报什么。

把三次的**实际观察**记进 `docs/ch06-check.md`。第 2 个实验尤其重要——它会让你亲身体会到
"**Vulkan 不保证抓到你所有的错误**"这件事。

---

## 7. 本单元完成标志

- [ ] `build/shaders/` 下能看到 `shader.vert.spv` 和 `shader.frag.spv`
- [ ] `readFile()` / `createShaderModule()` 能正确加载并创建模块
- [ ] 两个 `VkPipelineShaderStageCreateInfo` 填好（`pName = "main"`）
- [ ] `cleanup()` 里正确销毁了 shader module
- [ ] 完成第 6 节的三个观察实验并记录结果
- [ ] 能说清 `VkShaderModule` 和 OpenGL 的 program 对象的区别
- [ ] 能说出 Vulkan GLSL 与 OpenGL GLSL 的至少 4 处差异
- [ ] 能回答 `docs/ch06-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D6 打勾 + 3 行总结

---

## 8. 前瞻

D7 / D8 填完固定功能状态（顶点输入、视口、光栅化、混合……），D9 建渲染通道，**D10 才真正调用 `vkCreateGraphicsPipelines`** —— 那是整个教程最关键的一次 API 调用。

M1（彩色三角形）在 **D11**。

# D2 讲义：基础代码 + VkInstance + 验证层

> 教程章节：[基础代码](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Setup/Base_code) ·
> [实例](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Setup/Instance) ·
> [验证层](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Setup/Validation_layers)
>
> 代码骨架：`src/main.cpp`（所有 `// TODO(ch02)` 由你实现）
> 自检题：`docs/ch02-check.md`

---

## 0. 本单元你要亲手写的

| 函数 | 难度 | 说明 |
|---|---|---|
| `initWindow()` | ★ | GLFW 初始化 + 创建窗口，**不要**创建任何图形 API context |
| `checkValidationLayerSupport()` | ★★ | 第一次接触"两段式枚举"模式 |
| `getRequiredExtensions()` | ★ | GLFW 给的扩展 + 条件追加 debug utils |
| `createInstance()` | ★★★ | 本单元核心。两个结构体 + pNext 技巧 |
| `populateDebugMessengerCreateInfo()` | ★★ | 填 messenger 创建信息 |
| `debugCallback()` | ★★ | 签名不能改，体会 Vulkan 的调用约定要求 |
| `setupDebugMessenger()` | ★★ | 调用你自己写的扩展函数代理 |
| `CreateDebugUtilsMessengerEXT` / `DestroyDebugUtilsMessengerEXT` | ★★ | **你 OpenGL 经验直接迁移的地方** |
| `cleanup()` | ★ | 逆序销毁，教程要求你故意写错一次 |

---

## 1. 全景：VkInstance 是整张图的根

```
VkInstance  ← 你现在在这里（D2）
   │
   ├── VkSurfaceKHR        (D4，来自 GLFW)
   └── VkPhysicalDevice    (D3)
          └── VkDevice     (D3)
                 ├── VkQueue          (D3)
                 ├── VkSwapchainKHR   (D5)
                 ├── VkRenderPass     (D9)
                 ├── VkPipeline       (D10)
                 └── ...              (D11+)
```

**`VkInstance` 是所有 Vulkan 对象的祖先。** 它一经创建就活到程序结束，中间不做任何事——它只是"你的程序 ↔ Vulkan loader ↔ 驱动"之间的那条连接。

### OpenGL 对照

| | OpenGL | Vulkan |
|---|---|---|
| 有没有"实例" | ❌ 没有对应物 | `VkInstance` |
| 等价工作谁做的 | `wglCreateContext` / `glfwCreateWindow` 顺带创建 context，再由 GLEW/GLAD 初始化函数指针表 | **显式两步**：`vkCreateInstance` 建立连接，`vkGetInstanceProcAddr` 取函数 |
| 能不能告诉驱动"我是谁" | ❌ 没有渠道。`wglCreateContext` 只关心像素格式和版本 | ✅ `VkApplicationInfo` 让你声明应用名、引擎名、目标 API 版本 |
| 能不能用扩展 | 运行时字符串匹配 + `wglGetProcAddress` | 创建实例时**显式列出**，不列就没有 |

> **关键认知**：Vulkan 里"实例"不是"上下文"的改名。上下文是"我要画图"的容器；实例是"我要开始跟 Vulkan 打交道"的握手。窗口、设备、交换链全部是实例的后代。

---

## 2. 基础代码：为什么包成一个类

教程的结构就是全教程的骨架，认准它：

```cpp
void run() {
    initWindow();     // GLFW + 窗口
    initVulkan();     // 之后每章往这里加一个 createXxx()
    mainLoop();       // 事件循环（D11 起里面会调 drawFrame()）
    cleanup();        // 每章往这里加一个 destroy
}
```

`main()` 用 `try/catch(std::exception)` 包住，出错就打印 `what()` 并返回 `EXIT_FAILURE`。这是全教程的错误处理范式：**任何 Vulkan 调用返回非 `VK_SUCCESS`，就 `throw std::runtime_error("failed to ...")`**。

### 为什么教程不用 RAII？

教程原话：*"Vulkan 的特点是对每个操作都显式说明以避免错误，因此显式说明对象的生命周期以了解 API 的工作原理是很好的。"*

**这对你尤其重要。** 你从 OpenGL 来，习惯了"创建纹理、绑定、忘了删也没事"。Vulkan 里资源生命周期搞错会直接在验证层里炸出来。前 20 个单元全部手写 `createXxx` / `destroyXxx`，等 M1 之后再考虑 RAII 封装。

---

## 3. VkApplicationInfo：Vulkan 的"自我介绍"

```cpp
VkApplicationInfo appInfo{};
appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
appInfo.pApplicationName = "Hello Triangle";
appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
appInfo.pEngineName = "No Engine";
appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
appInfo.apiVersion = VK_API_VERSION_1_0;
```

技术上全是可选的，但驱动会用它做优化决策——比如识别出"这是 Unreal 引擎"就走某些特殊路径。**`apiVersion` 不是"驱动版本"，而是"我保证只调用这个版本范围内的 API"**，驱动据此选择行为。

### ⚠️ 第一个大坑：`{}` 不能省

```cpp
VkApplicationInfo appInfo{};   // ✅ 所有字段先清零
VkApplicationInfo appInfo;     // ❌ 栈上垃圾值，随机崩溃
```

**Vulkan 的所有结构体都必须值初始化。** 原因：结构体有很多字段，你只想填其中几个；剩下的如果不清零，就是栈上的随机字节——驱动会读到垃圾指针然后段错误。

> 这条规则贯穿全教程 100+ 个结构体。**从现在起形成条件反射：声明 Vulkan 结构体一律带 `{}`。**

### `sType` 和 `pNext`：Vulkan 的通用约定

- **`sType`**：显式声明"我这个结构体是什么类型"。C 没有 RTTI，驱动只能靠这个字段判断你传进来的指针到底该按哪种结构体解释。
- **`pNext`**：指向一个结构体链，用于传递**扩展**信息。基本结构体保持不动的版本 ABI 下，扩展通过往链上挂节点来扩展功能。

```
VkInstanceCreateInfo
   └─ pNext ──> VkDebugUtilsMessengerCreateInfoEXT ──> pNext ──> ... 
```

**记住这个模式，D8 的管线创建、D17 的图像创建都会用到 pNext 链。**

---

## 4. VkInstanceCreateInfo：扩展与层

```cpp
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
createInfo.enabledExtensionCount   = ...;
createInfo.ppEnabledExtensionNames = ...;
createInfo.enabledLayerCount       = ...;
createInfo.ppEnabledLayerNames     = ...;
```

注意 `ppEnabledExtensionNames` 是 **`const char**`**（指针的指针）——它是"字符串数组"，不是单个字符串。这是 C API 表达数组的经典方式。

### 扩展从哪来？GLFW 帮你解决平台差异

```cpp
uint32_t glfwExtensionCount = 0;
const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
```

Vulkan 是平台无关的，但**"把图像送到窗口"这件事天然是平台相关的**：Windows 要 `VK_KHR_win32_surface`，Linux 要 `VK_KHR_xlib_surface`/`wayland_surface`，macOS 走 MoltenVK 的 `VK_KHR_portability_enumeration`。

GLFW 的 `glfwGetRequiredInstanceExtensions()` 就是替你答这道题。**这就是为什么教程选 GLFW 而不是裸写 Win32** ——你还记得 Win32 创建窗口那套 WNDCLASS/消息循环有多长吧。

---

## 5. 验证层：你未来三周最重要的老师

### 为什么需要

Vulkan 的设计目标是"最小化驱动开销"，代价是**默认几乎不做错误检查**：

> 即使是像把枚举设成不正确的值、或把空指针传给必需参数这样简单的错误，通常也不会被显式处理，只会导致崩溃或未定义行为。

对照 OpenGL：`glGetError()` 至少还给你个错误码。Vulkan 连这个都没有——**不装验证层，你面对的就是黑屏和段错误**。

### 验证层是什么

验证层是**挂在 Vulkan 调用链上的可选组件**：

```cpp
// 验证层内部大概长这样
VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, ...) {
    if (pCreateInfo == nullptr || instance == nullptr) {
        log("Null pointer passed to required parameter!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return real_vkCreateInstance(pCreateInfo, ...);   // 转发给真正的实现
}
```

它们可以自由堆叠：参数校验、对象泄漏跟踪、线程安全检查、调用日志、性能分析……标准的一套叫 **`VK_LAYER_KHRONOS_validation`**，由 LunarG SDK 提供。

**只给 Debug 构建启用**，Release 关掉，两全其美。

### ⭐ 两段式枚举模式（本单元最值得记住的东西）

```cpp
uint32_t count = 0;
vkEnumerateInstanceLayerProperties(&count, nullptr);          // 第一步：问数量
std::vector<VkLayerProperties> layers(count);
vkEnumerateInstanceLayerProperties(&count, layers.data());    // 第二步：填数组
```

**这个模式在整本教程里至少出现 8 次**：实例层、实例扩展、物理设备、队列族、交换链图像、表面格式、呈现模式、描述符……

D2 学一次，后面全是复读。现在理解透，D5 的交换链那堆 `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` 就不会慌。

### `checkValidationLayerSupport()` 要做什么

1. 两段式枚举出所有可用层
2. 对 `validationLayers` 里的每个名字，在可用层里用 `strcmp` 找一遍
3. 有一个找不到就返回 `false`

然后在 `createInstance()` 开头：

```cpp
if (enableValidationLayers && !checkValidationLayerSupport()) {
    throw std::runtime_error("validation layers requested, but not available!");
}
```

`enableValidationLayers` 用 `NDEBUG` 控制（`NDEBUG` 在 Release 构建里由标准定义为"非调试"）。

---

## 6. 调试回调

### 函数签名不能改

```cpp
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
```

`VKAPI_ATTR` / `VKAPI_CALL` 保证调用约定（Windows 上通常是 `__stdcall`）。类型是 `PFN_vkDebugUtilsMessengerCallbackEXT`，**签名写错会直接崩溃**，验证层不会告诉你为什么。

### 四个参数

| 参数 | 含义 |
|---|---|
| `messageSeverity` | 严重性等级 |
| `messageType` | 消息类别 |
| `pCallbackData->pMessage` | **消息本体，你最常看的** |
| `pUserData` | 你在创建时塞进去的自定义指针（教程传 `nullptr`） |

### 严重性等级与 `>=` 比较

```
VERBOSE_BIT_EXT < INFO_BIT_EXT < WARNING_BIT_EXT < ERROR_BIT_EXT
```

这些枚举的值被设计成递增的，所以可以直接比较：

```cpp
if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // 值得看
}
```

**建议**：不要照抄教程只打一行。按等级分流（WARNING/ERROR → `std::cerr`，其他 → 忽略），这样报错会跳出来、正常信息不刷屏。

### `messageType` 三类

- `GENERAL`：与规范/性能无关的一般事件
- `VALIDATION`：**违反规范或可能出错** ← 你排查问题主要看这个
- `PERFORMANCE`：Vulkan 用法非最优（早期可以先忽略）

### 回调返回值

返回 `VK_TRUE` 会**中止**触发该消息的 Vulkan 调用并产生 `VK_ERROR_VALIDATION_FAILED_EXT`。这只用于测试验证层本身，**永远返回 `VK_FALSE`**。

---

## 7. ⭐ 扩展函数代理：你 OpenGL 经验直接迁移的地方

`vkCreateDebugUtilsMessengerEXT` **不在 loader 导出的函数表里**（它是扩展函数）。所以不能直接调用、也不能直接链接，必须自己查地址：

```cpp
auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
} else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}
```

**这不就是你 GLEW/GLAD 干的事吗？**

| | OpenGL | Vulkan |
|---|---|---|
| 取函数地址 | `wglGetProcAddress("glGenBuffers")` / GLEW 包装 | `vkGetInstanceProcAddr(instance, "vkXxx")` |
| 谁帮你做 | GLEW / GLAD 自动生成一大堆宏 | **没有自动化工具，你自己写**（或者用 `Vulkan-Hpp` / `volk`） |
| 为什么需要 | OpenGL 1.1 以上的函数不由 `opengl32.dll` 导出 | 扩展函数不由 `vulkan-1.dll` 导出 |

教程只用到 2 个这样的函数。**本教程里其余所有 `vkXxx` 都是核心函数，可以直接调用。**

---

## 8. `pNext` 技巧：调试实例创建和销毁

### 问题

`VkDebugUtilsMessengerEXT` 依赖 `VkInstance` 存在。所以：

```
创建顺序：VkInstance → VkDebugUtilsMessengerEXT
销毁顺序：VkDebugUtilsMessengerEXT → VkInstance
```

**结果：`vkCreateInstance` 和 `vkDestroyInstance` 本身出问题时，你收不到任何消息** —— 而这恰恰是最容易出问题的两步。

### 解法

把 messenger 的创建信息直接挂到 `VkInstanceCreateInfo::pNext` 上，让驱动在实例创建/销毁期间临时用这个 callback：

```cpp
VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
if (enableValidationLayers) {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    populateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
} else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
}

vkCreateInstance(&createInfo, nullptr, &instance);
```

### ⚠️ 第二个大坑：生命周期

`debugCreateInfo` **必须定义在 `if` 外面**，保证它在 `vkCreateInstance` 返回之后才离开作用域。

如果写在 `if` 里面，`if` 块结束时 `debugCreateInfo` 析构，而 `createInfo.pNext` 还指着它的地址——`vkCreateInstance` 读到悬垂指针，轻则乱报错，重则崩溃。教程原文特意强调：*"`debugCreateInfo` 变量放置在 `if` 语句外部，以确保它在 `vkCreateInstance` 调用之前不会被销毁。"*

**这是你第一次遇到 Vulkan 的"指针生命周期"问题，后面 D8 的管线创建、D15 的描述符写入都会反复遇到同样的陷阱。**

---

## 9. 本单元最容易踩的坑（按出现频率排序）

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | 随机崩溃 / 验证层说 `pApplicationInfo` 有问题 | 结构体没写 `{}` | 所有 Vulkan 结构体一律 `{}` |
| 2 | 验证层报 `sType is invalid` | 忘了设 `sType` | 每个结构体设 `sType` |
| 3 | `VK_ERROR_EXTENSION_NOT_PRESENT` | 没把 GLFW 的扩展加进 `enabledExtensionNames` | 用 `getRequiredExtensions()` |
| 4 | 启动就 `throw`："validation layers requested, but not available!" | SDK 没装 / 终端没重启 / `VK_LAYER_PATH` 没设 | 跑 `.\scripts\run.ps1 -Target env_check` 确认 |
| 5 | 程序崩溃在 `setupDebugMessenger` | 没检查 `CreateDebugUtilsMessengerEXT` 的返回值，或代理函数没写对 | 检查 `!= VK_SUCCESS` 就 throw |
| 6 | 退出时验证层报 "destroyed after VkInstance" | `cleanup()` 顺序错 | **教程要求你故意先写错一次，亲眼看看报错长什么样** |
| 7 | 回调里读 `pCallbackData` 崩溃 | 签名写错（少了 `VKAPI_CALL`） | 严格照抄签名 |

---

## 10. 教程要求的两个"故意犯错"实验（别跳过）

教程在"测试"一节要求你：**暂时删掉 `cleanup()` 里对 `DestroyDebugUtilsMessengerEXT` 的调用**，然后运行。

退出后你会看到类似：

```
validation layer: Validation Error: [ VUID-vkDestroyInstance-instance-00629 ] Object 0: handle = 0x... , type = VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT; ...
```

**这一步的价值远超它花的时间**：你第一次亲眼看到验证层如何精确指出"你泄漏了哪个对象、在哪一步违反了什么规则（VUID）"。以后所有报错你都会读得懂。

**做完记得改回来。** 然后在 `PROGRESS.md` 的踩坑表里记一行。

---

## 11. 本单元完成标志

- [ ] 窗口正常出现，关闭窗口后程序返回 0
- [ ] 控制台能看到验证层输出（哪怕只是 INFO 级）
- [ ] 完成了"故意不销毁 messenger"实验，并读懂了报错
- [ ] 能回答 `docs/ch02-check.md` 的全部问题
- [ ] `PROGRESS.md` 里 D2 打勾 + 写了 3 行总结

完成后 `git add -A && git commit -m "D2: instance + validation layers"`，然后我们进 D3（物理设备 + 队列族 + 逻辑设备）。

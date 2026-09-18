# D4 讲义：窗口表面 + 交换链探测

> 教程章节：[窗口表面](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Presentation/Window_surface) ·
> [交换链](https://tutorial.vulkan.net.cn/Drawing_a_triangle/Presentation/Swap_chain)（前半：查询与选择）
>
> 前置：D3 完成（`physicalDevice` / `device` / `graphicsQueue` 就绪）
>
> 📊 **你这台机器的实测硬件数据**（交换链能力 / 队列族 / 呈现模式 / 深度格式）：[`your-gpu.md`](your-gpu.md)
> 自检题：`docs/ch04-check.md`
>
> **本单元只做"查询和决策"，不创建交换链。** 创建在 D5。

---

## 0. 本单元你要亲手写的

| 函数 | 难度 | 说明 |
|---|---|---|
| `createSurface()` | ★ | 一行 GLFW 调用 + 销毁 |
| `checkDeviceExtensionSupport()` | ★★ | 又见两段式枚举，这次用 `std::set` 做差集 |
| `querySwapChainSupport()` | ★★★ | 三次查询，两种风格（单个结构体 vs 列表） |
| `chooseSwapSurfaceFormat()` | ★★ | 第一个"查询-选择"范例 |
| `chooseSwapPresentMode()` | ★★ | 理解四种呈现模式的取舍 |
| `chooseSwapExtent()` | ★★★ | **高 DPI 的坑在这里** |
| 更新 `createLogicalDevice()` | ★ | `enabledExtensionCount` 从 0 改成 `VK_KHR_swapchain` |
| 更新 `isDeviceSuitable()` | ★ | 第三次长大：加交换链可用性检查 |

---

## 1. 全景：为什么需要"交换链"

> Vulkan 没有"默认帧缓冲"的概念，因此它需要一个基础设施，该基础设施将拥有我们将在屏幕上可视化之前渲染到的缓冲区。此基础设施被称为**交换链**。

交换链本质上是**一个等待呈现到屏幕的图像队列**。你的应用从队列取一张图来画，画完还回去。

### OpenGL 对照

| | OpenGL | Vulkan |
|---|---|---|
| 前后缓冲 | 驱动隐式管理，`glBindFramebuffer(0)` 就能用 | 你从 `VkSwapchainKHR` 取图像 |
| 交换 | `SwapBuffers(hdc)` 一行 | `vkAcquireNextImageKHR` + `vkQueuePresentKHR`（D11） |
| 垂直同步 | `wglSwapIntervalEXT(1)`，一个 bool | `VkPresentModeKHR` 四选一 + 你自己的"飞行帧数" |
| 图像数量 | 驱动决定（通常 2） | `minImageCount` ~ `maxImageCount`，你自己谈 |
| 分辨率 | 跟随窗口，驱动处理 | 你要查 `currentExtent` 或自己算（**高 DPI 陷阱**） |
| 窗口缩放 | 驱动自动重建默认帧缓冲 | 你手动重建整条交换链（D12） |
| 像素格式 | `ChoosePixelFormat` + `PIXELFORMATDESCRIPTOR`，一次性 | `VkSurfaceFormatKHR` 列表，运行时挑 |

> **关键认知**：OpenGL 里 `SwapBuffers` 那一行，在 Vulkan 里展开成了整整两个章节（D4 + D5）。
> 这不是 Vulkan 故意为难你——这是驱动过去在背后做的事情第一次暴露在你面前。
> 而它的收益在 D12 会体现出来：当窗口缩放时，你能精确控制重建什么、复用什麼。

---

## 2. `createSurface()`：让 Vulkan 认识你的窗口

```cpp
void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}
```

一行搞定。但**要知道它在背后做了什么**，这正是"平台无关"的代价：

```cpp
// GLFW 在 Windows 上实际执行的是：
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
createInfo.hwnd = glfwGetWin32Window(window);      // 窗口句柄
createInfo.hinstance = GetModuleHandle(nullptr);   // 进程实例句柄
vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
```

- Linux 上是 `vkCreateXcbSurfaceKHR`（X11）或 `vkCreateWaylandSurfaceKHR`
- Android 上是 `vkCreateAndroidSurfaceKHR`
- **`VkSurfaceKHR` 是平台无关的，创建它的过程不是** —— 这就是 WSI（窗口系统集成）扩展存在的理由

`VK_KHR_surface` 和 `VK_KHR_win32_surface` 这两个实例扩展**你早就启用了**——它们是 `glfwGetRequiredInstanceExtensions()` 返回值的一部分（D2 的 `getRequiredExtensions()`）。当时你可能没注意，现在知道它们是干什么的了。

### 销毁顺序

```cpp
vkDestroySurfaceKHR(instance, surface, nullptr);  // 必须在 vkDestroyInstance 之前
```

所以 `cleanup()` 变成：

```
vkDestroyDevice → vkDestroySurfaceKHR → DestroyDebugUtilsMessengerEXT → vkDestroyInstance → window → glfwTerminate
```

> `VkSurfaceKHR` 依赖 `VkInstance`，但**不依赖 `VkDevice`**。不过交换链依赖 device，所以从 D5 起 device 要在 surface 之前销毁。

---

## 3. 设备扩展 `VK_KHR_swapchain`

### 为什么它是**设备**扩展而不是实例扩展

> 并非所有显卡都能够出于各种原因直接将图像呈现到屏幕，例如，因为它们是为服务器设计的，并且没有任何显示输出。

**"能不能呈现到窗口"是某一块具体显卡的能力**，不是整个 Vulkan 会话的属性。有些设备只支持计算。所以它是设备扩展。

### 修改 D3 的代码

```cpp
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
```

然后在 `createLogicalDevice()` 里把 D3 那行改掉：

```cpp
// D3 时是：createInfo.enabledExtensionCount = 0;
createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
createInfo.ppEnabledExtensionNames = deviceExtensions.data();
```

> **这是 D4 最容易忘的一步。** 忘了的后果是 `vkCreateSwapchainKHR` 返回 `VK_ERROR_EXTENSION_NOT_PRESENT`，或者更糟——验证层报一堆看着无关的错误。**D3 的"故意写错"实验（注释掉 `vkDestroyDevice`）现在可以再复习一遍**：在 Vulkan 里，"忘了声明"和"忘了销毁"都会被抓出来。

### `checkDeviceExtensionSupport()`：用 `std::set` 做差集

```cpp
bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}
```

**思路很漂亮**：把"我需要的"放进集合，然后用"实际有的"去逐个删除。最后集合空了 = 全都有。

对比 D2 里 `checkValidationLayerSupport()` 的嵌套 `strcmp` 循环——两种都行，性能无关紧要。**但 `std::set` 这个写法你以后会经常用到**（D5 的 `uniqueQueueFamilies` 也是同一招）。

别忘了 `#include <set>` 和 `#include <string>`。

---

## 4. ⭐ `querySwapChainSupport()`：本单元核心

只有"扩展存在"还不够 —— 扩展存在不代表**它和你的窗口表面兼容**（比如你这块屏不支持那个格式）。

```cpp
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
```

需要查三类属性：

| 属性 | 查询函数 | 返回形式 | 内容 |
|---|---|---|---|
| 基本能力 | `vkGetPhysicalDeviceSurfaceCapabilitiesKHR` | **单个结构体** | 图像数量范围、尺寸范围、支持的变换/合成模式 |
| 表面格式 | `vkGetPhysicalDeviceSurfaceFormatsKHR` | **列表**（两段式） | 像素格式 + 色彩空间 |
| 呈现模式 | `vkGetPhysicalDeviceSurfacePresentModesKHR` | **列表**（两段式） | 四种模式里的哪些可用 |

**注意两种返回风格的差别**：

```cpp
// 单个结构体：直接传地址，一次调用
vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

// 列表：两段式（D2 学过的模式，第 3 次出现了）
uint32_t formatCount;
vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
}
```

> ⚠️ **注意 `if (formatCount != 0)` 这个判断**。列表可能为空——比如一个只能计算的设备接到窗口上。
> 后面 `chooseSwapSurfaceFormat` 里有 `availableFormats[0]`，**如果前面没判空就会越界崩溃**。

### `isDeviceSuitable()` 第三次长大

```cpp
bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {                    // ← 顺序很重要！
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}
```

> **`if (extensionsSupported)` 这个前置判断不是可选的。** 如果不支持 `VK_KHR_swapchain` 就去调
> `vkGetPhysicalDeviceSurfaceFormatsKHR`，验证层会报错（依赖不满足的扩展函数被调用）。
> **记住这个模式：先确认能力存在，再查询它的细节。**

---

## 5. 三个"查询-选择"函数

这是 Vulkan 的通用套路：**查询所有可能 → 按优先级挑最好的 → 都有兜底**。

### 5.1 `chooseSwapSurfaceFormat`：像素格式

`VkSurfaceFormatKHR` 含两个字段：
- `format`：颜色通道和类型。`VK_FORMAT_B8G8R8A8_SRGB` = 按 B,G,R,A 顺序，每通道 8 位无符号整数，共 32 位
- `colorSpace`：是否支持 SRGB 色彩空间（`VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`）

**为什么优先 SRGB？** 教程原话：*"它可以产生更准确的感知颜色"*。它也是图像（纹理）的标准色彩空间。你从 OpenGL 来应该熟——`GL_SRGB8_ALPHA8`。

```cpp
for (const auto& availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
        return availableFormat;
    }
}
return availableFormats[0];    // 没办法，凑合用第一个
```

> 历史小知识：`VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` 在旧版规范里叫 `VK_COLORSPACE_SRGB_NONLINEAR_KHR`（少了中间的下划线）。看到老代码别懵。

### 5.2 `chooseSwapPresentMode`：呈现模式（最重要的设置）

| 模式 | 行为 | 类比 |
|---|---|---|
| `IMMEDIATE` | 提交就立刻送屏 | 关掉垂直同步，**会撕裂** |
| `FIFO` | 队列，满则等待；显示器刷新时从队首取 | **垂直同步**。**唯一保证存在的模式** |
| `FIFO_RELAXED` | 基本同 FIFO，但队列空时立即送 | 迟到时才撕裂 |
| `MAILBOX` | 队列满时不阻塞，直接替换队里那张 | **三重缓冲**，低延迟且不撕裂 |

```cpp
for (const auto& availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
        return availablePresentMode;
    }
}
return VK_PRESENT_MODE_FIFO_KHR;    // 保底，规范保证它一定存在
```

> **对照你的 OpenGL 经验**：`wglSwapIntervalEXT(0)` ≈ `IMMEDIATE`，`wglSwapIntervalEXT(1)` ≈ `FIFO`。
> MAILBOX 是 OpenGL 体系里**没有对应物**的——它是 Vulkan 才让你直接选到的。
>
> **在你的 Intel UHD 630 上**：集显一般也支持 MAILBOX。但注意教程的提醒——移动设备（笔记本用电池时）
> 从省电角度可能更想要 FIFO。你可以在 D5 跑起来后**把两个模式都试一下**，感受差别。

### 5.3 `chooseSwapExtent`：分辨率 —— **这里有高 DPI 的坑**

```cpp
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);   // ← 像素，不是屏幕坐标！

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width  = std::clamp(actualExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}
```

需要 `#include <limits>` 和 `#include <algorithm>`（`std::clamp` 是 C++17）。

#### 两个必须理解的点

**① `currentExtent` 是"特殊值"约定**

`currentExtent.width == UINT32_MAX` 表示"**窗口管理器允许你自己决定分辨率**"。否则驱动已经算好了，照着用就行。这是 Vulkan 里常见的一种"用极值表示特殊含义"的约定。

**② 高 DPI：屏幕坐标 ≠ 像素**

教程原话：

> GLFW 在测量尺寸时使用两个单位：像素和屏幕坐标。我们在创建窗口时指定的 `{WIDTH, HEIGHT}` 以屏幕坐标测量。但是 Vulkan 使用像素……如果您使用的是高 DPI 显示器，屏幕坐标不对应于像素。

**这对你的机器是真实风险**：笔记本外接/内建屏经常设 125% / 150% 缩放。假设缩放 150%，你请求 `600` 屏幕坐标高度，实际像素高度是 `900`。如果直接把 `WIDTH/HEIGHT` 塞进 `VkExtent2D`，交换链尺寸就和窗口对不上——表现是画面只占左上角一部分，或者被拉伸变形。

所以**必须用 `glfwGetFramebufferSize`**。

#### 🔬 本单元建议你做的小实验

在 `createSurface()` 之后加一段打印：

```cpp
int fbWidth = 0, fbHeight = 0;
glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
int winWidth = 0, winHeight = 0;
glfwGetWindowSize(window, &winWidth, &winHeight);

std::cout << "窗口尺寸(屏幕坐标): " << winWidth << " x " << winHeight << "\n";
std::cout << "帧缓冲(像素)      : " << fbWidth << " x " << fbHeight << "\n";
```

- 两个数字相同 → 你的显示缩放是 100%
- 不同 → 你在高 DPI 环境下，**这正是 `chooseSwapExtent` 存在的理由**

把结果记到 `docs/ch04-check.md`。这个数据在你以后调 D12（交换链重建）时会用到。

---

## 6. 本单元最容易踩的坑

| # | 现象 | 根因 | 处理 |
|---|---|---|---|
| 1 | `vkCreateSwapchainKHR` 报 `VK_ERROR_EXTENSION_NOT_PRESENT` | 忘了在 `createLogicalDevice` 里启用 `VK_KHR_swapchain` | 改 `enabledExtensionCount`（第 3 节） |
| 2 | 验证层报"调用了未启用扩展的函数" | 没确认 `extensionsSupported` 就调 `querySwapChainSupport` | `if (extensionsSupported)` 前置判断 |
| 3 | 崩溃在 `chooseSwapSurfaceFormat` 的 `availableFormats[0]` | 列表为空 | 前面判过空（`swapChainAdequate`），确认 `isDeviceSuitable` 写对 |
| 4 | 画面只占窗口一角 / 被拉伸 | 用了 `WIDTH/HEIGHT` 而不是 `glfwGetFramebufferSize` | 高 DPI 环境，见第 5.3 节 |
| 5 | `std::set` / `std::clamp` / `numeric_limits` 找不到 | 缺 `<set>` / `<algorithm>` / `<limits>` | 补 include |
| 6 | `vkCreateSwapchainKHR` 崩溃，日志提到 `SteamOverlayVulkanLayer` | Steam 覆盖层注入 | **你的机器已排查过：没装 Steam，不会遇到**（见 `SETUP.md`） |
| 7 | 窗口尺寸和交换链对不上，但缩放是 100% | `currentExtent` 已被驱动固定，你却自己算 | 优先用 `capabilities.currentExtent` |

---

## 7. 本单元完成标志

- [ ] `createSurface()` 成功，程序不抛异常
- [ ] `createLogicalDevice()` 已启用 `VK_KHR_swapchain`
- [ ] `isDeviceSuitable()` 返回 `indices.isComplete() && extensionsSupported && swapChainAdequate`
- [ ] 控制台打印出了窗口尺寸 vs 帧缓冲尺寸（第 5.3 节实验），并记录到自检题
- [ ] 能说出四种呈现模式的差别，以及为什么必须有 `FIFO` 兜底
- [ ] 能回答 `docs/ch04-check.md` 全部问题
- [ ] `PROGRESS.md` 里 D4 打勾 + 3 行总结

**本单元结束时你还是看不到任何画面** —— 这是正常的。交换链只是"图像的容器"，还没创建、也没往里画东西。D5 创建交换链和图像视图，D11 才第一次出画面。

---

## 8. 前瞻：D5 会做什么

D5 接着教程《交换链》章节的后半段：

1. **`createSwapChain()`** —— 填那个巨大的 `VkSwapchainCreateInfoKHR`
   - `imageCount = minImageCount + 1`（为什么 +1？教程有解释）
   - `imageSharingMode`：`graphicsFamily == presentFamily` 时用 `EXCLUSIVE`，否则 `CONCURRENT`
     （**你的机器是单 GPU 集显，两者几乎必然相同 → EXCLUSIVE**）
   - `oldSwapchain = VK_NULL_HANDLE`（D12 才会用到非空值）
2. **`vkGetSwapchainImagesKHR`** —— 取出图像句柄（又见两段式枚举）
3. **`createImageViews()`** —— 为每张图像建 view

所以 D4 打下的 `SwapChainSupportDetails` 和三个 `choose*` 函数，D5 会直接用上。

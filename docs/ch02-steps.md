# D2 实现步骤：5 个检查点

> 配套：[`ch02-instance-and-validation.md`](ch02-instance-and-validation.md)（讲义）· [`ch02-check.md`](ch02-check.md)（自检题）
>
> **为什么要有这份**：D2 要写 10 个函数。如果你一次全写完再编译运行，出问题时你面对的是
> "一个不做任何事的程序" + "一屏报错"，根本不知道从哪查。
>
> 这份把任务切成 5 步，**每一步都能编译、能运行、有明确的期望结果**。
> 卡住时你也知道是"刚写的那一步"错了，前面的都是好的。
>
> 建议照着顺序做，别跳。

---

## 开工前

```powershell
cd LearnVulkan
.\scripts\build.ps1        # 确认当前骨架能编译（应该秒过）
```

现在程序能编译，但**跑起来会立刻报一句** `window 是 nullptr —— 你还没实现 initWindow()` 然后退出。
那是骨架里的保护（没有它，GLFW 会因 `window == nullptr` 触发 assert，在 Debug 下弹模态框把程序挂死）。
这正是检查点 1 要解决的。

---

## 检查点 1：`initWindow()`

**写什么**

```cpp
void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}
```

**跑什么**

```powershell
.\scripts\build.ps1
.\scripts\run.ps1
```

**期望看到**

- 弹出一个 800×600 的窗口，标题 `Vulkan`
- **窗口是全白的/空白的**（这是对的 —— 还没画任何东西）
- 关掉窗口后控制台打印 `退出码: 0`

**✅ 通过标志**：能开关窗口，退出码 0。

**❌ 如果崩溃**：`glfwInit()` 失败？检查显卡驱动。窗口没出现？`glfwCreateWindow` 返回了 `nullptr`，加一句 `if (!window) throw std::runtime_error("failed to create window");` 看看。

---

## 检查点 2：`getRequiredExtensions()` + `checkValidationLayerSupport()`

这两个是纯查询函数，**不创建任何对象**，所以最好单独验证。

**写什么**：实现这两个函数（讲义第 4、5 节）。

**验证方式**：在 `initVulkan()` 开头临时插一段打印：

```cpp
void initVulkan() {
    // ---- 临时验证代码，检查点 2 通过后删掉 ----
    {
        auto exts = getRequiredExtensions();
        std::cout << "需要的实例扩展 (" << exts.size() << " 个):\n";
        for (auto* e : exts) std::cout << "  " << e << "\n";
        std::cout << "验证层可用: " << std::boolalpha << checkValidationLayerSupport() << "\n";
    }
    // ---- 临时验证代码结束 ----

    createInstance();
    setupDebugMessenger();
}
```

**期望看到**

```
需要的实例扩展 (3 个):
  VK_KHR_surface
  VK_KHR_win32_surface
  VK_EXT_debug_utils
验证层可用: true
```

- 前两个来自 GLFW，第三个是你自己加的条件扩展
- 扩展数量**必须是 3**（Debug 构建）。如果是 2，说明你忘了 push debug utils；如果是 5、6 个，你多加了
- `验证层可用: true` —— 如果是 `false`，跑 `.\scripts\run.ps1 -Target env_check` 确认环境

**✅ 通过标志**：3 个扩展 + `true`。

**然后删掉那段临时代码。**

---

## 检查点 3：`createInstance()`

**写什么**：实现 `createInstance()`（讲义第 3、4、8 节）。这是 D2 最核心的一步。

⚠️ **两个必须做对的点**：
1. `VkApplicationInfo appInfo{};` —— **`{}` 不能省**
2. `debugCreateInfo` 定义在 `if` **外面**（讲义第 8 节讲了为什么）

**跑什么**：`.\scripts\build.ps1` → `.\scripts\run.ps1`

**期望看到**

- 窗口照常出现
- **没有**立即抛异常
- 控制台**可能**开始出现验证层消息（如果 `setupDebugMessenger` 还没实现，就不会有）

**❌ 常见失败**

| 现象 | 原因 |
|---|---|
| 抛 `validation layers requested, but not available!` | 环境问题（但你 D1 已验证过，不该出现） |
| 抛 `failed to create instance!` | 扩展列表或层名填错了 |
| 随机崩溃 / 验证层说 `pApplicationInfo` 有问题 | 结构体没写 `{}` |

**✅ 通过标志**：不抛异常，窗口正常，退出码 0。

---

## 检查点 4：调试信使三件套

这一步要写 4 个东西，它们是**互相依赖的一个整体**：

1. `populateDebugMessengerCreateInfo()`
2. `debugCallback()`
3. 两个自由函数 `CreateDebugUtilsMessengerEXT` / `DestroyDebugUtilsMessengerEXT`
4. `setupDebugMessenger()`

**写什么**：讲义第 6、7 节。

**跑什么**：`.\scripts\build.ps1` → `.\scripts\run.ps1`

**期望看到**

- 窗口出现
- **没有**抛 `failed to set up debug messenger!`
- 控制台可能开始出现 INFO 级的 loader 消息（正常）

**验证方式**：在窗口循环前临时加一句自我确认（`run.ps1` 里）：

其实更简单 —— 直接跑检查点 5 的实验，能报错就说明这一步对了。

**❌ 常见失败**

| 现象 | 原因 |
|---|---|
| 崩溃在 `setupDebugMessenger` | 忘了检查 `CreateDebugUtilsMessengerEXT` 的返回值，或代理函数写错 |
| 回调里读 `pCallbackData` 崩溃 | `debugCallback` 签名少写了 `VKAPI_CALL` |
| 抛 `failed to set up debug messenger!` | `VK_EXT_DEBUG_UTILS_EXTENSION_NAME` 没加进扩展列表 |

---

## 检查点 5：`cleanup()` + 故意犯错实验

**先正常写对**：

```cpp
void cleanup() {
    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}
```

跑一次，确认退出码 0、控制台**没有 ERROR**。

**然后做实验**（讲义第 10 节）——把销毁 messenger 那两行注释掉：

```cpp
void cleanup() {
    // if (enableValidationLayers) {
    //     DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    // }
    vkDestroyInstance(instance, nullptr);
    ...
}
```

再跑一次。退出后你应该看到：

```
[ERROR] vkDestroyInstance(): VkInstance ... has 1 leaked objects that have not been destroyed.
VkDebugUtilsMessengerEXT 0x10000000001
... VUID-vkDestroyInstance-instance-00629
```

**✅ 通过标志**：**看到了那条报错**。

⚠️ **什么都没看到 = 没通过**，不是"太好了没问题"。说明验证层根本没在跟你说话 —— 回到讲义第 10 节末尾的排查清单。

**看完记得改回来**，然后再跑一次确认干净。

---

## 全部完成后

1. 回答 `docs/ch02-check.md` 的 10 道题（合上代码和讲义）
2. 在 `PROGRESS.md` 给 D2 打勾 + 写 3 行总结
3. 把踩到的坑记进 `PROGRESS.md` 的表格
4. `git add -A ; git commit -m "D2: instance + validation layers"`

然后告诉我，我给你 **D3 的骨架**（物理设备 + 队列族 + 逻辑设备）。

---

## 卡住时贴什么给我

**不要只说"报错了"**。贴这三样：

1. **完整报错原文** —— 验证层报错是**多行**的，从 `[ERROR]` 或 `Validation Error` 开头到 `VUID-...` 那一行结束，整段都要
2. **你写的那几个函数** —— 直接贴代码，或者告诉我你改到哪一步了
3. **你期望发生什么、实际发生了什么**

有这三样，我基本能一次定位。

> 💡 验证层往 **stderr** 输出。如果你把输出重定向到文件，记得用 `*>`（所有流），
> 只用 `>` 会丢掉验证层的报错 —— 见 `PROGRESS.md` 里那条教训。

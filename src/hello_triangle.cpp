// ===========================================================================
//  D6：着色器模块
//
//  教程章节：
//    Drawing_a_triangle/Graphics_pipeline_basics/Introduction
//    Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
//
//  讲义  ：docs/ch06-shader-modules.md   ← 动手前先读一遍
//  自检题：docs/ch06-check.md
//
//  （D5 的 swapChain / swapChainImages / swapChainImageViews 已就绪，
//    本单元在其上继续加。）
//
//  ---------------------------------------------------------------------------
//  工作方式
//  ---------------------------------------------------------------------------
//  所有标着  // TODO(ch06)  的地方由你来实现（老代码里标 // TODO(ch02)~(ch05)
//  的都已填好，不用动）。类结构、函数签名、成员变量、常量、include 已给好。
//
//  写完：
//      .\scripts\build.ps1
//      .\scripts\run.ps1
//
//  本单元的验收标准见讲义第 7 节。核心：
//    1. readFile() 从 build/shaders/ 读出 .spv（二进制、按文件大小读）
//    2. createShaderModule() 用 reinterpret_cast<const uint32_t*> 创建模块
//    3. createGraphicsPipeline() 填好两个 VkPipelineShaderStageCreateInfo（pName = "main"）
//    4. cleanup() 销毁两个 shader module
//    ⚠️ 本单元结束时仍然看不到画面 —— VkShaderModule 只是「字节码容器」，
//       真正把它编译进管线要等到 D10 的 vkCreateGraphicsPipelines。D11 才第一次出图。
// ===========================================================================

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>  // D4 chooseSwapExtent 要用 std::clamp
#include <cstdlib>
#include <cstring>
#include <fstream>   // D6 readFile 用 std::ifstream
#include <iostream>
#include <limits>     // D4 chooseSwapExtent 要用 std::numeric_limits
#include <map>        // D3 评分版设备选择要用 std::multimap
#include <optional>   // D3 队列族索引用 std::optional
#include <set>        // D4 checkDeviceExtensionSupport 用 std::set 求差集
#include <stdexcept>
#include <string>     // D4 std::set<std::string>
#include <vector>

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// D4 新增：逻辑设备需要的设备扩展。VK_KHR_swapchain 是「设备」扩展，
// 因为「能不能把图像呈现到显示器」是某一块具体显卡的能力（见 D3 自检第 7 题）。
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// NDEBUG 由构建系统在 Release 配置下定义（"非调试"）。教程用它区分 Debug/Release。
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// ---------------------------------------------------------------------------
//  扩展函数代理
//
//  vkCreateDebugUtilsMessengerEXT / vkDestroyDebugUtilsMessengerEXT 属于
//  VK_EXT_debug_utils 扩展，不在 loader 导出的核心函数表里，因此不能直接调用，
//  必须先用 vkGetInstanceProcAddr 查出地址。
//
//  这正是你在 OpenGL 里用 GLEW / GLAD 做的事情。区别是：Vulkan 不提供自动化工具，
//  要你自己手写这两个小包装。
//
//  TODO(ch02): 实现这两个函数（讲义第 7 节 / 教程"消息回调"一节）
//    提示：
//      - 用 (PFN_vkCreateDebugUtilsMessengerEXT) 强转 vkGetInstanceProcAddr 的返回值
//      - func 为 nullptr 时返回 VK_ERROR_EXTENSION_NOT_PRESENT
//      - Destroy 版本没有返回值，func 为 nullptr 时静默返回即可
// ---------------------------------------------------------------------------
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

// ===========================================================================
class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // -----------------------------------------------------------------------
    //  D3 新增：Vulkan 的三层"设备"模型
    //    VkPhysicalDevice —— 枚举出来的只读句柄，不创建不销毁（讲义第 1 节）
    //    VkDevice         —— 逻辑设备，你创建、你销毁
    //    VkQueue          —— 提交命令的通道，随 device 隐式销毁
    // -----------------------------------------------------------------------
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    // D4 新增：窗口表面（VK_KHR_surface 实例扩展提供的平台无关表面）
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    // D5 新增：交换链 + 图像视图（成员先占位，本单元 createSwapChain / createImageViews 填充）
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;   // 交换链本身
    std::vector<VkImage> swapChainImages;         // 交换链里的图像句柄（驱动创建，不销毁）
    VkFormat swapChainImageFormat;                // 选中的像素格式
    VkExtent2D swapChainExtent;                   // 选中的分辨率
    std::vector<VkImageView> swapChainImageViews; // 每个图像一个 view（你创建，要销毁）

    // D6 新增：着色器模块（SPIR-V 字节码容器）。
    // 为什么不放局部变量？因为 D6 还没建管线（D10 才 vkCreateGraphicsPipelines），
    // 模块得从 D6 活到 D10，所以先做成成员、在 cleanup() 里销毁。
    // （D10 会改成"建完管线立刻销毁"，讲义第 4 节"可以立刻销毁"讲的就是那个优化。）
    VkShaderModule vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule fragShaderModule = VK_NULL_HANDLE;

    // 队列族索引集合。为什么用 std::optional 而不是魔法值？
    // 一个合法的图形队列族索引可能是 0，所以 0 / -1 / UINT32_MAX 都不能当"无效值"。
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;   // D5 新增：呈现（present）队列族

        bool isComplete() {
            // TODO(ch05)：改成 graphicsFamily.has_value() && presentFamily.has_value()
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    // -----------------------------------------------------------------------
    //  D4 新增：交换链能力的查询结果（讲义第 4 节）
    //    注意三种返回风格：capabilities 是单个结构体，formats / presentModes 是列表。
    // -----------------------------------------------------------------------
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;         // 基本能力（图像数量/尺寸范围…）
        std::vector<VkSurfaceFormatKHR> formats;       // 支持哪些像素格式
        std::vector<VkPresentModeKHR> presentModes;    // 支持哪些呈现模式
    };

    // -----------------------------------------------------------------------
    //  窗口
    // -----------------------------------------------------------------------
    //  TODO(ch02):
    //    1. glfwInit()
    //    2. glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)   ← 不要创建 OpenGL context
    //    3. glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE)     ← 窗口缩放 D12 才处理
    //    4. window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr)
    //
    //  想一想：为什么必须关掉 GLFW_RESIZABLE？如果不关，缩放窗口时会发生什么？
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    // -----------------------------------------------------------------------
    //  Vulkan 初始化：之后每一章都会往这里加一个 createXxx()
    // -----------------------------------------------------------------------
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();         // D4：给 GLFW 窗口挂一个 Vulkan 表面 surface
        pickPhysicalDevice();    // D3：从系统枚举并挑一块 GPU → physicalDevice
        createLogicalDevice();   // D3：建立逻辑设备会话 → device + graphicsQueue
        createSwapChain();       // D5：创建交换链，取出图像句柄
        createImageViews();      // D5：为每张图像建一个 view
        createGraphicsPipeline();// D6：读 .spv、建着色器模块、填两个 stage info
    }

    void mainLoop() {
        // ⚠️ 骨架专用保护，不是教程内容。
        //
        // 如果 initWindow() 还没实现，window 就是 nullptr。此时直接进下面的循环，
        // glfwWindowShouldClose(nullptr) 会触发 GLFW 内部 window.c 的 assert，
        // 在 Debug 构建下【弹出模态对话框把程序彻底挂住】—— 没有输出、没有报错、
        // 终端不返回，最难排查的一种失败。
        //
        // 这几行把它变成一条明确的异常信息。实现完 initWindow() 后可以删掉（留着也无害）。
        if (window == nullptr) {
            throw std::runtime_error(
                "window 是 nullptr —— 你还没实现 initWindow()。"
                "见 docs/ch02-steps.md 的检查点 1。");
        }

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    // -----------------------------------------------------------------------
    //  实例创建 —— 本单元的核心
    // -----------------------------------------------------------------------
    //  TODO(ch02):（讲义第 3、4、8 节）
    //
    //  步骤：
    //    a) if (enableValidationLayers && !checkValidationLayerSupport()) throw ...
    //    b) 填 VkApplicationInfo      —— 注意 {} 值初始化，别漏 sType
    //    c) 填 VkInstanceCreateInfo   —— pApplicationInfo 指向上面的 appInfo
    //    d) 拿扩展列表：auto extensions = getRequiredExtensions();
    //       填入 enabledExtensionCount / ppEnabledExtensionNames
    //    e) 层 + pNext：
    //         VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    //         （必须定义在 if 外面！讲义第 8 节讲了为什么）
    //         if (enableValidationLayers) { 填层名; populateDebugMessengerCreateInfo(debugCreateInfo);
    //                                       createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo; }
    //         else                       { createInfo.enabledLayerCount = 0; createInfo.pNext = nullptr; }
    //    f) vkCreateInstance(&createInfo, nullptr, &instance)，失败就 throw std::runtime_error
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // -----------------------------------------------------------------------
    //  扩展列表
    // -----------------------------------------------------------------------
    //  TODO(ch02):
    //    1. glfwGetRequiredInstanceExtensions(&glfwExtensionCount)
    //    2. 用 (glfwExtensions, glfwExtensions + glfwExtensionCount) 构造 std::vector
    //    3. enableValidationLayers 为真时 push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
    //       用这个宏，不要手写字符串，否则拼错了验证层只会给你一个含糊的错误
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    // -----------------------------------------------------------------------
    //  验证层可用性检查
    // -----------------------------------------------------------------------
    //  TODO(ch02):（讲义第 5 节 —— 两段式枚举模式，本教程最常复用的模式）
    //    1. 第一段：vkEnumerateInstanceLayerProperties(&layerCount, nullptr)
    //    2. 第二段：构造 vector 后再调一次填数据
    //    3. 对 validationLayers 里每个名字，在 availableLayers 里 strcmp 查找
    //       有一个找不到就 return false
    bool checkValidationLayerSupport() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    // -----------------------------------------------------------------------
    //  调试信使
    // -----------------------------------------------------------------------
    //  TODO(ch02):
    //    1. if (!enableValidationLayers) return;
    //    2. VkDebugUtilsMessengerCreateInfoEXT createInfo;
    //       populateDebugMessengerCreateInfo(createInfo);
    //    3. 调用你自己写的 CreateDebugUtilsMessengerEXT，不是 VK_SUCCESS 就 throw
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // -----------------------------------------------------------------------
    //  填充 messenger 创建信息（被 createInstance 和 setupDebugMessenger 共用）
    // -----------------------------------------------------------------------
    //  TODO(ch02):（讲义第 6 节）
    //    - createInfo = {};   ← 先清零，因为会被复用
    //    - sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
    //    - messageSeverity：教程用 VERBOSE | WARNING | ERROR（故意不含 INFO）
    //                       你可以按自己口味调，但至少要有 WARNING | ERROR
    //    - messageType    ：GENERAL | VALIDATION | PERFORMANCE
    //    - pfnUserCallback = debugCallback
    //    - pUserData      可选，教程传 nullptr
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    // -----------------------------------------------------------------------
    //  验证层消息回调
    //
    //  ⚠️ 签名不能改。VKAPI_ATTR / VKAPI_CALL 是调用约定要求，参数类型和顺序
    //     必须与 PFN_vkDebugUtilsMessengerCallbackEXT 完全一致。
    // -----------------------------------------------------------------------
    //  TODO(ch02):（讲义第 6 节）
    //    - 把 pCallbackData->pMessage 打出来
    //    - 建议按等级分流：WARNING 及以上走 std::cerr，其余忽略或走 std::cout
    //    - 永远 return VK_FALSE
    //    - 进阶：用 pUserData 传 this 指针进来，这样回调里可以访问类成员
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    // -----------------------------------------------------------------------
    //  窗口表面（D4）
    // -----------------------------------------------------------------------
    //  TODO(ch04):（讲义第 2 节）
    //    一行调用：
    //      if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    //          throw std::runtime_error("failed to create window surface!");
    //
    //  这行在 Windows 上背后实际执行的是 vkCreateWin32SurfaceKHR（讲义第 2 节有展开），
    //  GLFW 帮你把平台差异藏起来了。VK_KHR_surface / VK_KHR_win32_surface 这两个
    //  实例扩展早在 D2 的 getRequiredExtensions() 里就启用了，现在知道它们是干啥的了。
    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    // -----------------------------------------------------------------------
    //  挑选物理设备
    // -----------------------------------------------------------------------
    //  TODO(ch03):（讲义第 2、3 节 —— 又见两段式枚举 + 评分版选择）
    //    1. 第一段：vkEnumeratePhysicalDevices(instance, &count, nullptr)
    //       count == 0 直接 throw（"没有支持 Vulkan 的 GPU"）
    //    2. 第二段：构造 vector 再调一次填数据
    //    3. 用 std::multimap<int, VkPhysicalDevice> 存 {分数, 设备}，rateDeviceSuitability 打分
    //       —— 它天然按分数排序，rbegin() 就是最高分
    //    4. 取 rbegin()：若分数 <= 0 淘汰；否则 physicalDevice = 第二字段的设备
    //    5. 建议顺手打印选中的设备名（讲义第 9 节的实验），props.deviceName
    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::multimap<int, VkPhysicalDevice> candidates;
        for (const auto& device : devices) {
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }

        // 取最高分的设备。⚠️ 分数必须 > 0：0 分 = 不满足硬性要求（如缺 geometryShader），
        // 即使它是唯一一块设备也要淘汰 —— 这就是评分版 vs"挑第一个" 的本质区别。
        //
        // TODO(ch04)：把下面的 if 条件改成
        //     if (candidates.rbegin()->first > 0 && isDeviceSuitable(candidates.rbegin()->second))
        //  分数只是「排名」，isDeviceSuitable 才是「硬性资格」（图形队列族 + 设备扩展 + 交换链能力）。
        //  不加上这个 &&，isDeviceSuitable 在 D4 长出来的检查就是死代码，永远用不上。
        if (candidates.rbegin()->first > 0 && isDeviceSuitable(candidates.rbegin()->second)) {
            physicalDevice = candidates.rbegin()->second;
        } else {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    // -----------------------------------------------------------------------
    //  设备评分（教程推荐的可选做法）
    // -----------------------------------------------------------------------
    //  TODO(ch03):（讲义第 3 节）
    //    - vkGetPhysicalDeviceProperties 拿 props，vkGetPhysicalDeviceFeatures 拿 features
    //    - DISCRETE_GPU（独立显卡）加分 +1000
    //    - 加上 props.limits.maxImageDimension2D（最大纹理尺寸）
    //    - features.geometryShader == false 直接返回 0（硬性要求，淘汰）
    int rateDeviceSuitability(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(device, &props);
        vkGetPhysicalDeviceFeatures(device, &features);

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }
        score += props.limits.maxImageDimension2D;

        if (!features.geometryShader) {
            return 0;
        }

        return score;
    }

    // -----------------------------------------------------------------------
    //  设备是否合适
    // -----------------------------------------------------------------------
    //  TODO(ch03):（讲义第 3 节）
    //    先 findQueueFamilies(device) 拿 indices，然后 return indices.isComplete();
    //    —— 这个函数会一路长大到 D19（见讲义第 3 节的表格），现在只检查图形队列族
    bool isDeviceSuitable(VkPhysicalDevice device) {
        auto indices = findQueueFamilies(device);

        // TODO(ch04):（讲义第 4 节 —— isDeviceSuitable 第三次长大）
        //   1. bool extensionsSupported = checkDeviceExtensionSupport(device);
        //   2. bool swapChainAdequate = false;
        //      if (extensionsSupported) {
        //          // ⚠️ 这个 if 不是可选的：不支持的设备上直接查会触发验证层报错
        //          SwapChainSupportDetails details = querySwapChainSupport(device);
        //          swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
        //      }
        //   3. return indices.isComplete() && extensionsSupported && swapChainAdequate;

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails details = querySwapChainSupport(device);
            swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    // -----------------------------------------------------------------------
    //  查找队列族
    // -----------------------------------------------------------------------
    //  TODO(ch03):（讲义第 4 节 —— 第一次用 std::optional）
    //    1. QueueFamilyIndices indices; 先用空 optional 占位
    //    2. 两段式枚举 vkGetPhysicalDeviceQueueFamilyProperties
    //    3. 遍历时逐个判断：queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT
    //       命中就 indices.graphicsFamily = i;
    //    4. ⚠️ break 的位置要抄对（讲义第 8 节坑 #4）：
    //       别在赋值前 break，也别漏 break 导致覆盖
    //    5. return indices;
    //
    //  TODO(ch05):（讲义第 3.3 节的伏笔兑现 —— 加 presentFamily）
    //    ① 删掉上面那个 break（现在要遍历完整个列表，两个族都要找）
    //    ② 循环里再加一段：
    //         VkBool32 presentSupport = false;
    //         vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    //         if (presentSupport) indices.presentFamily = i;
    //    ⚠️ 你的机器上 graphicsFamily == presentFamily == 0（同一个族），
    //       两者会命中同一个 i，最终走 EXCLUSIVE 分支（见 docs/your-gpu.md）。
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }

        return indices;
    }

    // -----------------------------------------------------------------------
    //  检查设备扩展支持（D4）
    // -----------------------------------------------------------------------
    //  TODO(ch04):（讲义第 3 节 —— 用 std::set 求差集）
    //    1. 两段式枚举 vkEnumerateDeviceExtensionProperties（第三参数传 nullptr 表示
    //       "设备本身的扩展"，不看任何 layer）
    //    2. 把 deviceExtensions（我需要的）塞进 std::set<std::string>
    //    3. 遍历 availableExtensions，用 erase 逐个删掉"实际有"的
    //    4. return requiredExtensions.empty();   // 空了 = 全都有
    //
    //  对照 D2 的 checkValidationLayerSupport 用嵌套 strcmp —— 两种都行，这只是更优雅的写法。
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

    // -----------------------------------------------------------------------
    //  查询交换链能力（D4 核心）
    // -----------------------------------------------------------------------
    //  TODO(ch04):（讲义第 4 节 —— 三次查询，两种返回风格）
    //    1. vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities)
    //       —— 单个结构体，直接传地址一次调用
    //    2. vkGetPhysicalDeviceSurfaceFormatsKHR       —— 列表，两段式
    //    3. vkGetPhysicalDeviceSurfacePresentModesKHR  —— 列表，两段式
    //    两个列表的 ⚠️ if (count != 0) 守卫别漏：列表可能为空（讲义第 4 节）。
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    // -----------------------------------------------------------------------
    //  挑选表面像素格式（D4 —— "查询-选择"套路范例）
    // -----------------------------------------------------------------------
    //  TODO(ch04):（讲义第 5.1 节）
    //    遍历 availableFormats，命中 VK_FORMAT_B8G8R8A8_SRGB + VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    //    直接返回；否则 return availableFormats[0]（兜底）。
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) { 
        VkSurfaceFormatKHR bestFormat = availableFormats[0];
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                bestFormat = availableFormat;
                break;
            }
        }
        return bestFormat;
    }

    // -----------------------------------------------------------------------
    //  挑选呈现模式（D4 —— 最重要的设置之一）
    // -----------------------------------------------------------------------
    //  TODO(ch04):（讲义第 5.2 节）
    //    优先 VK_PRESENT_MODE_MAILBOX_KHR（三重缓冲式，低延迟且不撕裂），
    //    没有就 return VK_PRESENT_MODE_FIFO_KHR（规范保证一定存在 → 兜底）。
    //  ⚠️ 你的 Intel UHD 630 不支持 MAILBOX，所以最终会拿到 FIFO —— 这是正常的，
    //     见 docs/your-gpu.md 第 1 条。
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes) {
        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto& availableMode : availablePresentModes) {
            if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                bestMode = availableMode;
                break;
            }
        }
        return bestMode;
    }

    // -----------------------------------------------------------------------
    //  挑选交换链分辨率（D4 —— 高 DPI 的坑在这里）
    // -----------------------------------------------------------------------
    //  TODO(ch04):（讲义第 5.3 节）
    //    1. if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    //         return capabilities.currentExtent;    // 驱动已算好，直接用
    //    2. else：
    //         int width, height;
    //         glfwGetFramebufferSize(window, &width, &height);   // ← 像素，不是屏幕坐标！
    //         VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
    //         actualExtent.width  = std::clamp(actualExtent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
    //         actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    //         return actualExtent;
    //  ⚠️ 你的机器 currentExtent 是固定 800×600（不是 UINT32_MAX），所以走 if 分支，
    //     else 分支不会执行 —— 但仍要写对（可移植性，见 docs/your-gpu.md 第 2 条）。
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // -----------------------------------------------------------------------
    //  创建逻辑设备（四个结构体 + 一个生命周期陷阱）
    // -----------------------------------------------------------------------
    //  TODO(ch03):（讲义第 5、6 节）
    //    1. queueIndices = findQueueFamilies(physicalDevice); 只用一个队列族，count=1
    //    2. VkDeviceQueueCreateInfo queueCreateInfo{};
    //       - sType / queueFamilyIndex = queueIndices.graphicsFamily.value()
    //       - queueCount = 1
    //       - ⚠️ float queuePriority = 1.0f; 必须定义在 vkCreateDevice 之前、
    //         且离开作用域要在 vkCreateDevice 之后（生命周期陷阱，和 D2 debugCreateInfo 同类）
    //       - pQueuePriorities = &queuePriority;
    //    3. VkPhysicalDeviceFeatures deviceFeatures{};  ← 现在全 false，全空
    //    4. VkDeviceCreateInfo createInfo{};
    //       - pQueueCreateInfos = &queueCreateInfo; queueCreateInfoCount = 1
    //       - pEnabledFeatures = &deviceFeatures;
    //       - 层字段：if (enableValidationLayers) 填层，else count=0（历史遗留，照设即可）
    //       - enabledExtensionCount = 0   ← D5 会换成 VK_KHR_swapchain
    //    5. vkCreateDevice(physicalDevice, &createInfo, nullptr, &device)
    //    6. vkGetDeviceQueue(device, queueIndices.graphicsFamily.value(), 0, &graphicsQueue)
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);   // 只查一次，两处共用

        float queuePriority = 1.0f;

        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;

        // TODO(ch04):（讲义第 3 节）
        //   ① 启用设备扩展：把下面这一行
        //        createInfo.enabledExtensionCount = 0;
        //      换成
        //        createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
        //        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        //      ⚠️ 这是 D4 最容易忘的一步：忘了，D5 的 vkCreateSwapchainKHR 会报
        //      VK_ERROR_EXTENSION_NOT_PRESENT。
        //   ② 顺手删掉下面这段「设备的验证层字段」（if (enableValidationLayers) {...} else {...}）：
        //      Device Layer 从 Vulkan 1.0 起就从未生效过，留着只会触发验证层警告
        //      （你 D3 跑起来应该看到过 "enabledLayerCount is 1 (not zero)"）。见 D3 自检第 8 题。
        //      删掉后，createInfo 里的 enabledLayerCount 保持 {}-初始化的 0 即可。

        createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }

    // -----------------------------------------------------------------------
    //  创建交换链（D5 —— 全教程最大的一个创建结构体）
    // -----------------------------------------------------------------------
    //  TODO(ch05):（讲义第 3 节）
    //    1. SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    //    2. 用三个 choose* 定下来：
    //         VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    //         VkPresentModeKHR   presentMode   = chooseSwapPresentMode(swapChainSupport.presentModes);
    //         VkExtent2D         extent        = chooseSwapExtent(swapChainSupport.capabilities);
    //    3. imageCount = minImageCount + 1；若 maxImageCount > 0 且超了，就夹回 maxImageCount
    //       （⚠️ maxImageCount == 0 表示"无上限"，所以必须先判断 > 0 —— 讲义第 3.1 节）
    //    4. 填 VkSwapchainCreateInfoKHR{}：
    //         sType / surface / minImageCount / imageFormat = surfaceFormat.format /
    //         imageColorSpace = surfaceFormat.colorSpace / imageExtent = extent /
    //         imageArrayLayers = 1 / imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT /
    //         preTransform = capabilities.currentTransform /
    //         compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR /
    //         presentMode / clipped = VK_TRUE / oldSwapchain = VK_NULL_HANDLE
    //    5. imageSharingMode：先 QueueFamilyIndices indices = findQueueFamilies(physicalDevice)，
    //         graphicsFamily != presentFamily → CONCURRENT（count=2 + 两个族索引）
    //         否则 → EXCLUSIVE（count=0 + nullptr）
    //    6. vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain)，失败 throw
    //    7. 两段式 vkGetSwapchainImagesKHR 取图像句柄 → swapChainImages
    //    8. 存下 swapChainImageFormat = surfaceFormat.format;  swapChainExtent = extent;
    //    ⚠️ 你的机器 currentExtent 固定 800×600、graphics==present，
    //       会直接命中 chooseSwapExtent / EXCLUSIVE 这两个最简分支。
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        uint32_t swapChainImageCount;
        vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
        swapChainImages.resize(swapChainImageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    // -----------------------------------------------------------------------
    //  创建图像视图（D5 —— 第一次接触 subresourceRange）
    // -----------------------------------------------------------------------
    //  TODO(ch05):（讲义第 4 节）
    //    为每张 swapChainImages[i] 建一个 VkImageView，填 VkImageViewCreateInfo{}：
    //      sType / image = swapChainImages[i] / viewType = VK_IMAGE_VIEW_TYPE_2D /
    //      format = swapChainImageFormat
    //      components 四个通道全 VK_COMPONENT_SWIZZLE_IDENTITY
    //      subresourceRange:
    //        aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT
    //        baseMipLevel   = 0
    //        levelCount     = 1        ⚠️ 别写成 0（讲义第 4.2 节的经典错误）
    //        baseArrayLayer = 0
    //        layerCount     = 1        ⚠️ 别写成 0
    //    然后 vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i])，失败 throw
    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    // -----------------------------------------------------------------------
    //  读着色器文件（D6 —— 二进制读取，不是文本）
    // -----------------------------------------------------------------------
    //  TODO(ch06):（讲义第 2 节 / 教程「着色器模块」一节的 readFile）
    //    1. std::ifstream file(std::string(SHADER_DIR) + "/" + filename,
    //                         std::ios::ate | std::ios::binary);
    //       - SHADER_DIR 是编译期宏（CMakeLists.txt 里定义），指向 build/shaders/，
    //         所以这里只用传文件名，不用关心当前工作目录。
    //       - ios::ate：打开后定位到文件末尾 → tellg() 直接拿到文件大小
    //       - ios::binary：.spv 是二进制，Windows 上不加 binary 会吞 0x1A（EOF 字符）
    //    2. if (!file.is_open()) throw std::runtime_error("failed to open file: " + filename);
    //    3. size_t fileSize = (size_t)file.tellg();
    //    4. std::vector<char> buffer(fileSize);
    //    5. file.seekg(0);                        ← 回开头再读
    //    6. file.read(buffer.data(), fileSize);   ← 一次性读入
    //    7. file.close(); return buffer;
    static std::vector<char> readFile(const std::string& filename) {
        // TODO(ch06): 实现上面的步骤
        throw std::runtime_error("TODO(ch06): readFile 未实现");
    }

    // -----------------------------------------------------------------------
    //  创建着色器模块（D6 —— VkShaderModule 只是字节码容器）
    // -----------------------------------------------------------------------
    //  TODO(ch06):（讲义第 4 节「createShaderModule() 的两个坑」）
    //    1. VkShaderModuleCreateInfo createInfo{};
    //       createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    //       createInfo.codeSize = code.size();
    //       createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());   // ← 坑 1
    //    ⚠️ 坑 1：pCode 是 const uint32_t*（SPIR-V 按 4 字节存），必须 reinterpret_cast
    //       （不能直接传 const char*）。std::vector<char> 的对齐满足要求，所以安全。
    //    ⚠️ 坑 2：codeSize 必须是 4 的倍数。glslc 生成的 .spv 一定满足，
    //       但文件损坏/截断时这一行会返回错误或崩溃。
    //    2. VkShaderModule shaderModule;
    //       if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    //           throw std::runtime_error("failed to create shader module!");
    //    3. return shaderModule;
    VkShaderModule createShaderModule(const std::vector<char>& code) {
        // TODO(ch06): 实现上面的步骤
        throw std::runtime_error("TODO(ch06): createShaderModule 未实现");
    }

    // -----------------------------------------------------------------------
    //  创建图形管线（D6 只写「着色器阶段」这一小块 —— 完整管线 D10 才建）
    // -----------------------------------------------------------------------
    //  TODO(ch06):（讲义第 0 节「本单元你要亲手写的」+ 第 4 节）
    //    1. 读两个 .spv（文件名就是 build/shaders/ 下的产物名）：
    //         auto vertShaderCode = readFile("shader.vert.spv");
    //         auto fragShaderCode = readFile("shader.frag.spv");
    //    2. 用它们创建模块，存进成员（cleanup 里要销毁）：
    //         vertShaderModule = createShaderModule(vertShaderCode);
    //         fragShaderModule = createShaderModule(fragShaderCode);
    //    3. 填两个 VkPipelineShaderStageCreateInfo{}：
    //         - vert：sType / stage = VK_SHADER_STAGE_VERTEX_BIT   / module = vertShaderModule / pName = "main"
    //         - frag：sType / stage = VK_SHADER_STAGE_FRAGMENT_BIT / module = fragShaderModule / pName = "main"
    //       ⚠️ 坑 3：pName 必须传 "main"（入口函数名），传 nullptr 到 D10 会崩。
    //
    //    ⚠️ 本函数到此为止 —— 不调用 vkCreateGraphicsPipelines。
    //       VkPipelineLayout 在 D8、vkCreateGraphicsPipelines 在 D10。
    //       两个 stage info 填好后还没人用，D10 会接着往下写。
    void createGraphicsPipeline() {
        // TODO(ch06): 实现上面的步骤（只写着色器阶段这一小块，别往下写）
    }

    // -----------------------------------------------------------------------
    //  清理
    // -----------------------------------------------------------------------
    //  TODO(ch03): 在 TODO(ch02) 基础上，最前面加一行 vkDestroyDevice
    //  逆序：device → debugMessenger（仅 enableValidationLayers 时）→ instance → window → glfwTerminate
    //
    //  ⚠️ 讲义第 10 节要求：故意把 vkDestroyDevice 注释掉跑一次，
    //     看验证层怎么报 VkDevice 资源泄漏。
    //
    //  TODO(ch05):（讲义第 5 节 —— 交换链的逆序销毁）
    //    在 vkDestroyDevice 之前加：
    //      for (auto imageView : swapChainImageViews) vkDestroyImageView(device, imageView, nullptr);
    //      vkDestroySwapchainKHR(device, swapChain, nullptr);
    //    ⚠️ imageView 循环必须写在 vkDestroySwapchainKHR 之前
    //      （view 依赖交换链里的 image，后销毁 exchange 链会破坏 view）。
    //
    //  TODO(ch06):（讲义第 4 节「可以立刻销毁」+ 第 7 节）
    //    在 vkDestroyDevice 之前加：
    //      vkDestroyShaderModule(device, vertShaderModule, nullptr);
    //      vkDestroyShaderModule(device, fragShaderModule, nullptr);
    //    ⚠️ 顺序：shader module 属于 device，必须在 vkDestroyDevice 之前销毁。
    //       这里是 D6 的「临时位置」；D10 建完管线会改成「立刻销毁」，届时删掉这里。
    void cleanup() {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, swapChain, nullptr);

        // TODO(ch06): 在这里加两个 vkDestroyShaderModule（见上方注释）

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
};

// ===========================================================================
int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

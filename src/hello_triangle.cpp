// ===========================================================================
//  D2：基础代码 + 实例 + 验证层
//
//  教程章节：
//    Drawing_a_triangle/Setup/Base_code
//    Drawing_a_triangle/Setup/Instance
//    Drawing_a_triangle/Setup/Validation_layers
//
//  讲义  ：docs/ch02-instance-and-validation.md   ← 动手前先读一遍
//  自检题：docs/ch02-check.md
//
//  ---------------------------------------------------------------------------
//  工作方式
//  ---------------------------------------------------------------------------
//  所有标着  // TODO(ch02)  的地方由你来实现。
//  类结构、函数签名、成员变量、常量、include 已经给好 —— 请不要改签名
//  （尤其是 debugCallback，它的签名是 ABI 要求，改了会崩）。
//
//  写完：
//      .\scripts\build.ps1
//      .\scripts\run.ps1
//
//  本单元的验收标准见讲义第 11 节。核心三条：
//    1. 窗口正常出现，关闭后返回 0
//    2. 控制台能看到验证层输出
//    3. 完成"故意不销毁 debug messenger"实验并读懂报错
// ===========================================================================

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
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
    //  清理
    // -----------------------------------------------------------------------
    //  TODO(ch02): 逆序销毁
    //    debugMessenger（仅 enableValidationLayers 时）→ instance → window → glfwTerminate
    //
    //  ⚠️ 教程"测试"一节要求你先把销毁 debugMessenger 那两行注释掉跑一次，
    //     亲眼看到验证层报 "destroyed after VkInstance" 之类的错误，然后再改回来。
    //     这一步别跳过 —— 它是你学会读验证层报错的起点。
    void cleanup() {
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

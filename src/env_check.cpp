// ===========================================================================
//  D1：环境自检（不是教程内容，是本工程自带的工具）
//
//  一口气跑完，输出一份自检报告，直接告诉你 SETUP.md 里的检查项是否全部通过。
//
//  什么时候重跑它：
//    - 换机器 / 升级显卡驱动之后
//    - 验证层突然不报错了、或者程序行为异常，怀疑环境变了
//    - 想确认某个 Vulkan 扩展/层在当前机器上到底有没有
//
//  它独立于教程主程序（src/hello_triangle.cpp），永远不会被教程代码覆盖。
//
//  运行方式：
//      .\scripts\build.ps1
//      .\scripts\run.ps1 -Target env_check
// ===========================================================================

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// GLM 线性代数。
//
// ⚠️ 下面这个宏是 Vulkan 必需的：Vulkan 的 NDC 深度范围是 [0, 1]，OpenGL 是 [-1, 1]。
// 不定义它，glm::perspective 会按 OpenGL 约定生成投影矩阵，表现是物体被深度测试
// 莫名其妙裁掉，且极难排查。这是 OpenGL 转 Vulkan 的经典坑之一。
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 注：教程《开发环境》章节还写了 #define GLM_FORCE_RADIANS。
// 从 GLM 0.9.9 起「弧度」已经是唯一行为，GLM 1.0 已把该宏移除。
// 本工程用 GLM 1.0.1，故意不定义它 —— 定义了轻则毫无作用，重则触发编译期报错。

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// 一个极简的自检报告器：把"检查项 + 结果 + 细节"打印成人能一眼看懂的样子
// ---------------------------------------------------------------------------
class Report {
public:
    void section(const std::string& title) {
        std::cout << "\n== " << title << " ==\n";
    }

    void check(bool pass, const std::string& name, const std::string& detail = "") {
        std::cout << (pass ? "  [通过] " : "  [失败] ") << name;
        if (!detail.empty()) std::cout << "   (" << detail << ")";
        std::cout << "\n";
        allPassed_ = allPassed_ && pass;
    }

    void info(const std::string& text) {
        std::cout << "         " << text << "\n";
    }

    bool allPassed() const { return allPassed_; }

private:
    bool allPassed_ = true;
};

void glfwErrorCallback(int code, const char* description) {
    std::cerr << "[GLFW 错误 " << code << "] " << description << "\n";
}

void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

std::string versionString(uint32_t version) {
    return std::to_string(VK_VERSION_MAJOR(version)) + "." +
           std::to_string(VK_VERSION_MINOR(version)) + "." +
           std::to_string(VK_VERSION_PATCH(version));
}

// ---------------------------------------------------------------------------
// 检查 1：GLFW 与 Vulkan loader
// ---------------------------------------------------------------------------
void checkLoader(Report& report) {
    report.section("Vulkan Loader");

    uint32_t loaderVersion = 0;
    const VkResult result = vkEnumerateInstanceVersion(&loaderVersion);
    const bool ok = (result == VK_SUCCESS && loaderVersion > 0);
    report.check(ok, "vkEnumerateInstanceVersion 可用",
                 ok ? "loader 版本 " + versionString(loaderVersion) : "loader 缺失或过旧");

    if (loaderVersion < VK_API_VERSION_1_1) {
        report.info("注意：Vulkan 1.1 以下的 loader 无法使用 vkEnumerateInstanceVersion。");
    }
    report.info("loader 由显卡驱动提供，位于 C:\\WINDOWS\\System32\\vulkan-1.dll");
    report.info("它的作用等价于你 OpenGL 时代的 GLEW/GLAD，但由系统统一提供。");
}

// ---------------------------------------------------------------------------
// 检查 2：实例扩展（D2 创建 VkInstance 时必须显式列出）
// ---------------------------------------------------------------------------
void checkInstanceExtensions(Report& report) {
    report.section("Vulkan 实例扩展");

    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (count == 0) {
        report.check(false, "枚举实例扩展", "返回 0 个扩展，驱动可能有问题");
        return;
    }

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

    report.check(true, "枚举实例扩展", std::to_string(count) + " 个");

    bool hasDebugUtils = false;
    bool hasWin32Surface = false;
    for (const auto& ext : extensions) {
        report.info(std::string("  - ") + ext.name + "  (rev " + std::to_string(ext.specVersion) + ")");
        if (std::strcmp(ext.name, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) hasDebugUtils = true;
        if (std::strcmp(ext.name, "VK_KHR_win32_surface") == 0)          hasWin32Surface = true;
    }

    // 这两个扩展是 D2/D4 的硬需求，提前告诉你结果，省得到时候一脸茫然
    report.check(hasDebugUtils, "VK_EXT_debug_utils（D2 验证层输出需要它）");
    report.check(hasWin32Surface, "VK_KHR_win32_surface（D4 创建窗口表面需要它）");
}

// ---------------------------------------------------------------------------
// 检查 3：验证层 —— 学习 Vulkan 期间最重要的工具，必须装好
// ---------------------------------------------------------------------------
void checkValidationLayers(Report& report) {
    report.section("验证层 (Validation Layers)");

    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    std::vector<VkLayerProperties> layers(count);
    if (count > 0) {
        vkEnumerateInstanceLayerProperties(&count, layers.data());
    }

    bool hasKhronosValidation = false;
    for (const auto& layer : layers) {
        report.info(std::string("  - ") + layer.layerName + "  (" + layer.description + ")");
        if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            hasKhronosValidation = true;
        }
    }

    report.check(hasKhronosValidation,
                 "VK_LAYER_KHRONOS_validation 可用",
                 std::to_string(count) + " 个层");

    if (!hasKhronosValidation) {
        report.info(">>> 这说明 Vulkan SDK 还没装好，或者终端没重启（VULKAN_SDK 未生效）。");
        report.info(">>> 请回到 SETUP.md 步骤 2。验证层对学 Vulkan 是必需品，不要跳过。");
    } else {
        report.info("这是你未来两周最可靠的老师：它会精确指出哪个参数违规，");
        report.info("而 OpenGL 只会给你一个 glGetError 错误码或者干脆黑屏。");
    }
}

// ---------------------------------------------------------------------------
// 检查 4：GLM 线性代数（Vulkan 自己不带数学库，这点和 OpenGL 一样）
// ---------------------------------------------------------------------------
void checkGlm(Report& report) {
    report.section("GLM 线性代数");

    const float aspect = 800.0f / 600.0f;
    const glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10.0f);

    // 相机在原点看向 -Z，所以取一个位于 -Z 方向的点
    const glm::vec4 point(0.0f, 0.0f, -2.0f, 1.0f);
    const glm::vec4 clip = proj * point;

    report.check(std::abs(clip.w - 2.0f) < 1e-5f,
                 "perspective 投影矩阵计算正确",
                 "clip.w = " + std::to_string(clip.w) + "（期望 2.0）");

    // GLM_FORCE_DEPTH_ZERO_TO_ONE：近平面映射到 z=0，远平面映射到 z=1（Vulkan 约定）
    const glm::vec4 nearPoint = proj * glm::vec4(0.0f, 0.0f, -0.1f, 1.0f);
    const float ndcZ = nearPoint.z / nearPoint.w;
    report.check(std::abs(ndcZ) < 1e-3f,
                 "深度范围已按 Vulkan 约定映射到 [0, 1]",
                 "近平面 NDC z = " + std::to_string(ndcZ) + "（期望 ~0.0，OpenGL 约定会是 -1.0）");
}

} // namespace

// ===========================================================================
int main() {
    std::cout << "===============================================\n";
    std::cout << " Vulkan 学习环境自检  (D1)\n";
    std::cout << "===============================================\n";

    Report report;

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "\n[致命] glfwInit() 失败，无法继续。\n";
        return EXIT_FAILURE;
    }

    report.section("GLFW");
    report.check(true, "glfwInit 成功", glfwGetVersionString());
    report.info("GLFW 只负责窗口和输入。Vulkan 和 OpenGL 最大的区别之一：");
    report.info("OpenGL 的窗口创建会顺带创建 context，Vulkan 不会 —— 它只要一个 surface。");

    // 告诉 GLFW 不要创建任何图形 API 的 context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "D1 环境自检 - Vulkan", nullptr, nullptr);
    if (!window) {
        std::cerr << "\n[致命] 创建窗口失败。\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwSetKeyCallback(window, keyCallback);

    checkLoader(report);
    checkInstanceExtensions(report);
    checkValidationLayers(report);
    checkGlm(report);

    std::cout << "\n===============================================\n";
    if (report.allPassed()) {
        std::cout << " 结论：全部通过 ✅\n";
        std::cout << " 下一步：读 Overview 与 Graphics_pipeline_basics/Introduction，\n";
        std::cout << "         然后在 PROGRESS.md 给 D1 打勾，我们开始 D2。\n";
    } else {
        std::cout << " 结论：有检查项失败 ❌\n";
        std::cout << " 请对照 SETUP.md 的『常见失败』表处理，或把上面的输出贴给我。\n";
    }
    std::cout << "===============================================\n";
    std::cout << "\n（按 ESC 或关闭窗口退出）\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "已正常退出。\n";
    return report.allPassed() ? EXIT_SUCCESS : EXIT_FAILURE;
}

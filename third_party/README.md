# third_party/

GLFW 和 GLM 的**本地源码目录**。这个目录默认是空的 —— 空着的时候 CMake 会自动从
GitHub 拉取依赖（见根目录 `CMakeLists.txt`）。

## 为什么要有这个目录

教程的依赖（GLFW 3.4 / GLM 1.0.1）需要从 GitHub 拉取。**GitHub 在国内经常拉不动**，
`cmake` 配置阶段会卡住或报 `fatal: unable to access ...`。

CMake 会按下面的优先级自动选择来源，你不需要改任何代码：

| 优先级 | 条件 | 行为 |
|---|---|---|
| 1 | 目录 `third_party/glfw/CMakeLists.txt` 存在 | 直接用本地源码 |
| 2 | 传了 `-DGLFW_LOCAL_DIR=<路径>` | 用指定的源码目录 |
| 3 | 以上都没有 | 从 `GLFW_GIT_URL` 拉取（默认 GitHub） |

GLM 同理（`third_party/glm` / `-DGLM_LOCAL_DIR` / `GLM_GIT_URL`）。

---

## 方案 A：配代理（最省事，推荐先试）

如果你有本地代理（Clash / v2ray 等），让 git 走代理即可。常见的端口是 `7890`：

```powershell
git config --global http.proxy  http://127.0.0.1:7890
git config --global https.proxy http://127.0.0.1:7890

# 验证（能列出 3.4 这个分支/tag 就说明通了）
git ls-remote --heads https://github.com/glfw/glfw.git 3.4

# 用完想取消：
# git config --global --unset http.proxy
# git config --global --unset https.proxy
```

然后正常跑 `.\scripts\build.ps1 -Configure` 就行。

## 方案 B：换镜像源

不用改文件，配置时传 URL 即可：

```powershell
.\scripts\build.ps1 -Configure -GlfwUrl "https://gitee.com/<某个镜像>/glfw.git" `
                               -GlmUrl  "https://gitee.com/<某个镜像>/glm.git"
```

> ⚠️ 镜像的可用性我没法替你验证（第三方镜像经常失效、或者缺失某几个 tag）。
> 如果镜像里没有 `3.4` / `1.0.1` 这两个 tag，可以同时指定 tag：
> `-GlfwTag 3.4 -GlmTag 1.0.1`，或者换一个镜像试。
> **优先用方案 A 或 C**，镜像只作为备选。

## 方案 C：手动下载（最可靠，一定能成）

浏览器能上 GitHub 的话，直接下 ZIP：

1. GLFW：https://github.com/glfw/glfw/releases/tag/3.4
   下载 **Source code (zip)**，解压后把里面的目录**改名成 `glfw`**，放到 `third_party/glfw`
   （即 `third_party/glfw/CMakeLists.txt` 必须存在）

2. GLM：https://github.com/g-truc/glm/releases/tag/1.0.1
   同样下载 Source code (zip)，解压改名成 `glm`，放到 `third_party/glm`

最终结构：

```
third_party/
├─ glfw/
│  ├─ CMakeLists.txt     ← 必须在这个位置
│  ├─ include/
│  └─ src/
└─ glm/
   ├─ CMakeLists.txt
   └─ glm/
```

放好之后跑 `.\scripts\build.ps1 -Configure`，CMake 会打印
`依赖 glfw: 使用本地源码 ...`，说明生效了。

> 如果 GitHub 的 releases 页面也打不开，GLFW 官网
> https://www.glfw.org/download.html 提供 **64-bit Windows binaries** 预编译包 ——
> 但那个包没有 `CMakeLists.txt`，不能直接用方案 C 的方式喂给 CMake。
> 真遇到这种情况告诉我，我把 `CMakeLists.txt` 改成"链接预编译 lib"的写法。

---

## 这个目录要不要提交到 git？

`glfw/` 和 `glm/` 已经在 `.gitignore` 里排除了 —— 依赖源码不该进你的学习仓库。
`third_party/README.md`（本文件）会被提交。

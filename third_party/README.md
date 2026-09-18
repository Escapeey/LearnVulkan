# third_party/

GLFW 和 GLM 的**本地源码目录**。

## 当前状态：已就绪 ✅

```
third_party/
├─ glfw/    ← GLFW 3.4    已克隆，tag 3.4，含 CMakeLists.txt
├─ glm/     ← GLM 1.0.1   已克隆，tag 1.0.1，含 CMakeLists.txt
└─ README.md
```

两个依赖**已经在本地**，所以 `cmake` 配置阶段完全不需要联网 —— 构建不会再碰到网络问题。

> 这两个目录被 `.gitignore` 排除（依赖源码不该进学习仓库），但会留在你的磁盘上，
> 除非你手动删掉。删了也没关系，见下面的获取方式。

## CMake 怎么选来源

| 优先级 | 条件 | 行为 |
|---|---|---|
| 1 | `-DGLFW_LOCAL_DIR=<路径>` | 用指定的源码目录 |
| 2 | `third_party/glfw/CMakeLists.txt` 存在 | **用本地源码（当前就是这条）** |
| 3 | 以上都没有 | 从 `GLFW_GIT_URL` 拉取 |

`build.ps1` 每次都会打印实际用了哪条：

```
  依赖 glfw : 本地源码 third_party\glfw      ← 这条最好
  依赖 glfw : 将从 ... 拉取                   ← 需要联网
```

---

## ⚠️ 这台机器的实测网络情况

我在这台机器上实测过：

| 目标 | 结果 |
|---|---|
| `github.com:443` | ❌ **TCP 层连不上**（21 秒超时）—— GitHub 被墙 |
| `gitee.com` | ✅ 正常 |

所以**本工程默认的依赖源已经改成 Gitee 官方镜像**，不是 GitHub。

### 已验证可用的镜像

| 依赖 | 地址 | tag | 确认 |
|---|---|---|---|
| GLFW | `https://gitee.com/mirrors/glfw.git` | `3.4`（2024-02-22） | ✅ 已查 tag 列表实际存在 |
| GLM | `https://gitee.com/mirrors/glm.git` | `1.0.1`（2024-02-27） | ✅ 已查 tag 列表实际存在 |

`mirrors` 是 Gitee 官方的"极速下载"镜像组，一直在跟上游同步，比第三方个人镜像可靠。

> 另有一个 `gitee.com/snow-github-mirrors/glfw`，但最后推送停在 2023-02，
> **没有 GLFW 3.4**，别用那个。

---

## 需要重新获取依赖时

### 方案 A：重新克隆（命令我在这台机器上验证过能通）

```powershell
cd C:\Users\d00944037\Code\LearnVulkan
git clone --depth 1 --branch 3.4   https://gitee.com/mirrors/glfw.git third_party/glfw
git clone --depth 1 --branch 1.0.1 https://gitee.com/mirrors/glm.git  third_party/glm
```

然后 `.\scripts\build.ps1 -Configure`。

### 方案 B：让 CMake 自己拉（默认走 Gitee）

```powershell
.\scripts\build.ps1 -Configure
```

换源：

```powershell
.\scripts\build.ps1 -Configure -GlfwUrl "https://gitee.com/别处/glfw.git" `
                               -GlmUrl  "https://gitee.com/别处/glm.git"
```

### 方案 C：手动下载 ZIP

浏览器打开 https://gitee.com/mirrors/glfw →「克隆/下载」→ 下载 ZIP，
解压后改名成 `glfw` 放进 `third_party/`，确保 `third_party/glfw/CMakeLists.txt` 存在。GLM 同理。

---

## 排查：`schannel: AcquireCredentialsHandle failed: SEC_E_NO_CREDENTIALS`

看到这个错误说明 **git 的 schannel TLS 后端拿不到证书凭据**。
换成 git 自带的 OpenSSL 后端即可（这条我在你这台机器上验证过能通）：

```powershell
git -c http.sslBackend=openssl clone --depth 1 --branch 3.4 https://gitee.com/mirrors/glfw.git third_party/glfw
```

只影响 git；`vulkan-1.dll`、SDK 的 `glslc` 都不走这条路。

# 构建 / 运行工具问答

## 1. `build.ps1`、`run.ps1`、`run.ps1 -Target env_check` 有什么区别

### 问题

下面两个命令有什么区别？

```powershell
.\scripts\run.ps1                     # 跑教程主程序
.\scripts\run.ps1 -Target env_check   # 偶尔重跑环境自检
```

### 解答

三者分工不同，一句话：**`build.ps1` 负责编译，`run.ps1` 负责运行，`-Target` 只是换运行的目标 exe。**

| 命令 | 干什么 | 涉及的目标 |
|---|---|---|
| `.\scripts\build.ps1` | 配置 + 编译（生成 exe） | 只编译，不运行 |
| `.\scripts\run.ps1` | 运行教程主程序 | `learn_vulkan.exe`（默认） |
| `.\scripts\run.ps1 -Target env_check` | 运行环境自检 | `env_check.exe` |

关键点：

- `run.ps1` 的 `-Target` 参数默认值是 `learn_vulkan`（`run.ps1:22-23`），所以不写参数 = 跑主程序。
- 这两个 exe 都是 `build.ps1` 编译出来的**两个目标**（`build.ps1:186-191`）；`run.ps1` 本身**不编译**，只负责找已经编译好的 exe 并运行。
- 所以「改代码 → 先 `build.ps1` 编译 → 再 `run.ps1` 运行」是一套固定流程；光跑 `run.ps1` 不会带上你刚改的代码。

**一句话总结**：`build.ps1` 编译，`run.ps1` 运行；`-Target env_check` 只是把运行的目标从主程序换成环境自检。

> 相关内容：还有两套构建目录的概念（`build\` 权威 vs VS 的 `out\build\`），见主 `README.md`「构建方式」一节。
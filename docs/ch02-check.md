# D2 自检题

> 规则：**合上代码和教程**，先自己答。答不上来的题，回去看 `docs/ch02-instance-and-validation.md` 对应小节。
> 全部答得出，再把 D2 打勾。

---

### 1. 概念对照

`VkApplicationInfo` 在 OpenGL 里对应什么？

<details>
<summary>提示</summary>
如果答案是"没有对应物"，那要接着回答：OpenGL 为什么不需要它？是驱动不想要这个信息，还是没有渠道给？
</details>

---

### 2. 值初始化

为什么 Vulkan 的所有结构体都必须写成 `VkXxx info{}` 而不是 `VkXxx info;`？

<details>
<summary>提示</summary>
想想你只填了 6 个字段，但结构体有 10 个字段时，剩下 4 个是什么值。驱动会怎么用它们。
</details>

---

### 3. `sType` 与 `pNext`

1. `sType` 解决什么问题？（C 语言没有 RTTI，驱动拿到一个 `void*` 怎么知道它是什么？）
2. `pNext` 解决什么问题？（提示：**向后兼容**。如果你要给 `VkInstanceCreateInfo` 加个新字段，又不能改变它的内存布局，怎么办？）

---

### 4. 两段式枚举模式

写出这个模式的两步调用骨架（不用写具体函数名，用 `vkEnumerateXxx` 代替）：

```cpp
uint32_t count = 0;
// 第一步：？

// 第二步：？
```

**追问**：这个模式在本教程后面至少还会出现 7 次。说出其中 3 个你会用到它的场景。

---

### 5. 扩展函数

`vkCreateDebugUtilsMessengerEXT` 为什么不能像 `vkCreateInstance` 那样直接调用？

**追问（这题最重要）**：这和你在 OpenGL 里用 GLEW/GLAD 的经验是什么关系？Vulkan 为什么不像 OpenGL 那样提供 GLEW 的等价物？

---

### 6. `pNext` 生命周期（核心考点）

教程为什么要求 `debugCreateInfo` 定义在 `if` 语句**外面**？

把你的答案写成"如果写在里面会发生什么"的形式，要具体到内存层面。

---

### 7. 故障推演

`enableValidationLayers == true`，但用户没装 Vulkan SDK。程序会在**哪一行**、以什么方式失败？

<details>
<summary>提示</summary>
`checkValidationLayerSupport()` 返回什么？`createInstance()` 开头做了什么？抛出的是什么类型的异常？谁来 catch？
</details>

**追问**：如果 SDK 装了但终端没重启（`VK_LAYER_PATH` 没生效），现象一样吗？怎么快速区分这两种情况？

---

### 8. 生命周期图

画出 D2 结束时你拥有的所有需要手动销毁的对象，标出**正确的销毁顺序**，并说明为什么是这个顺序。

（提示：本单元只有 2 个 Vulkan 对象 + 1 个 GLFW 对象 + GLFW 库本身）

---

### 9. 验证层报错阅读

教程"测试"一节让你故意不销毁 debug messenger。请写出：

1. 你实际看到的报错原文（**必须是你自己跑出来的，不能从教程抄**）
2. 报错里的 `VUID-...` 是什么？（提示：去搜 "Vulkan VUID"）
3. 报错里的 `type = VK_OBJECT_TYPE_...` 告诉了你什么？

> 这题没有 `<details>` 提示。它要求你动手做那个实验。

---

### 10. 延伸思考（没有标准答案）

Vulkan 把错误检查从驱动里拿掉、放进可选的验证层。这个设计：

- 对**发布版游戏**有什么好处？
- 对**刚入门的学习者**有什么代价？
- 你现在理解为什么我在 `ROADMAP.md` 里说"验证层是你的老师，不是敌人"了吗？

---

## 答题记录

> 把答不上的题号记在这里，复习时重点看。

| 题号 | 第一次是否答对 | 需要复习的点 |
|---|---|---|
| 1 | | |
| 2 | | |
| 3 | | |
| 4 | | |
| 5 | | |
| 6 | | |
| 7 | | |
| 8 | | |
| 9 | | |
| 10 | | |

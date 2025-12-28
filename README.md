# 🎒 UE5 多人网络同步空间背包系统 (C++ 插件)

> **一个基于 Unreal Engine 5 C++ 开发的、支持服务器权威验证（Server-Authoritative）的空间网格背包系统。
> 采用了数据驱动架构与高效的网络增量同步方案。**

## 📖 项目简介

本项目实现了一套完整的背包后端与前端解决方案，模仿了类似《逃离塔科夫》或《暗黑破坏神》的复杂物品管理系统。

不同于简单的列表式背包，本插件完全基于 **C++** 编写，解决了**多尺寸物品的空间管理**（如 1x1, 2x3 格子）、**拖拽交互**以及**多人网络环境下的数据一致性**问题。
项目深度应用了 UE5 的现代特性（如 `StructUtils` 和 `Fast Array`），旨在展示高性能、模块化的系统设计能力。


## ✨ 核心功能

*   **🧩 空间网格系统 (Spatial Grid):**
    *   自主研发的二维空间搜索算法 (`HasRoomForItem`)，支持任意尺寸物品的自动堆叠与碰撞检测。
    *   基于 `CanvasPanel` 的自定义 UI 渲染逻辑，突破了原生 `UniformGridPanel` 的布局限制。
*   **🌐 权威服务器网络架构:**
    *   严格遵循 Server-Authoritative 模式，所有逻辑校验均在服务器完成，杜绝客户端作弊。
    *   客户端通过 RPC 请求交互（拾取、移动、丢弃、使用）。
    *   实现了复杂的 `UObject` 子对象网络复制 (`SubObject Replication`)。
*   **🧱 组合式数据架构 (Composition Pattern):**
    *   摒弃传统的深层继承树，采用 **Manifest + Fragment** 模式。
    *   利用 UE5 `StructUtils` (`FInstancedStruct`) 实现多态数据存储，极大地提高了物品属性的扩展性和内存连续性。
*   **⚔️ 装备与交互系统:**
    *   基于 `GameplayTags` 的类型过滤系统，支持头部、胸部、武器等专用装备栏。
    *   包含右键菜单（拆分堆叠、消耗物品）及动态属性面板（Composite UI 模式）。

---

## 🛠️ 技术架构深度解析 (面试重点)

### 1. 数据结构：组合优于继承
为了避免传统 RPG 项目中 Item 基类过于臃肿的问题，我设计了基于 **Fragment（碎片）** 的数据结构：
*   **实现方式**：物品本身是一个轻量级的 Manifest，内部包含一个 `FInstancedStruct` 数组。
*   **优势**：策划可以在编辑器中随意为物品添加“可堆叠”、“有网格大小”、“可装备”等特性模块，无需修改 C++ 代码，符合**数据驱动 (Data-Driven)** 设计理念。

### 2. 网络优化：Fast Array Serialization
考虑到背包可能包含上百个格子，普通的 `Replicated TArray` 会导致巨大的带宽浪费（只要变动一个元素就全量同步）。
*   **解决方案**：实现了 `FFastArraySerializer` (Fast Array)。
*   **效果**：系统只通过网络发送发生变化的数组元素（增量更新），并利用 `PostReplicatedAdd` 回调精准刷新客户端 UI，大幅降低了网络负载。

### 3. 空间搜索算法
核心的“俄罗斯方块”逻辑位于 `UInv_InventoryGrid` 中：
*   **逻辑**：通过将 2D 坐标映射为 1D 索引，结合位图思想检测区域占用。
*   **交互**：在 `NativeTick` 中实现了基于**象限 (Quadrant)** 的鼠标位置检测，确保了在不同分辨率下拖拽物品时的落点计算精确无误。

---

## 📁 代码结构说明

*   `Source/Public/Items`: 核心数据定义 (Manifest, Fragments, StructUtils集成)。
*   `Source/Public/InventoryManagement`: 组件逻辑、网络同步 (Fast Array) 实现。
*   `Source/Public/Widgets/Spatial`: 复杂的空间 UI 逻辑与算法实现。
*   `Source/Public/Interaction`: 玩家控制器与射线检测逻辑。

---

## 👨‍💻 关于开发者

**吴一凡**
*UE5 C++ 游戏开发工程师*

该项目旨在展示我在 **UE5 C++ 架构设计**、**网络同步编程**以及**复杂 Gameplay 系统实现**方面的能力。我对构建可扩展、高性能的游戏系统充满热情。

*   📧 邮箱: 1678310645@qq.com
*   🔗 个人主页/作品集: https://space.bilibili.com/319447466/upload/video
*   小工作室网站：https://asdfri.cn/ （在大学和志同道合的人组的开发游戏的工作室）

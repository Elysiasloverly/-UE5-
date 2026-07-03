# UE5 多人网络同步空间背包系统

基于 Unreal Engine 5 开发的 C++ 插件，实现了一套完整的 Server-Authoritative 空间网格背包系统，支持多尺寸物品管理、拖拽交互及多人网络环境下的数据一致性。

## 项目简介

本项目实现了类似《逃离塔科夫》或《暗黑破坏神》的复杂物品管理系统。不同于简单的列表式背包，本系统完全基于 C++ 构建，解决了多尺寸物品空间管理（如 1x1、2x3 格子）、拖拽交互以及多人网络环境下的数据同步问题。项目深度应用了 UE5 的现代特性（StructUtils、Fast Array、GameplayTags 等），展示高性能、模块化的系统设计能力。

## 技术栈

UE5 · C++ · StructUtils (FInstancedStruct) · Fast Array Serialization · GameplayTags · Slate / UMG · RPC · SubObject Replication

## 核心功能

### 空间网格系统

- 自研二维空间搜索算法（HasRoomForItem），支持任意尺寸物品的自动堆叠与碰撞检测
- 基于 CanvasPanel 的自定义 UI 渲染逻辑，突破原生 UniformGridPanel 的布局限制
- 基于象限的鼠标位置检测，确保不同分辨率下拖拽落点计算精确

### 权威服务器网络架构

- 严格遵循 Server-Authoritative 模式，所有逻辑校验在服务器完成，杜绝客户端作弊
- 客户端通过 RPC 请求交互（拾取、移动、丢弃、使用）
- 实现 UObject 子对象网络复制（SubObject Replication）

### 组合式数据架构

- 摒弃传统深层继承树，采用 Manifest + Fragment 模式
- 利用 StructUtils (FInstancedStruct) 实现多态数据存储，提高物品属性扩展性和内存连续性
- 策划可在编辑器中为物品自由添加"可堆叠"、"有网格大小"、"可装备"等特性模块，无需修改 C++ 代码，符合数据驱动设计理念

### 装备与交互系统

- 基于 GameplayTags 的类型过滤系统，支持头部、胸部、武器等专用装备栏
- 包含右键菜单（拆分堆叠、消耗物品）及动态属性面板（Composite UI 模式）

## 网络优化：Fast Array Serialization

考虑到背包可能包含上百个格子，普通的 Replicated TArray 会产生大量带宽浪费（变动一个元素就全量同步）。本系统实现了 FFastArraySerializer（Fast Array），只通过网络发送发生变化的数组元素（增量更新），并利用 PostReplicatedAdd 回调精准刷新客户端 UI，大幅降低网络负载。

## 代码结构

| 目录 | 职责 |
|------|------|
| `Source/Public/Items` | 核心数据定义（Manifest、Fragments、StructUtils 集成） |
| `Source/Public/InventoryManagement` | 组件逻辑、网络同步（Fast Array）实现 |
| `Source/Public/Widgets/Spatial` | 空间 UI 逻辑与搜索算法实现 |
| `Source/Public/Interaction` | 玩家控制器与射线检测逻辑 |
| `Source/Public/EquipmentManagement` | 装备系统组件与装备 Actor |

## 构建

1. 安装 Unreal Engine 5.7
2. 右键 `Inventory.uproject` -> Generate Visual Studio project files
3. 打开解决方案编译，或双击 `Inventory.uproject` 让 UE 自动编译

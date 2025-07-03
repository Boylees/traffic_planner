# 交通网络路径规划系统 (C)

> 多交通方式 · 地标级节点 · 最优/多点路径 · 可扩展数据源

---

## 1. 项目简介

本项目为个人实现的哈尔滨工业大学（威海）数据结构课程设计。
本系统使用纯 **C** 语言实现，模拟中国 50+ 城市的交通网络，支持 **驾车 / 公交 / 高铁 / 航空** 四种出行方式，为用户提供：

* 单点最优路径（支持时间/花费权衡）
* 多点旅行商 (TSP) 最优回路
* JSON & 人类可读双格式输出

> 未来计划：GIS 可视化 / 实时路况接入 / 高并发服务化。

---

## 2. 目录结构

```
├─include/               # 公共头文件
│   └─traffic_network.h  # 核心结构体 + 接口声明
├─src/                   # 源码实现
│   ├─data_loader.c      # CSV → 内存图加载 & 回退静态数据
│   ├─distance.c         # 哈弗辛公式
│   ├─pathfinding.c      # Dijkstra & Held-Karp TSP
│   └─main.c             # 菜单交互 / I/O
├─data/
│   └─nodes.csv          # 节点数据源（50 城 * N 节点）
├─bin/                   # 编译产物 (make 后生成)
├─Makefile               # 编译脚本
└─README.md
```

---

## 3. 快速开始

### 3.1 环境依赖

* **gcc ≥ 9**  (支持 C99)
* 任意 **POSIX / Windows** 终端

### 3.2 构建

```bash
# 克隆仓库后
make         # 生成 bin/traffic_planner.exe (Windows) 或无扩展名可执行文件
```

### 3.3 运行 & 交互示例

```bash
$ ./bin/traffic_planner.exe
========== 交通网络路径规划系统 ==========
1. 单点路径规划
2. 多点旅行规划 (TSP)
3. 退出
请选择功能: 1
请输入起点地标: 故宫
请输入终点地标: 外滩
请输入时间权重 (0.0-1.0): 0.7
请输入成本权重 (0.0-1.0): 0.3
...  # 输出省略
```

> 时间与成本权重相加不强制等于 1，可根据偏好自由输入。

---

## 4. 数据格式 (`data/nodes.csv`)

CSV UTF-8，无表头：

```
city_name,node_type,node_name,latitude,longitude
北京,landmark,故宫,39.9163,116.3972
北京,airport,首都国际机场,40.0801,116.5845
北京,hsr,北京南站,39.8652,116.3786
...
```

* **node_type** 可取 `landmark | airport | hsr` (高铁) — 解析后映射 `NodeType` 枚举。
* 新增/修改城市只需追加行，无需改源码。

---

## 5. 核心概念 & 主要接口


| 结构体 / 枚举   | 作用                               | 位置                      |
| ----------------- | ------------------------------------ | --------------------------- |
| `Node`          | 节点 (地标 / 机场 / 车站)          | include/traffic_network.h |
| `CityMeta`      | 城市到主要节点的索引               | 同上                      |
| `RoutePath`     | 由`PathSegment` 链表组成的完整路径 | 同上                      |
| `TransportMode` | 交通方式枚举                       | 同上                      |

```c
RoutePath* find_shortest_path(int start_node, int end_node,
                              double time_weight, double cost_weight);
RoutePath* solve_tsp(int* node_ids, int count,
                     double time_weight, double cost_weight);
void        free_route_path(RoutePath* path);
```

调用者仅需传入 **节点 ID** 与权重，算法自动考虑可达性 & 成本。

---

## 6. 算法实现概要

1. **距离计算** – 哈弗辛公式 (准确到 km)
2. **可达性规则** – 按节点类型 + 城际/市内判定过滤不合理线路
3. **Dijkstra** – 使用归一化 time/cost 作为代价
4. **Held-Karp TSP** – 状态压缩 DP，支持 ≤10 目的地

> 更多细节请阅读 `src/pathfinding.c` 内注释。

---

## 7. 开发路线图

- [X] 阶段 1：核心算法 & 命令行
- [X] 阶段 2：地标级节点 / CSV 数据源 / TSP 完善
- [ ] 阶段 3：地图可视化 (Leaflet/WebGL)
- [ ] 阶段 4：实时路况 API、增量更新 & 性能优化

---

## 8. 贡献指南

1. 新地标：编辑 `data/nodes.csv`，保持五列格式。
2. 新算法：在 `src/` 下新增 `*.c` 并在 Makefile 的 `SRCS` 通配中自动编译。
3. PR / Issue 欢迎提出需求与改进建议。

## 9. 阅读指南

以下顺序建议由浅入深、由外到内逐步掌握项目：

1. **README.md** (本文件)
   * 通览功能、目录、算法与路线图。
2. **Makefile**
   * 了解编译入口、依赖与产物位置，便于后续调试/扩展。
3. **include/traffic_network.h**
   * 核心数据结构与所有公共接口声明，阅读其注释可对整体模型一目了然。
4. **data/nodes.csv**
   * 熟悉数据格式；可对照 `NodeType` 理解节点语义。
5. **src/data_loader.c**
   * 启动流程第一站：CSV → Node / CityMeta 内存图。掌握数据如何进入程序。
6. **src/distance.c**
   * 哈弗辛公式实现；为后续路径成本提供基础距离。
7. **src/pathfinding.c**
   * 重点阅读：
     - `calculate_travel_info` 交通方式可达规则
     - `find_shortest_path` Dijkstra
     - `solve_tsp` Held-Karp 动态规划
8. **src/main.c**
   * 菜单交互与输出格式；结合前面接口可串起完整调用链。
9. **实际运行调试**
   * `make && ./bin/traffic_planner`，尝试不同地标与 TSP 输入，加深理解。

> 完成以上后，可继续研究未来待实现的可视化/实时路况等拓展点。

## 10. 许可证

MIT

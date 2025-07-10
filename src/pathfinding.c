/**
 * @file pathfinding.c
 * @brief 实现了项目核心的寻路算法，包括Dijkstra、TSP和顺序路径规划。
 */
#include "pathfinding.h"
#include "distance.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

/**
 * @brief 定义了不同交通方式的基础属性（速度和单位成本）。
 * @details 这是一个静态常量数组，用于快速查找和计算旅行成本。
 *          注意：这里的速度和成本是估算值，用于模拟真实的交通状况。
 */
static const struct {
    double speed_kmh;   // 巡航速度 (公里/小时)
    double cost_per_km; // 每公里的花费 (元/公里)
} transport_attrs[TRANSPORT_MODE_COUNT] = {
    {60.0,  0.8}, // DRIVING: 假设市内平均速度较低
    {250.0, 0.4}, // HIGH_SPEED_RAIL: 高铁平均速度
    {800.0, 0.6}, // FLIGHT: 飞机巡航速度
    {40.0,  0.2}  // BUS: 假设市内公交/长途大巴的平均速度
};

/**
 * @brief 计算两个节点之间通过特定交通方式旅行的详细信息（时间、花费、可达性）。
 * @details 这是寻路算法中计算图中"边"的权重的核心辅助函数。
 *          它包含了一系列现实世界规则，例如：
 *          - 飞机和高铁不能在同城内使用。
 *          - 飞机只能往返于机场之间，高铁往返于高铁站之间。
 *          - 市内交通的速度和成本有特定的计算方式。
 * 
 * @param distance_km 两节点间的地理距离。
 * @param mode 要评估的交通方式。
 * @param from_node 起点节点。
 * @param to_node 终点节点。
 * @return TravelInfo 包含时间、花费和可达性标志的结构体。
 */
static TravelInfo calculate_travel_info(double distance_km, TransportMode mode, const Node* from_node, const Node* to_node) {
    TravelInfo info;
    info.is_reachable = 0; // 默认设置为不可达

    if (!from_node || !to_node) return info; // 安全检查

    bool is_intra_city = (from_node->city_id == to_node->city_id);

    // --- 交通规则检查 ---
    switch (mode) {
        case FLIGHT:
            // 飞机不能同城，且必须在机场之间
            if (is_intra_city || from_node->type != NODE_TYPE_AIRPORT || to_node->type != NODE_TYPE_AIRPORT) return info;
            break;
        case HIGH_SPEED_RAIL:
            // 高铁不能同城，且必须在高铁站之间
            if (is_intra_city || from_node->type != NODE_TYPE_HSR_STATION || to_node->type != NODE_TYPE_HSR_STATION) return info;
            break;
        case DRIVING:
        case BUS:
            // 驾车或公交在同城内不能连接两个同类型的交通枢纽（如机场到机场）
            if (is_intra_city && from_node->type != NODE_TYPE_LANDMARK && from_node->type == to_node->type) return info;
            break;
        default:
            return info; // 未知交通方式
    }

    // 如果通过了规则检查，则设为可达并计算成本
    info.is_reachable = 1;
    if (is_intra_city) { // 市内交通有特定的速度和成本模型
        if (mode == DRIVING) {
            info.time_hours = distance_km / 30.0; // 市内驾车速度更快
            info.cost_yuan = distance_km * 1.5;   // 市内驾车成本更高
        } else { // 默认为公交
            info.time_hours = distance_km / 25.0; // 市内公交速度
            info.cost_yuan = distance_km * 0.3;   // 市内公交成本
        }
    } else { // 城际交通
        info.time_hours = distance_km / transport_attrs[mode].speed_kmh;
        info.cost_yuan = distance_km * transport_attrs[mode].cost_per_km;
    }
    return info;
}

// 释放RoutePath对象及其所有段的内存
void free_route_path(RoutePath* path) {
    if (!path) return;
    PathSegment* current = path->segments_head;
    while (current != NULL) {
        PathSegment* next = current->next;
        free(current);
        current = next;
    }
    free(path);
}

// Dijkstra算法的实现
RoutePath* find_shortest_path(const TrafficNetwork* network, int start_node_id, int end_node_id, double time_weight, double cost_weight) {
    int node_count = traffic_network_get_node_count(network);
    if (start_node_id < 0 || start_node_id >= node_count || end_node_id < 0 || end_node_id >= node_count) return NULL;

    // --- 数据归一化准备 ---
    // 为了让时间和花费有可比性，需要将它们归一化到相似的尺度(0-1)。
    // 这里估算一个理论上的最大时间和花费，作为归一化的分母。
    const double MAX_DIST_ESTIMATE = 6000.0;                    // 假设最大距离为6000公里 
    const double MAX_TIME_ESTIMATE = MAX_DIST_ESTIMATE / 40.0;  // 按最慢的公交速度估算
    const double MAX_COST_ESTIMATE = MAX_DIST_ESTIMATE * 1.5;   // 按最贵的驾车成本估算

    // --- Dijkstra算法初始化 ---
    DijkstraNode* dijkstra_nodes = (DijkstraNode*)malloc(node_count * sizeof(DijkstraNode));
    bool* visited = (bool*)calloc(node_count, sizeof(bool));
    // 初始化所有节点的成本为无穷大，前驱为-1
    for (int i = 0; i < node_count; i++) {
        dijkstra_nodes[i].cost = DBL_MAX;
        dijkstra_nodes[i].predecessor_node_id = -1;
    }
    dijkstra_nodes[start_node_id].cost = 0; // 起点的成本为0

    // --- 主循环 ---
    // 循环 node_count-1 次，或直到找到终点
    for (int i = 0; i < node_count; i++) {
        // 1. 在所有未访问的节点中，找到当前成本最小的节点(u)
        int u = -1;
        double min_cost = DBL_MAX;
        for (int j = 0; j < node_count; j++) {
            if (!visited[j] && dijkstra_nodes[j].cost < min_cost) {
                min_cost = dijkstra_nodes[j].cost;
                    u = j;
                }
            }

        // 如果找不到可选节点(u=-1)或已到达终点，则结束搜索
        if (u == -1 || u == end_node_id) break;
        visited[u] = true; // 标记u为已访问

        // 2. "松弛"操作：用节点u来更新其所有邻居的成本
        const Node* from_node = traffic_network_get_node_by_id(network, u);
        for (int v = 0; v < node_count; v++) {
            if (visited[v]) continue; // 跳过已访问的邻居
            
            const Node* to_node = traffic_network_get_node_by_id(network, v);
            double distance = calculate_distance(from_node->latitude, from_node->longitude, to_node->latitude, to_node->longitude);
            if (distance <= 0.1) continue; // 忽略距离过近或相同的节点

            // 尝试所有可能的交通方式
            for (int mode_idx = 0; mode_idx < TRANSPORT_MODE_COUNT; mode_idx++) {
                TravelInfo travel = calculate_travel_info(distance, (TransportMode)mode_idx, from_node, to_node);
                if (travel.is_reachable) {
                    // 计算加权成本
                    double normalized_time = travel.time_hours / MAX_TIME_ESTIMATE;
                    double normalized_cost = travel.cost_yuan / MAX_COST_ESTIMATE;
                    double weighted_cost = normalized_time * time_weight + normalized_cost * cost_weight;

                    // 如果通过u到达v的成本更低，则更新v的成本和前驱
                    if (dijkstra_nodes[u].cost + weighted_cost < dijkstra_nodes[v].cost) {
                        dijkstra_nodes[v].cost = dijkstra_nodes[u].cost + weighted_cost;
                        dijkstra_nodes[v].predecessor_node_id = u;
                        dijkstra_nodes[v].predecessor_mode = (TransportMode)mode_idx;
                    }
                }
            }
        }
    }

    // --- 路径重建 ---
    // 如果终点的前驱仍然是-1，说明不可达
    if (dijkstra_nodes[end_node_id].predecessor_node_id == -1 && start_node_id != end_node_id) {
        free(dijkstra_nodes);
        free(visited);
        return NULL;
    }

    RoutePath* path = (RoutePath*)calloc(1, sizeof(RoutePath));
    // 从终点开始，沿着前驱链条回溯到起点
    int current_node_id = end_node_id;
    while (current_node_id != start_node_id && dijkstra_nodes[current_node_id].predecessor_node_id != -1) {
        int pred_node_id = dijkstra_nodes[current_node_id].predecessor_node_id;
        
        PathSegment* segment = (PathSegment*)malloc(sizeof(PathSegment));
        const Node* from = traffic_network_get_node_by_id(network, pred_node_id);
        const Node* to = traffic_network_get_node_by_id(network, current_node_id);
        double dist = calculate_distance(from->latitude, from->longitude, to->latitude, to->longitude);
        TravelInfo travel = calculate_travel_info(dist, dijkstra_nodes[current_node_id].predecessor_mode, from, to);
        
        segment->from_node_id = pred_node_id;
        segment->to_node_id = current_node_id;
        segment->mode = dijkstra_nodes[current_node_id].predecessor_mode;
        segment->distance_km = dist;
        segment->time_hours = travel.time_hours;
        segment->cost_yuan = travel.cost_yuan;
        
        // 使用头插法构建路径段链表，这样回溯结束后顺序自然是正确的
        segment->next = path->segments_head;
        path->segments_head = segment;
        
        // 累加总计
        path->total_distance += dist;
        path->total_time += travel.time_hours;
        path->total_cost += travel.cost_yuan;
        path->segment_count++;
        
        current_node_id = pred_node_id; // 继续回溯
    }

    free(dijkstra_nodes);
    free(visited);
    return path;
}

/**
 * @brief 将一个路径(leg)拼接到另一个主路径(main_path)的前面。
 * @param main_path 主路径，拼接后它将包含两个路径的内容。
 * @param leg_to_prepend 要被拼接到前面的路径段。函数执行后，此路径容器将被释放。
 */
static void stitch_paths(RoutePath* main_path, RoutePath* leg_to_prepend) {
    if (!leg_to_prepend || !leg_to_prepend->segments_head) {
        free_route_path(leg_to_prepend);
        return;
    }
    // 如果主路径为空，直接将新路径段赋给它
    if (main_path->segments_head == NULL) {
        main_path->segments_head = leg_to_prepend->segments_head;
    } else {
        // 找到新路径段的尾部
        PathSegment* tail = leg_to_prepend->segments_head;
            while (tail->next != NULL) tail = tail->next;
        // 将尾部指向主路径的头部，完成拼接
            tail->next = main_path->segments_head;
            main_path->segments_head = leg_to_prepend->segments_head;
    }
    // 更新总计数据
    main_path->segment_count += leg_to_prepend->segment_count;
    main_path->total_cost += leg_to_prepend->total_cost;
    main_path->total_time += leg_to_prepend->total_time;
    main_path->total_distance += leg_to_prepend->total_distance;
    
    // 释放已被"掏空"的路径容器
    leg_to_prepend->segments_head = NULL;
    free_route_path(leg_to_prepend);
}

// TSP求解实现
RoutePath* solve_tsp(const TrafficNetwork* network, int* node_ids_to_visit, int num_nodes, double time_weight, double cost_weight) {
    if (num_nodes <= 1) return NULL;
    if (num_nodes > 10) { // 动态规划的复杂度是 O(n^2 * 2^n)，n较大时计算量巨大
        fprintf(stderr, "TSP求解器目前仅支持最多10个节点。\n");
        return NULL;
    }
    int* node_ids = (int*)malloc(num_nodes * sizeof(int));
    memcpy(node_ids, node_ids_to_visit, num_nodes * sizeof(int));

    // 1. 构建成本矩阵：计算每两个待访问节点之间的最短加权路径成本
    double** cost_matrix = (double**)malloc(num_nodes * sizeof(double*));
    for (int i = 0; i < num_nodes; i++) {
        cost_matrix[i] = (double*)malloc(num_nodes * sizeof(double));
        for (int j = 0; j < num_nodes; j++) {
            if (i == j) {
                cost_matrix[i][j] = 0;
            } else {
                RoutePath* p = find_shortest_path(network, node_ids[i], node_ids[j], time_weight, cost_weight);
                if (p && p->total_distance > 0) {
                    double normalized_time = p->total_time / (6000.0/40.0);
                    double normalized_cost = p->total_cost / (6000.0*1.5);
                    cost_matrix[i][j] = normalized_time * time_weight + normalized_cost * cost_weight;
                } else {
                    cost_matrix[i][j] = DBL_MAX; // 不可达
                }
                free_route_path(p);
            }
        }
    }

    // 2. 动态规划求解
    // dp_table[mask][i] 表示：经过mask所代表的城市子集，最终停在城市i的最低成本。
    // mask是一个位掩码，例如 mask = 0...01011 表示访问了城市0, 1, 3。
    int num_subsets = 1 << num_nodes;
    double** dp_table = (double**)malloc(num_subsets * sizeof(double*));
    int** path_table = (int**)malloc(num_subsets * sizeof(int*)); // 记录路径的前驱节点
    for (int i = 0; i < num_subsets; i++) {
        dp_table[i] = (double*)malloc(num_nodes * sizeof(double));
        path_table[i] = (int*)malloc(num_nodes * sizeof(int));
        for (int k = 0; k < num_nodes; k++) dp_table[i][k] = DBL_MAX;
    }

    dp_table[1][0] = 0; // 起点是城市0，只访问自己的成本是0 (mask=1)
    for (int mask = 1; mask < num_subsets; mask++) {
        for (int u = 0; u < num_nodes; u++) {
            if (!(mask & (1 << u))) continue; // u不在当前子集中，跳过
            if (dp_table[mask][u] == DBL_MAX) continue;

            for (int v = 0; v < num_nodes; v++) {
                if (!(mask & (1 << v))) { // v不在当前子集中，是下一个要访问的目标
                    int next_mask = mask | (1 << v);
                    if (dp_table[mask][u] + cost_matrix[u][v] < dp_table[next_mask][v]) {
                        dp_table[next_mask][v] = dp_table[mask][u] + cost_matrix[u][v];
                        path_table[next_mask][v] = u; // 记录v的前驱是u
                    }
                }
            }
        }
    }

    // 3. 找到最优路径的终点
    // 遍历所有可能的终点i，计算 (访问所有城市并停在i的成本 + 从i返回起点的成本) 的最小值
    int final_mask = num_subsets - 1; // 访问了所有城市的掩码
    double min_tour_cost = DBL_MAX;
    int tour_end_city = -1;
    for (int i = 1; i < num_nodes; i++) {
        if (dp_table[final_mask][i] != DBL_MAX && cost_matrix[i][0] != DBL_MAX) {
            if (dp_table[final_mask][i] + cost_matrix[i][0] < min_tour_cost) {
                min_tour_cost = dp_table[final_mask][i] + cost_matrix[i][0];
                tour_end_city = i;
            }
        }
    }

    if (tour_end_city == -1) { /* 错误处理和内存释放 */ return NULL; }

    // 4. 从终点回溯，重建完整路径
    RoutePath* final_path = (RoutePath*)calloc(1, sizeof(RoutePath));
    int current_city_idx = tour_end_city;
    int current_mask = final_mask; 
    // 先拼接上从最后一个城市返回起点的路段
    stitch_paths(final_path, find_shortest_path(network, node_ids[tour_end_city], node_ids[0], time_weight, cost_weight));
    while (current_city_idx != 0) {
        int prev_city_idx = path_table[current_mask][current_city_idx];
        // 拼接 (前一个城市 -> 当前城市) 的路段
        stitch_paths(final_path, find_shortest_path(network, node_ids[prev_city_idx], node_ids[current_city_idx], time_weight, cost_weight));
        current_mask ^= (1 << current_city_idx); // 从掩码中移除当前城市
        current_city_idx = prev_city_idx;        // 回溯到前一个城市
    }

    // 释放所有动态分配的内存
    for (int i = 0; i < num_nodes; ++i) free(cost_matrix[i]);
    free(cost_matrix);
    for (int i = 0; i < num_subsets; ++i) { free(dp_table[i]); free(path_table[i]); }
    free(dp_table);
    free(path_table);
    free(node_ids);

    return final_path;
}

// 顺序路径规划实现
RoutePath* find_sequential_path(const TrafficNetwork* network, int* node_ids_to_visit, int num_nodes, double time_weight, double cost_weight) {
    if (num_nodes < 2) return NULL; 

    RoutePath* final_path = (RoutePath*)calloc(1, sizeof(RoutePath));
    if (!final_path) return NULL;

    // 遍历所有需要连接的路段
    for (int i = 0; i < num_nodes - 1; i++) {
        int start_node_id = node_ids_to_visit[i];
        int end_node_id = node_ids_to_visit[i+1];

        // 查找当前路段的最短路径
        RoutePath* leg_path = find_shortest_path(network, start_node_id, end_node_id, time_weight, cost_weight);

        // 如果任何一段路径无法找到，则整个规划失败
        if (!leg_path || !leg_path->segments_head) {
            const Node* start_node = traffic_network_get_node_by_id(network, start_node_id);
            const Node* end_node = traffic_network_get_node_by_id(network, end_node_id);
            fprintf(stderr, "错误: 无法找到从 %s 到 %s 的路径。\n", start_node ? start_node->name : "未知", end_node ? end_node->name : "未知");
            free_route_path(final_path);
            free_route_path(leg_path);
            return NULL;
        }

        // 简单地将找到的路段拼接到最终路径的尾部
        // 注意：这里的实现比 stitch_paths 简单，但对于顺序拼接是足够的
        PathSegment* tail = final_path->segments_head;
        if (!tail) {
            final_path->segments_head = leg_path->segments_head;
        } else {
            while (tail->next) tail = tail->next;
            tail->next = leg_path->segments_head;
        }
        
        // 累加总计
        final_path->segment_count += leg_path->segment_count;
        final_path->total_cost += leg_path->total_cost;
        final_path->total_distance += leg_path->total_distance;
        final_path->total_time += leg_path->total_time;

        leg_path->segments_head = NULL;
        free_route_path(leg_path);
    }

    return final_path;
} 
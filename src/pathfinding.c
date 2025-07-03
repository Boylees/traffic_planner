#include "traffic_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

// 交通方式的属性
static const struct {
    double speed_kmh;
    double cost_per_km;
} transport_attrs[TRANSPORT_MODE_COUNT] = {
    {60.0,  0.8}, // DRIVING 市内速度降低
    {250.0, 0.4}, // HIGH_SPEED_RAIL
    {800.0, 0.6}, // FLIGHT
    {40.0,  0.2}  // BUS 市内速度降低
};

// 计算两节点间某种交通方式的旅行信息
TravelInfo calculate_travel_info(double distance_km, TransportMode mode, const Node* from_node, const Node* to_node) {
    TravelInfo info;
    info.is_reachable = 0;
    bool is_intra_city = (from_node->city_id == to_node->city_id);

    switch (mode) {
        case FLIGHT:
            if (is_intra_city || from_node->type != NODE_TYPE_AIRPORT || to_node->type != NODE_TYPE_AIRPORT) return info;           // 禁止同城同类型节点直连，如机场到机场
            break;
        case HIGH_SPEED_RAIL:
            if (is_intra_city || from_node->type != NODE_TYPE_HSR_STATION || to_node->type != NODE_TYPE_HSR_STATION) return info;   // 禁止同城同类型节点直连，如高铁站到高铁站
            break;
        case DRIVING:
            if (!is_intra_city && (from_node->type != NODE_TYPE_LANDMARK || to_node->type != NODE_TYPE_LANDMARK)) return info;       // 禁止跨城非地标节点直连
            if (is_intra_city && from_node->type == to_node->type) return info;                                                      // 禁止同城同类型节点直连，如地标到地标
            break;
        case BUS:
            if (!is_intra_city && (from_node->type != NODE_TYPE_LANDMARK || to_node->type != NODE_TYPE_LANDMARK)) return info;       // 禁止跨城非地标节点直连
            if (is_intra_city && from_node->type == to_node->type) return info;                                                      // 禁止同城同类型节点直连，如地标到地标
            break;
        default:
            return info;
    }

    info.is_reachable = 1;
    if(is_intra_city) {                                                                                                             // 市内交通，速度和费用固定
        if (mode == DRIVING) {
            info.time_hours = distance_km / 60.0;
            info.cost_yuan = distance_km * 0.8;
        } else {
            info.time_hours = distance_km / 40.0;
            info.cost_yuan = distance_km * 0.2;
        }
    } else {
        info.time_hours = distance_km / transport_attrs[mode].speed_kmh;                                                             // 计算时间
        info.cost_yuan = distance_km * transport_attrs[mode].cost_per_km;                                                            // 计算费用
    }
    if (mode == BUS && distance_km > 300) info.cost_yuan = info.cost_yuan * 1.5;                                                     // 长途公交费用增加
    return info;
}

// 释放路径链表
void free_route_path(RoutePath* path) {
    if (!path) return;                                                                                                               // 如果路径为空，直接返回
    PathSegment* current = path->segments_head;
    while (current != NULL) {
        PathSegment* next = current->next;                                                                                           // 获取下一个节点
        free(current);                                                                                                               // 释放当前节点
        current = next;                                                                                                              // 移动到下一个节点
    }
    free(path);                                                                                                                      // 释放路径
}

// 最短路径算法（Dijkstra）
RoutePath* find_shortest_path(int start_node_id, int end_node_id, double time_weight, double cost_weight) {
    if (start_node_id < 0 || start_node_id >= graph_node_count || end_node_id < 0 || end_node_id >= graph_node_count) return NULL;      // 检查节点ID是否有效

    double max_dist = 6000;                                                                                                         // 最大距离
    const double MAX_TRIP_TIME = max_dist / 40.0;                                                                                   // 公交速度
    const double MAX_TRIP_COST = max_dist * 0.8;                                                                                    // 驾车费用
    const double MIN_NORMALIZED_COST_PER_KM = (MAX_TRIP_TIME > 0 ? (1.0 / 800.0 / MAX_TRIP_TIME) * time_weight : 0) + (MAX_TRIP_COST > 0 ? (0.2 / MAX_TRIP_COST) * cost_weight : 0); // 最小归一化成本
    (void)MIN_NORMALIZED_COST_PER_KM; // 移除 A* 后只为抑制未使用警告

    DijkstraNode* dijkstra_nodes = (DijkstraNode*)malloc(graph_node_count * sizeof(DijkstraNode));
    bool* visited = (bool*)calloc(graph_node_count, sizeof(bool));
    int i;
    for (i = 0; i < graph_node_count; i++) {
        dijkstra_nodes[i].cost = DBL_MAX;
        dijkstra_nodes[i].predecessor_node_id = -1;
    }
    dijkstra_nodes[start_node_id].cost = 0;

    for (i = 0; i < graph_node_count; i++) {
        // 1. 选出 f_cost 最小的未访问节点 u
        // 2. 若 u==-1 (无可选) 或已到终点，则结束
        // 3. 标记 u 已访问
        // 4. 用 u "松弛"所有邻居 v
        int u = -1;
        double min_f_cost = DBL_MAX;
        int j;
        for (j = 0; j < graph_node_count; j++) {
            if (!visited[j] && dijkstra_nodes[j].cost != DBL_MAX) {
                /* 纯 Dijkstra：仅比较当前实际累计 cost，不加入启发式 */
                double f_cost = dijkstra_nodes[j].cost;
                if (f_cost < min_f_cost) {
                    min_f_cost = f_cost;
                    u = j;
                }
            }
        }
        if (u == -1 || u == end_node_id) break;
        visited[u] = 1;

        const Node* from_node = &graph_nodes[u];
        int v;
        for (v = 0; v < graph_node_count; v++) {
            if (visited[v]) continue;
            const Node* to_node = &graph_nodes[v];
            double distance = calculate_distance(from_node->latitude, from_node->longitude, to_node->latitude, to_node->longitude);
            if (distance <= 0.1) continue;

            int mode_idx;
            for (mode_idx = 0; mode_idx < TRANSPORT_MODE_COUNT; mode_idx++) {
                TravelInfo travel = calculate_travel_info(distance, (TransportMode)mode_idx, from_node, to_node);
                if (travel.is_reachable) {
                    double normalized_time = (MAX_TRIP_TIME > 0) ? (travel.time_hours / MAX_TRIP_TIME) : 0;
                    double normalized_cost = (MAX_TRIP_COST > 0) ? (travel.cost_yuan / MAX_TRIP_COST) : 0;
                    double weighted_cost = normalized_time * time_weight + normalized_cost * cost_weight;

                    if (dijkstra_nodes[u].cost + weighted_cost < dijkstra_nodes[v].cost) {
                        dijkstra_nodes[v].cost = dijkstra_nodes[u].cost + weighted_cost;
                        dijkstra_nodes[v].predecessor_node_id = u;
                        dijkstra_nodes[v].predecessor_mode = (TransportMode)mode_idx;
                    }
                }
            }
        }
    }

    if (dijkstra_nodes[end_node_id].predecessor_node_id == -1 && start_node_id != end_node_id) {
        free(dijkstra_nodes);
        free(visited);
        return NULL;
    }

    RoutePath* path = (RoutePath*)calloc(1, sizeof(RoutePath));
    int current_node_id = end_node_id;
    while (current_node_id != start_node_id && dijkstra_nodes[current_node_id].predecessor_node_id != -1) {
        int pred_node_id = dijkstra_nodes[current_node_id].predecessor_node_id;
        PathSegment* segment = (PathSegment*)malloc(sizeof(PathSegment));
        const Node* from = &graph_nodes[pred_node_id];
        const Node* to = &graph_nodes[current_node_id];
        double dist = calculate_distance(from->latitude, from->longitude, to->latitude, to->longitude);
        TravelInfo travel = calculate_travel_info(dist, dijkstra_nodes[current_node_id].predecessor_mode, from, to);
        // 结构体成员一行一行赋值
        segment->from_node_id = pred_node_id;
        segment->to_node_id = current_node_id;
        segment->mode = dijkstra_nodes[current_node_id].predecessor_mode;
        segment->distance_km = dist;
        segment->time_hours = travel.time_hours;
        segment->cost_yuan = travel.cost_yuan;
        segment->next = path->segments_head;
        path->segments_head = segment;
        path->total_distance += dist;
        path->total_time += travel.time_hours;
        path->total_cost += travel.cost_yuan;
        path->segment_count++;
        current_node_id = pred_node_id;
    }

    free(dijkstra_nodes);
    free(visited);
    return path;
}

// 拼接路径链表
static void stitch_paths(RoutePath* main_path, RoutePath* leg_to_prepend) {
    if (!leg_to_prepend || !main_path) {
        free_route_path(leg_to_prepend);
        return;
    }
    if (main_path->segments_head == NULL) {
        main_path->segments_head = leg_to_prepend->segments_head;
        leg_to_prepend->segments_head = NULL;
    } else {
        PathSegment* tail = leg_to_prepend->segments_head;
        if (tail) {
            while (tail->next != NULL) tail = tail->next;
            tail->next = main_path->segments_head;
            main_path->segments_head = leg_to_prepend->segments_head;
            leg_to_prepend->segments_head = NULL;
        }
    }
    main_path->segment_count += leg_to_prepend->segment_count;
    main_path->total_cost += leg_to_prepend->total_cost;
    main_path->total_time += leg_to_prepend->total_time;
    main_path->total_distance += leg_to_prepend->total_distance;
    free_route_path(leg_to_prepend);
}

// TSP求解
RoutePath* solve_tsp(int* node_ids_to_visit, int num_nodes, double time_weight, double cost_weight) {
    if (num_nodes <= 1) return NULL;
    if (num_nodes > 10) {
        fprintf(stderr, "TSP求解器目前仅支持最多10个节点。\n");
        return NULL;
    }
    int* node_ids = (int*)malloc(num_nodes * sizeof(int));
    memcpy(node_ids, node_ids_to_visit, num_nodes * sizeof(int));

    double** cost_matrix = (double**)malloc(num_nodes * sizeof(double*));
    int i, j;
    for (i = 0; i < num_nodes; i++) {
        cost_matrix[i] = (double*)malloc(num_nodes * sizeof(double));
        for (j = 0; j < num_nodes; j++) {
            if (i == j) cost_matrix[i][j] = 0;
            else {
                RoutePath* p = find_shortest_path(node_ids[i], node_ids[j], time_weight, cost_weight);
                if (p && p->total_distance > 0) {
                    double normalized_time = p->total_time / (6000.0/40.0);
                    double normalized_cost = p->total_cost / (6000.0*0.8);
                    cost_matrix[i][j] = normalized_time * time_weight + normalized_cost * cost_weight;
                } else {
                    cost_matrix[i][j] = DBL_MAX;
                }
                free_route_path(p);
            }
        }
    }

    int num_subsets = 1 << num_nodes;
    double** dp_table = (double**)malloc(num_subsets * sizeof(double*));
    int** path_table = (int**)malloc(num_subsets * sizeof(int*));
    for (i = 0; i < num_subsets; i++) {
        dp_table[i] = (double*)malloc(num_nodes * sizeof(double));
        path_table[i] = (int*)malloc(num_nodes * sizeof(int));
        int k;
        for (k = 0; k < num_nodes; k++) dp_table[i][k] = DBL_MAX;
    }

    dp_table[1][0] = 0;
    int mask, u, v;
    for (mask = 1; mask < num_subsets; mask++) {
        for (u = 0; u < num_nodes; u++) {
            if (!(mask & (1 << u)) || dp_table[mask][u] == DBL_MAX) continue;
            for (v = 0; v < num_nodes; v++) {
                if (!(mask & (1 << v)) && cost_matrix[u][v] != DBL_MAX) {
                    int next_mask = mask | (1 << v);
                    if (dp_table[mask][u] + cost_matrix[u][v] < dp_table[next_mask][v]) {
                        dp_table[next_mask][v] = dp_table[mask][u] + cost_matrix[u][v];
                        path_table[next_mask][v] = u;
                    }
                }
            }
        }
    }

    int final_mask = num_subsets - 1;
    double min_tour_cost = DBL_MAX;
    int tour_end_city = -1;
    for (i = 1; i < num_nodes; i++) {
        if (dp_table[final_mask][i] != DBL_MAX && cost_matrix[i][0] != DBL_MAX) {
            if (dp_table[final_mask][i] + cost_matrix[i][0] < min_tour_cost) {
                min_tour_cost = dp_table[final_mask][i] + cost_matrix[i][0];
                tour_end_city = i;
            }
        }
    }

    if (tour_end_city == -1) {
        for (i = 0; i < num_nodes; ++i) free(cost_matrix[i]);
        free(cost_matrix);
        for (i = 0; i < num_subsets; ++i) { free(dp_table[i]); free(path_table[i]); }
        free(dp_table);
        free(path_table);
        free(node_ids);
        return NULL;
    }

    RoutePath* final_path = (RoutePath*)calloc(1, sizeof(RoutePath));
    int current_city_idx = tour_end_city;
    int current_mask = final_mask;
    stitch_paths(final_path, find_shortest_path(node_ids[tour_end_city], node_ids[0], time_weight, cost_weight));
    while (current_city_idx != 0) {
        int prev_city_idx = path_table[current_mask][current_city_idx];
        stitch_paths(final_path, find_shortest_path(node_ids[prev_city_idx], node_ids[current_city_idx], time_weight, cost_weight));
        current_mask ^= (1 << current_city_idx);
        current_city_idx = prev_city_idx;
    }

    for (i = 0; i < num_nodes; ++i) free(cost_matrix[i]);
    free(cost_matrix);
    for (i = 0; i < num_subsets; ++i) { free(dp_table[i]); free(path_table[i]); }
    free(dp_table);
    free(path_table);
    free(node_ids);

    return final_path;
} 
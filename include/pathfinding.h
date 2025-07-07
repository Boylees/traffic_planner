#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "graph.h"
#include "types.h"

/**
 * @brief 使用Dijkstra算法查找两个节点之间的最短加权路径。
 * @details "最短"是根据时间和花费的加权组合来定义的，并非单纯的地理距离最短。
 * 
 * @param network 指向交通网络实例的只读指针。
 * @param start_node_id 起始节点的ID。
 * @param end_node_id 目标节点的ID。
 * @param time_weight 时间在总成本计算中的权重 (0.0 to 1.0)。
 * @param cost_weight 花费在总成本计算中的权重 (0.0 to 1.0)。
 * @return RoutePath* 成功时返回一个指向新创建的RoutePath对象的指针，包含了完整的路径信息。
 *                    如果找不到路径或发生错误，则返回NULL。调用者有责任使用 free_route_path() 释放返回的路径。
 */
RoutePath* find_shortest_path(const TrafficNetwork* network, int start_node_id, int end_node_id, double time_weight, double cost_weight);

/**
 * @brief 使用动态规划（Held-Karp算法）解决旅行商问题(TSP)。
 * @details 找到一条访问所有给定节点并返回起点的、总加权成本最低的路线。
 * 
 * @param network 指向交通网络实例的只读指针。
 * @param node_ids_to_visit 指向一个包含待访问节点ID数组的指针。数组的第一个元素被视作起点和终点。
 * @param num_nodes 待访问节点的数量。
 * @param time_weight 时间权重。
 * @param cost_weight 花费权重。
 * @return RoutePath* 成功时返回最优的TSP路径。失败或无解时返回NULL。调用者需负责释放。
 */
RoutePath* solve_tsp(const TrafficNetwork* network, int* node_ids_to_visit, int num_nodes, double time_weight, double cost_weight);

/**
 * @brief 按照给定的节点顺序，规划一条依次访问的路径。
 * @details 这不是TSP，它不会重新排序节点，而是严格按照用户指定的顺序连接各个点。
 * 
 * @param network 指向交通网络实例的只读指针。
 * @param node_ids_to_visit 指向一个包含待访问节点ID数组的指针，路径将严格遵循此顺序。
 * @param num_nodes 待访问节点的数量。
 * @param time_weight 时间权重。
 * @param cost_weight 花费权重。
 * @return RoutePath* 成功时返回拼接好的顺序路径。如果其中任何一段无法连接，则返回NULL。调用者需负责释放。
 */
RoutePath* find_sequential_path(const TrafficNetwork* network, int* node_ids_to_visit, int num_nodes, double time_weight, double cost_weight);

/**
 * @brief 释放由寻路函数创建的RoutePath对象及其内部所有路径段所占用的内存。
 * 
 * @param path 指向要释放的RoutePath对象的指针。
 */
void free_route_path(RoutePath* path);

#endif // PATHFINDING_H 
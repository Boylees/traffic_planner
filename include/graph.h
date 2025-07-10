#ifndef GRAPH_H
#define GRAPH_H

#include "types.h"

/**
 * @brief 交通网络的核心数据结构。
 * @details 这是一个 "不透明" 结构体的句柄，封装了所有节点、城市和它们之间的关系。
 *          外部模块通过操作这个结构体的指针来与图数据交互，而无需关心其内部实现。
 */
typedef struct {
    Node* nodes;            ///< 指向节点数组的指针，存储了所有的交通节点。
    int node_count;         ///< 节点数组中的元素总数。
    CityMeta* cities;       ///< 指向城市元数据数组的指针。
    int city_count;         ///< 城市数组中的元素总数。
    int node_capacity;
    int city_capacity;
} TrafficNetwork;

/**
 * @brief 从CSV文件加载数据，创建并初始化一个新的交通网络实例。
 * @details 这是程序的入口点之一，负责分配内存并解析数据文件来构建图。
 *          调用者必须在程序结束时使用 traffic_network_destroy() 来释放返回的实例。
 * 
 * @param nodes_csv_path 指向包含节点数据的CSV文件的路径字符串。
 * @return TrafficNetwork* 成功时，返回一个指向新创建的TrafficNetwork实例的指针。
 *                         如果文件无法打开或内存分配失败，则返回NULL。
 */
TrafficNetwork* traffic_network_create(const char* nodes_csv_path);

/**
 * @brief 释放由 traffic_network_create() 创建的交通网络实例所占用的所有内存。
 * @details 这是一个关键的清理函数，用于防止内存泄漏。
 * 
 * @param network 指向要销毁的TrafficNetwork实例的指针。如果传入NULL，函数会安全地不做任何操作。
 */
void traffic_network_destroy(TrafficNetwork* network);

/**
 * @brief 根据节点的唯一ID安全地获取一个只读的节点引用。
 * @details 提供了一个受控的访问节点数据的方式，避免了直接暴露内部数组。
 * 
 * @param network 指向交通网络实例的指针。
 * @param node_id 要获取的节点的ID。
 * @return const Node* 成功时，返回一个指向Node结构体的只读指针。
 *                     如果network为NULL或node_id越界，则返回NULL。
 */
const Node* traffic_network_get_node_by_id(const TrafficNetwork* network, int node_id);

/**
 * @brief 获取当前交通网络中的总节点数。
 * 
 * @param network 指向交通网络实例的指针。
 * @return int 返回网络中的节点总数。如果network为NULL，返回0。
 */
int traffic_network_get_node_count(const TrafficNetwork* network);

/**
 * @brief 根据节点的字符串名称来查找其唯一ID。
 * @details 这是用户输入（地名）和内部ID系统的桥梁。
 * 
 * @param network 指向交通网络实例的指针。
 * @param name 要查找的节点的名称字符串。
 * @return int 如果找到匹配的节点，则返回其ID。如果network或name为NULL，或未找到匹配项，则返回-1。
 */
int traffic_network_find_node_id_by_name(const TrafficNetwork* network, const char* name);

#endif // GRAPH_H 
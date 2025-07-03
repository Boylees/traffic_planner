#ifndef TRAFFIC_NETWORK_H
#define TRAFFIC_NETWORK_H

#include <stdbool.h>

// --- 核心数据结构 ---

// 交通网络中的节点类型
typedef enum {
    NODE_TYPE_LANDMARK,     // 城市地标/市中心
    NODE_TYPE_AIRPORT,      // 机场
    NODE_TYPE_HSR_STATION,  // 高铁站
} NodeType;

// 通用节点结构，代表图上的一个点
typedef struct {
    int id;                 // 全局唯一的节点ID
    int city_id;            // 所属城市的ID
    NodeType type;          // 节点类型
    char name[100];         // 节点的具体名称 (如 "故宫", "北京首都国际机场")
    double latitude;        // 纬度
    double longitude;       // 经度
} Node;

// 城市元数据，包含其所有节点的信息
typedef struct {
    int city_id;            // 城市ID
    char city_name[50];     // 城市名称
    
    // 关联的节点ID
    int landmark_node_id;   // 地标节点ID   
    int airport_node_id;    // 机场节点ID
    int hsr_node_id;        // 高铁站节点ID
} CityMeta;


// 交通方式枚举
typedef enum {
    DRIVING = 0,            // 驾车
    HIGH_SPEED_RAIL,        // 高铁
    FLIGHT,                 // 飞机
    BUS,                    // 公交
    TRANSPORT_MODE_COUNT    // 交通方式数量
} TransportMode;

// 路径段结构
typedef struct PathSegment {
    int from_node_id;       // 起点节点ID
    int to_node_id;         // 终点节点ID
    TransportMode mode;     // 交通方式
    double distance_km;     // 距离
    double time_hours;      // 时间
    double cost_yuan;       // 费用
    struct PathSegment* next; // 用于构建链表
} PathSegment;

// 完整路径结构
typedef struct {
    PathSegment *segments_head;      // 路径段链表头指针
    int segment_count;               // 路径段数量
    double total_time;               // 总时间
    double total_cost;               // 总费用
    double total_distance;           // 总距离
} RoutePath;

// 用于Dijkstra算法的节点
typedef struct {
    double cost;                     // 综合成本 (time * time_weight + cost * cost_weight)
    int predecessor_node_id;         // 前驱节点ID
    TransportMode predecessor_mode;  // 前驱交通方式
} DijkstraNode;

// 交通信息（基于距离）
typedef struct {
    double time_hours;              // 时间
    double cost_yuan;               // 费用
    bool is_reachable;              // 是否可达
} TravelInfo;


// --- 全局数据 ---
extern Node* graph_nodes;           // 图节点数组
extern int graph_node_count;        // 图节点数量
extern CityMeta* city_meta_data;    // 城市元数据数组
extern int city_meta_count;         // 城市元数据数量

// --- 函数原型 ---

// data_loader.c 
void initialize_graph_data();        // 初始化图数据
void free_graph_data();              // 释放图数据

// distance.c
double calculate_distance(double lat1, double lng1, 
                          double lat2, double lng2); // 计算两点之间的距离

// pathfinding.c
RoutePath* find_shortest_path(int start_node_id, int end_node_id, 
                              double time_weight, double cost_weight); // 计算最短路径

// 输入为节点ID列表（通常为各城市的地标节点ID或用户指定的任意节点ID）
RoutePath* solve_tsp(int* node_ids_to_visit, int num_nodes, 
                     double time_weight, double cost_weight); // 计算TSP问题

void free_route_path(RoutePath* path); // 释放路径

#endif // TRAFFIC_NETWORK_H 
#ifndef TRAFFIC_NETWORK_H
#define TRAFFIC_NETWORK_H

#include <stdbool.h>

// --- 核心数据结构 ---

// 交通网络中的节点类型
typedef enum {
    NODE_TYPE_LANDMARK, // 城市地标/市中心
    NODE_TYPE_AIRPORT,
    NODE_TYPE_HSR_STATION
} NodeType;

// 通用节点结构，代表图上的一个点
typedef struct {
    int id;          // 全局唯一的节点ID
    int city_id;     // 所属城市的ID
    NodeType type;   // 节点类型
    char name[100];  // 节点的具体名称 (如 "故宫", "北京首都国际机场")
    double latitude;
    double longitude;
} Node;

// 城市元数据，包含其所有节点的信息
typedef struct {
    int city_id;
    char city_name[50];
    
    // 关联的节点ID
    int landmark_node_id;
    int airport_node_id; // -1 if not available
    int hsr_node_id;     // -1 if not available
} CityMeta;


// 交通方式枚举
typedef enum {
    DRIVING = 0,
    HIGH_SPEED_RAIL = 1,
    FLIGHT = 2,
    BUS = 3,
    TRANSPORT_MODE_COUNT // 用于计数
} TransportMode;

// 路径段结构
typedef struct PathSegment {
    int from_node_id; // 使用节点ID
    int to_node_id;   // 使用节点ID
    TransportMode mode;
    double distance_km;
    double time_hours;
    double cost_yuan;
    struct PathSegment* next; // 用于构建链表
} PathSegment;

// 完整路径结构
typedef struct {
    PathSegment *segments_head; // 使用链表头指针
    int segment_count;
    double total_time;
    double total_cost;
    double total_distance;
} RoutePath;

// 用于Dijkstra算法的节点
typedef struct {
    double cost; // 综合成本 (time * time_weight + cost * cost_weight)
    int predecessor_node_id;
    TransportMode predecessor_mode;
} DijkstraNode;

// 交通信息（基于距离）
typedef struct {
    double time_hours;
    double cost_yuan;
    bool is_reachable;
} TravelInfo;


// --- 全局数据 ---
extern Node* graph_nodes;
extern int graph_node_count;
extern CityMeta* city_meta_data;
extern int city_meta_count;

// --- 函数原型 ---

// data_loader.c (新文件)
void initialize_graph_data();
void free_graph_data();

// distance.c
double calculate_distance(double lat1, double lng1, double lat2, double lng2);

// pathfinding.c
RoutePath* find_shortest_path(int start_node_id, int end_node_id, double time_weight, double cost_weight);
// 输入为节点ID列表（通常为各城市的地标节点ID或用户指定的任意节点ID）
RoutePath* solve_tsp(int* node_ids_to_visit, int num_nodes, double time_weight, double cost_weight);

void free_route_path(RoutePath* path);

#endif // TRAFFIC_NETWORK_H 
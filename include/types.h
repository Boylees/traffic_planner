#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

/**
 * @brief 定义了交通网络中节点的具体类型。
 * @details 用于区分一个节点是城市中的普通地标、机场还是高铁站，
 *          这在计算不同交通方式的可达性时至关重要。
 */
typedef enum {
    NODE_TYPE_LANDMARK,     ///< 普通地标，如市中心、旅游景点。
    NODE_TYPE_AIRPORT,      ///< 机场，是飞机交通的起止点。
    NODE_TYPE_HSR_STATION,  ///< 高铁站，是高速铁路的起止点。
} NodeType;

/**
 * @brief 定义了图上的一个通用节点（例如一个具体的地标或站点）。
 * @details 这是构成交通网络图的基本单位。
 */
typedef struct {
    int id;                 ///< 在节点数组中的索引，全局唯一，用于快速查找。
    int city_id;            ///< 该节点所属城市的ID，用于判断是否同城。
    NodeType type;          ///< 节点的类型 (地标, 机场, 或高铁站)。
    char name[100];         ///< 节点的具体名称，例如 "故宫" 或 "首都国际机场"。
    double latitude;        ///< 地理纬度，用于计算距离。
    double longitude;       ///< 地理经度，用于计算距离。
} Node;

/**
 * @brief 存储一个城市的元数据，并快速索引到该城市的重要交通枢纽。
 */
typedef struct {
    int city_id;            ///< 在城市数组中的索引，全局唯一。
    char city_name[50];     ///< 城市的名称，例如 "北京"。
    
    // --- 关联的关键节点ID ---
    // 保存ID而非指针，可以简化内存管理，避免悬挂指针。
    int landmark_node_id;   ///< 该城市默认地标的节点ID。
    int airport_node_id;    ///< 该城市机场的节点ID (-1表示没有)。
    int hsr_node_id;        ///< 该城市高铁站的节点ID (-1表示没有)。
} CityMeta;


/**
 * @brief 定义了所有支持的交通方式。
 * @details 此枚举的最后一个成员 `TRANSPORT_MODE_COUNT` 是一个技巧，
 *          它自动代表了枚举成员的总数，方便在循环中使用。
 */
typedef enum {
    DRIVING = 0,            ///< 驾车。
    HIGH_SPEED_RAIL,        ///< 高铁。
    FLIGHT,                 ///< 飞机。
    BUS,                    ///< 公交/大巴。
    TRANSPORT_MODE_COUNT    ///< 交通方式的总数量，必须是最后一个。
} TransportMode;

// 前向声明，因为 PathSegment 结构体中需要用到自己类型的指针。
struct PathSegment;

/**
 * @brief 代表一条完整路径中的一个路段。
 * @details 这是一个链表节点，多个路段链接在一起构成一条完整路径。
 */
typedef struct PathSegment {
    int from_node_id;           ///< 该路段的起点节点ID。
    int to_node_id;             ///< 该路段的终点节点ID。
    TransportMode mode;         ///< 该路段所使用的交通方式。
    double distance_km;         ///< 该路段的地理距离（公里）。
    double time_hours;          ///< 走完该路段预计需要的时间（小时）。
    double cost_yuan;           ///< 走完该路段预计需要的花费（元）。
    struct PathSegment* next;   ///< 指向下一个路段的指针，形成链表。
} PathSegment;

/**
 * @brief 代表一条从起点到终点的完整路径。
 * @details 包含一个路径段链表和该路径的总计信息。
 */
typedef struct {
    PathSegment *segments_head; ///< 指向路径中第一个路段的指针（链表头）。
    int segment_count;          ///< 路径中包含的路段总数。
    double total_time;          ///< 完成整条路径的总时间。
    double total_cost;          ///< 完成整条路径的总花费。
    double total_distance;      ///< 整条路径的总距离。
} RoutePath;

/**
 * @brief 在Dijkstra寻路算法中，用于记录每个节点状态的辅助结构。
 * @details 它不直接暴露给外部模块，仅在 pathfinding.c 内部使用。
 */
typedef struct {
    double cost;                ///< 从起点到此节点的累计加权成本（时间+花费）。
    int predecessor_node_id;    ///< 在最短路径树上，此节点的前一个节点的ID。
    TransportMode predecessor_mode; ///< 从前驱节点到此节点所使用的交通方式。
} DijkstraNode;

/**
 * @brief 封装了一次旅行（一个路段）的计算结果。
 * @details 用于在计算两个节点之间通过某种交通方式旅行的可行性和成本时，
 *          临时存储和返回计算结果。
 */
typedef struct {
    double time_hours;          ///< 预计时间。
    double cost_yuan;           ///< 预计花费。
    bool is_reachable;          ///< 标记这两个节点之间通过该方式是否可达。
} TravelInfo;

#endif // TYPES_H 
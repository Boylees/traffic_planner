/**
 * @file graph.c
 * @brief 实现了交通网络数据的加载、管理和查询功能。
 * @details 该模块将所有数据操作封装起来，对上层提供一个干净的、面向对象的接口。
 *          所有内存分配和释放的责任都集中在此模块中。
 */
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 从CSV文件创建交通网络结构
 * @details 该函数负责：
 *          1. 打开并解析CSV格式的节点数据文件
 *          2. 动态分配内存存储网络结构
 *          3. 构建城市和节点的索引关系
 * 
 * @param nodes_csv_path CSV文件路径
 * @return TrafficNetwork* 成功返回网络指针，失败返回NULL
 */
TrafficNetwork* traffic_network_create(const char* nodes_csv_path) {
    // ==================== 文件打开阶段 ====================
    // 以只读模式打开CSV文件，使用二进制模式避免文本转换问题
    FILE* fp = fopen(nodes_csv_path, "rb");
    if (fp == NULL) {
        // 文件打开失败时打印具体错误信息（包括errno）
        fprintf(stderr, "错误：无法打开文件 %s (错误码: %d)\n", 
                nodes_csv_path, errno);
        return NULL;
    }

    // ==================== 内存分配阶段 ====================
    // 为整个交通网络结构体分配内存（使用calloc确保零初始化）
    TrafficNetwork* network = (TrafficNetwork*)calloc(1, sizeof(TrafficNetwork));
    if (!network) {
        fprintf(stderr, "错误: 交通网络对象内存分配失败\n");
        fclose(fp);
        return NULL;
    }

    // --- 初始化动态数组 ---
    // 为节点和城市数组预分配初始容量，减少realloc调用
    const int INITIAL_CAP_NODES = 256;   // 节点数组初始容量
    const int INITIAL_CAP_CITIES = 128;  // 城市数组初始容量
    
    // 分配节点数组内存（使用calloc初始化）
    network->nodes = (Node*)calloc(INITIAL_CAP_NODES, sizeof(Node));
    // 分配城市元数据数组内存
    network->cities = (CityMeta*)calloc(INITIAL_CAP_CITIES, sizeof(CityMeta));
    
    // 记录当前分配容量（新增字段）
    network->node_capacity = INITIAL_CAP_NODES;
    network->city_capacity = INITIAL_CAP_CITIES;
    // 初始化元素计数器
    network->node_count = 0;
    network->city_count = 0;

    // 检查关键内存分配是否成功
    if (!network->nodes || !network->cities) {
        fprintf(stderr, "错误: 节点或城市数组内存分配失败\n");
        // 使用销毁函数确保安全释放所有已分配内存
        traffic_network_destroy(network);
        fclose(fp);
        return NULL;
    }

    // ==================== 文件解析阶段 ====================
    char line[512]; // 行缓冲区（足够大以容纳长行）
    
    // 读取并丢弃CSV文件的第一行（表头）
    if (!fgets(line, sizeof(line), fp)) {
        fprintf(stderr, "警告: 空文件或读取表头失败\n");
    }

    // 逐行解析文件内容
    while (fgets(line, sizeof(line), fp)) {
        // --- 行预处理 ---
        // 跳过空行、纯换行行和注释行（以#开头）
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // --- CSV字段解析 ---
        char city_name[50] = {0};  // 城市名称缓冲区
        char type_str[20] = {0};   // 节点类型字符串缓冲区
        char node_name[100] = {0}; // 节点名称缓冲区
        double lat = 0.0, lon = 0.0; // 经纬度
        
        // 使用安全格式解析（限制字段长度，防止缓冲区溢出）
        int parsed = sscanf(line, "%49[^,],%19[^,],%99[^,],%lf,%lf", 
                           city_name, type_str, node_name, &lat, &lon);
        
        // 检查是否成功解析5个字段（城市名,类型,节点名,纬度,经度）
        if (parsed != 5) {
            fprintf(stderr, "警告: 跳过格式错误行: %s", line);
            continue;
        }

        // --- 节点类型转换 ---
        NodeType ntype;
        if (strcmp(type_str, "landmark") == 0) {
            ntype = NODE_TYPE_LANDMARK;
        } 
        else if (strcmp(type_str, "airport") == 0) {
            ntype = NODE_TYPE_AIRPORT;
        }
        else if (strcmp(type_str, "hsr") == 0) {
            ntype = NODE_TYPE_HSR_STATION;
        }
        else {
            fprintf(stderr, "警告: 未知节点类型 '%s'\n", type_str);
            continue; // 跳过不支持的类型
        }

        // --- 城市管理 ---
        // 查找或创建城市记录
        int city_id = -1;
        
        // 遍历现有城市查找匹配项
        for (int i = 0; i < network->city_count; i++) {
            if (strcmp(network->cities[i].city_name, city_name) == 0) {
                city_id = i;
                break;
            }
        }
        
        // 如果是新城市
        if (city_id == -1) {
            // 检查城市数组容量
            if (network->city_count >= network->city_capacity) {
                // 容量翻倍策略减少realloc次数
                int new_capacity = network->city_capacity * 2;
                CityMeta* new_cities = (CityMeta*)realloc(
                    network->cities, new_capacity * sizeof(CityMeta));
                
                if (!new_cities) {
                    fprintf(stderr, "错误: 城市数组扩容失败\n");
                    continue; // 跳过当前节点（不终止整个加载过程）
                }
                
                network->cities = new_cities;
                network->city_capacity = new_capacity;
            }
            
            // 初始化新城市记录
            city_id = network->city_count;
            CityMeta* city = &network->cities[city_id];
            
            city->city_id = city_id;
            // 安全拷贝城市名称（确保终止符）
            strncpy(city->city_name, city_name, sizeof(city->city_name) - 1);
            city->city_name[sizeof(city->city_name) - 1] = '\0';
            
            // 初始化枢纽节点ID为-1（表示未设置）
            city->landmark_node_id = -1;
            city->airport_node_id = -1;
            city->hsr_node_id = -1;
            
            network->city_count++;
        }

        // --- 节点添加 ---
        // 检查节点数组容量
        if (network->node_count >= network->node_capacity) {
            // 容量翻倍策略
            int new_capacity = network->node_capacity * 2;
            Node* new_nodes = (Node*)realloc(
                network->nodes, new_capacity * sizeof(Node));
            
            if (!new_nodes) {
                fprintf(stderr, "错误: 节点数组扩容失败\n");
                continue;
            }
            
            network->nodes = new_nodes;
            network->node_capacity = new_capacity;
        }
        
        // 获取当前节点指针
        Node* node = &network->nodes[network->node_count];
        
        // 初始化节点字段
        node->id = network->node_count; // ID等于当前计数
        node->city_id = city_id;
        node->type = ntype;
        node->latitude = lat;
        node->longitude = lon;
        
        // 安全拷贝节点名称（确保终止符）
        strncpy(node->name, node_name, sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
        
        // --- 更新城市索引 ---
        // 如果是该城市第一个此类枢纽节点，则记录其ID
        CityMeta* city = &network->cities[city_id];
        switch (ntype) {
            case NODE_TYPE_LANDMARK:
                if (city->landmark_node_id == -1) {
                    city->landmark_node_id = node->id;
                }
                break;
                
            case NODE_TYPE_AIRPORT:
                if (city->airport_node_id == -1) {
                    city->airport_node_id = node->id;
                }
                break;
                
            case NODE_TYPE_HSR_STATION:
                if (city->hsr_node_id == -1) {
                    city->hsr_node_id = node->id;
                }
                break;
                
            default:
                break; // 不应发生（前面已过滤）
        }
        
        network->node_count++; // 成功添加节点
    }

    // ==================== 清理阶段 ====================
    fclose(fp); // 关闭文件
    
    // 打印加载统计信息（调试用）
    printf("成功加载: %d 个城市, %d 个节点\n", 
           network->city_count, network->node_count);
    
    return network;
}

void traffic_network_destroy(TrafficNetwork* network) {
    if (network) {
        free(network->nodes);   // 释放节点数组
        free(network->cities);  // 释放城市数组
        free(network);          // 释放网络结构体本身
    }
}

const Node* traffic_network_get_node_by_id(const TrafficNetwork* network, int node_id) {
    // 检查network指针是否有效，以及node_id是否在有效范围内
    if (network && node_id >= 0 && node_id < network->node_count) {
        return &network->nodes[node_id]; // 返回指向该节点的指针
    }
    return NULL; // 如果无效，返回NULL
}

int traffic_network_get_node_count(const TrafficNetwork* network) {
    return network ? network->node_count : 0; // 如果network为NULL，安全地返回0
}

int traffic_network_find_node_id_by_name(const TrafficNetwork* network, const char* name) {
    if (!network || !name) return -1; // 防御性检查
    // 遍历所有节点
    for (int i = 0; i < network->node_count; i++) {
        // 如果找到名称匹配的节点
        if (strcmp(network->nodes[i].name, name) == 0) {
            return i; // 返回其ID
        }
    }
    return -1; // 遍历完都未找到，返回-1
} 
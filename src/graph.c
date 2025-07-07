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

TrafficNetwork* traffic_network_create(const char* nodes_csv_path) {
    // 以只读模式打开CSV文件
    FILE* fp = fopen(nodes_csv_path, "r");
    if (fp == NULL) {
        // 如果文件打开失败，向标准错误流打印错误信息并返回NULL
        fprintf(stderr, "错误：无法打开 %s 文件\n", nodes_csv_path);
        return NULL;
    }

    // 为整个交通网络结构体分配内存
    TrafficNetwork* network = (TrafficNetwork*)malloc(sizeof(TrafficNetwork));
    if (!network) {
        // 如果内存分配失败，打印错误信息，关闭文件并返回NULL
        fprintf(stderr, "错误: 交通网络对象内存分配失败\n");
        fclose(fp);
        return NULL;
    }

    // --- 初始化动态数组 ---
    // 为节点和城市数组预分配一个初始容量，以减少频繁的realloc调用
    const int INITIAL_CAP_NODES = 256;
    const int INITIAL_CAP_CITIES = 128;
    // 分配初始内存
    network->nodes = malloc(INITIAL_CAP_NODES * sizeof(Node));
    network->cities = malloc(INITIAL_CAP_CITIES * sizeof(CityMeta));
    // 记录当前容量
    int node_cap = INITIAL_CAP_NODES;
    int city_cap = INITIAL_CAP_CITIES;
    // 初始化当前元素数量为0
    network->node_count = 0;
    network->city_count = 0;
    
    // 检查初始内存分配是否成功
    if (!network->nodes || !network->cities) {
        fprintf(stderr, "错误: 节点或城市数组内存分配失败\n");
        // 如果任一分配失败，需要释放所有已成功分配的内存，防止泄漏
        free(network->nodes);   // free(NULL)是安全的
        free(network->cities);
        free(network);
        fclose(fp);
        return NULL;
    }

    char line[512]; // 用于存储从文件中读取的每一行
    fgets(line, sizeof(line), fp); // 读取并丢弃CSV文件的第一行（表头）

    // 逐行读取文件内容
    while (fgets(line, sizeof(line), fp)) {
        // 跳过空行或以'#'开头的注释行
        if (line[0] == '#' || line[0] == '\n') continue;

        // --- 解析CSV行 ---
        char city_name[50] = {0};
        char type_str[20] = {0};
        char node_name[100] = {0};
        double lat = 0.0, lon = 0.0;
        
        // 使用sscanf从行中解析数据，%[^,]是一个匹配模式，表示读取直到逗号之前的所有字符
        int parsed = sscanf(line, "%49[^,],%19[^,],%99[^,],%lf,%lf", city_name, type_str, node_name, &lat, &lon);
        if (parsed != 5) continue; // 如果行格式不匹配（例如缺少字段），则跳过此行

        // --- 将字符串类型转换为枚举类型 ---
        NodeType ntype;
        if (strcmp(type_str, "landmark") == 0) ntype = NODE_TYPE_LANDMARK;
        else if (strcmp(type_str, "airport") == 0) ntype = NODE_TYPE_AIRPORT;
        else if (strcmp(type_str, "hsr") == 0) ntype = NODE_TYPE_HSR_STATION;
        else continue; // 如果是不支持的类型，则跳过

        // --- 查找或创建城市 ---
        int city_id = -1;
        // 遍历已有的城市，检查是否已存在
        for (int i = 0; i < network->city_count; i++) {
            if (strcmp(network->cities[i].city_name, city_name) == 0) {
                city_id = network->cities[i].city_id; // 如果找到，记录其ID
                break;
            }
        }
        // 如果city_id仍然是-1，说明这是一个新城市
        if (city_id == -1) {
            city_id = network->city_count; // 新城市的ID就是当前城市的数量
            // 检查城市数组是否已满
            if (network->city_count >= city_cap) {
                city_cap *= 2; // 如果已满，容量翻倍
                CityMeta* new_cities = realloc(network->cities, city_cap * sizeof(CityMeta));
                if (!new_cities) { /* 错误处理 */ traffic_network_destroy(network); fclose(fp); return NULL; }
                network->cities = new_cities; // 指向新的内存区域
            }
            // 添加新城市信息
            network->cities[city_id].city_id = city_id;
            // 使用strncpy防止缓冲区溢出
            strncpy(network->cities[city_id].city_name, city_name, sizeof(network->cities[city_id].city_name) - 1);
            network->cities[city_id].landmark_node_id = -1;
            network->cities[city_id].airport_node_id = -1;
            network->cities[city_id].hsr_node_id = -1;
            network->city_count++; // 城市数量加一
        }

        // --- 添加新节点 ---
        // 检查节点数组是否已满
        if (network->node_count >= node_cap) {
            node_cap *= 2; // 容量翻倍
            Node* new_nodes = realloc(network->nodes, node_cap * sizeof(Node));
            if (!new_nodes) { /* 错误处理 */ traffic_network_destroy(network); fclose(fp); return NULL; }
            network->nodes = new_nodes;
        }
        int node_id = network->node_count;
        // 使用C99的指定初始化器来设置新节点的值
        network->nodes[node_id] = (Node){.id = node_id, .city_id = city_id, .type = ntype, .latitude = lat, .longitude = lon};
        strncpy(network->nodes[node_id].name, node_name, sizeof(network->nodes[node_id].name) - 1);
        network->node_count++; // 节点数量加一

        // --- 更新城市的快速索引 ---
        // 如果该类型的枢纽尚未被记录，则记录此节点的ID
        if (ntype == NODE_TYPE_LANDMARK && network->cities[city_id].landmark_node_id == -1)
            network->cities[city_id].landmark_node_id = node_id;
        if (ntype == NODE_TYPE_AIRPORT && network->cities[city_id].airport_node_id == -1)
            network->cities[city_id].airport_node_id = node_id;
        if (ntype == NODE_TYPE_HSR_STATION && network->cities[city_id].hsr_node_id == -1)
            network->cities[city_id].hsr_node_id = node_id;
    }
    fclose(fp); // 关闭文件
    
    // 可以在这里添加realloc来缩减数组到实际大小，以节省内存，但为了简单起见省略了
    printf("成功加载 %d 个城市，%d 个节点\n", network->city_count, network->node_count);
    return network; // 返回创建好的网络对象
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
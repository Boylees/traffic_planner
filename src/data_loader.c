#include "traffic_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- 全局变量定义 ---
Node* graph_nodes = NULL;
int graph_node_count = 0;
CityMeta* city_meta_data = NULL;
int city_meta_count = 0;

void initialize_graph_data() {
    FILE* fp = fopen("data/nodes.csv", "r");
    if (fp == NULL) {
        fprintf(stderr, "错误：无法打开 data/nodes.csv 文件\n");
        fprintf(stderr, "请确保 data/nodes.csv 文件存在且格式正确\n");
        exit(1);
    }

    // --- 动态加载 CSV ---
    const int INITIAL_CAP_NODES = 256;
    const int INITIAL_CAP_CITIES = 128;
    graph_nodes = malloc(INITIAL_CAP_NODES * sizeof(Node));
    city_meta_data = malloc(INITIAL_CAP_CITIES * sizeof(CityMeta));
    int node_cap = INITIAL_CAP_NODES;
    int city_cap = INITIAL_CAP_CITIES;
    graph_node_count = 0;
    city_meta_count = 0;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        // 跳过注释或空行
        if (line[0] == '#' || line[0] == '\n') continue;

        char city_name[50] = {0};
        char type_str[20] = {0};
        char node_name[100] = {0};
        double lat = 0.0, lon = 0.0;

        // 解析 CSV 行: city,node_type,node_name,lat,lon
        int parsed = sscanf(line, " %49[^,] , %19[^,] , %99[^,] , %lf , %lf", city_name, type_str, node_name, &lat, &lon);
        if (parsed != 5) continue; // 格式不正确

        // 去除可能的首尾空格 (简单实现)
        char* p;
        p = city_name; while (*p==' '||*p=='\t') p++; memmove(city_name, p, strlen(p)+1);
        p = type_str;  while (*p==' '||*p=='\t') p++; memmove(type_str, p, strlen(p)+1);
        p = node_name; while (*p==' '||*p=='\t') p++; memmove(node_name, p, strlen(p)+1);
        for (int i=strlen(city_name)-1; i>=0 && (city_name[i]==' '||city_name[i]=='\t'||city_name[i]=='\r'||city_name[i]=='\n'); i--) city_name[i]='\0';
        for (int i=strlen(type_str)-1;  i>=0 && (type_str[i]==' '||type_str[i]=='\t'||type_str[i]=='\r'||type_str[i]=='\n');  i--) type_str[i]='\0';
        for (int i=strlen(node_name)-1; i>=0 && (node_name[i]==' '||node_name[i]=='\t'||node_name[i]=='\r'||node_name[i]=='\n'); i--) node_name[i]='\0';

        NodeType ntype;
        if (strcmp(type_str, "landmark") == 0) ntype = NODE_TYPE_LANDMARK;
        else if (strcmp(type_str, "airport") == 0) ntype = NODE_TYPE_AIRPORT;
        else if (strcmp(type_str, "hsr") == 0 || strcmp(type_str, "railway") == 0) ntype = NODE_TYPE_HSR_STATION;
        else continue;

        // ---- 查找或新增城市 ----
        int city_id = -1;
        for (int i = 0; i < city_meta_count; i++) {
            if (strcmp(city_meta_data[i].city_name, city_name) == 0) { city_id = city_meta_data[i].city_id; break; }
        }
        if (city_id == -1) {
            // 新增城市
            city_id = city_meta_count;
            if (city_meta_count >= city_cap) {
                city_cap *= 2;          // 扩容
                city_meta_data = realloc(city_meta_data, city_cap * sizeof(CityMeta));
            }
            city_meta_data[city_meta_count].city_id = city_id;
            strcpy(city_meta_data[city_meta_count].city_name, city_name);
            city_meta_data[city_meta_count].landmark_node_id = -1;
            city_meta_data[city_meta_count].airport_node_id = -1;
            city_meta_data[city_meta_count].hsr_node_id = -1;
            city_meta_count++;
        }

        // ---- 添加节点 ----
        if (graph_node_count >= node_cap) {
            node_cap *= 2;            // 扩容
            graph_nodes = realloc(graph_nodes, node_cap * sizeof(Node));
        }
        int node_id = graph_node_count;
        graph_nodes[node_id] = (Node){ node_id, city_id, ntype, "", lat, lon };
        strcpy(graph_nodes[node_id].name, node_name);
        graph_node_count++;

        // 更新城市索引记录 (保留第一个出现的作为代表)
        if (ntype == NODE_TYPE_LANDMARK && city_meta_data[city_id].landmark_node_id == -1)
            city_meta_data[city_id].landmark_node_id = node_id;
        if (ntype == NODE_TYPE_AIRPORT && city_meta_data[city_id].airport_node_id == -1)
            city_meta_data[city_id].airport_node_id = node_id;
        if (ntype == NODE_TYPE_HSR_STATION && city_meta_data[city_id].hsr_node_id == -1)
            city_meta_data[city_id].hsr_node_id = node_id;
    }
    fclose(fp);

    // 压缩到实际大小
    if (graph_node_count > 0) {
        graph_nodes = realloc(graph_nodes, graph_node_count * sizeof(Node));
    }
    if (city_meta_count > 0) {
        city_meta_data = realloc(city_meta_data, city_meta_count * sizeof(CityMeta));
    }

    printf("成功加载 %d 个城市，%d 个节点\n", city_meta_count, graph_node_count);
}

void free_graph_data() {
    free(graph_nodes);
    free(city_meta_data);
    graph_nodes = NULL;
    city_meta_data = NULL;
} 
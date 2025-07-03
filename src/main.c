#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "traffic_network.h"

static const char* mode_to_string(TransportMode mode) {
    switch (mode) {
        case DRIVING: return "driving";
        case HIGH_SPEED_RAIL: return "high_speed_rail";
        case FLIGHT: return "flight";
        case BUS: return "bus";
        default: return "unknown";
    }
}

static const char* mode_to_string_cn(TransportMode mode) {
    switch (mode) {
        case DRIVING: return "驾车";
        case HIGH_SPEED_RAIL: return "高铁";
        case FLIGHT: return "飞机";
        case BUS: return "公交";
        default: return "未知";
    }
}

static int find_city_meta_id_by_name(const char* name) {
    for (int i = 0; i < city_meta_count; i++) {
        if (strcmp(city_meta_data[i].city_name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int find_node_id_by_name(const char* name) {
    for (int i = 0; i < graph_node_count; i++) {
        if (strcmp(graph_nodes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void print_route_human_readable(const RoutePath* path) {
    if (!path || path->segment_count == 0) {
        printf("\n> 未能找到有效路径。\n");
        return;
    }
    printf("\n--- 规划结果 ---\n");
    PathSegment* current = path->segments_head;
    while(current){
        printf("  %s --(%s)--> %s\n",
               graph_nodes[current->from_node_id].name,
               mode_to_string_cn(current->mode),
               graph_nodes[current->to_node_id].name);
        current = current->next;
    }
    printf("--- 总计: 距离 %.1fkm, 时间 %.2fh, 成本 %.2f元 ---\n",
           path->total_distance, path->total_time, path->total_cost);
}

static void print_route_json(const RoutePath* path) {
    if (!path) {
        printf("{\"error\": \"No route found.\"}\n");
        return;
    }
    printf("{\n  \"route\": {\n    \"segments\": [\n");
    PathSegment* current = path->segments_head;
    while (current) {
        printf("      {\n");
        printf("        \"from\": \"%s\",\n", graph_nodes[current->from_node_id].name);
        printf("        \"to\": \"%s\",\n", graph_nodes[current->to_node_id].name);
        printf("        \"mode\": \"%s\",\n", mode_to_string(current->mode));
        printf("        \"distance_km\": %.2f,\n", current->distance_km);
        printf("        \"time_hours\": %.2f,\n", current->time_hours);
        printf("        \"cost_yuan\": %.2f\n", current->cost_yuan);
        printf("      }%s\n", current->next ? "," : "");
        current = current->next;
    }
    printf("    ],\n");
    printf("    \"total_time_hours\": %.2f,\n", path->total_time);
    printf("    \"total_cost_yuan\": %.2f,\n", path->total_cost);
    printf("    \"total_distance_km\": %.2f\n", path->total_distance);
    printf("  }\n}\n");
}

void handle_single_path_planning() {
    char start_name[100], end_name[100];
    printf("请输入起点地标: ");
    scanf("%99s", start_name);
    printf("请输入终点地标: ");
    scanf("%99s", end_name);

    int start_node_id = find_node_id_by_name(start_name);
    int end_node_id = find_node_id_by_name(end_name);

    if (start_node_id == -1 || end_node_id == -1) {
        printf("错误: 未找到输入的地标名称。\n");
        return;
    }

    double time_w, cost_w;
    printf("请输入时间权重 (0.0-1.0): ");
    scanf("%lf", &time_w);
    printf("请输入成本权重 (0.0-1.0): ");
    scanf("%lf", &cost_w);

    RoutePath* path = find_shortest_path(start_node_id, end_node_id, time_w, cost_w);
    print_route_human_readable(path);
    printf("\n--- JSON格式输出 ---\n");
    print_route_json(path);
    free_route_path(path);
}

void handle_tsp_planning() {
    int node_ids[20];
    int count = 0;
    char node_name[100];

    printf("请输入要经过的地标列表 (起点为第一个, 输入 'done' 结束):\n");
    while (count < 10) {
        printf("地标 %d: ", count + 1);
        scanf("%s", node_name);
        if (strcmp(node_name, "done") == 0) break;

        int id = find_node_id_by_name(node_name);
        if (id != -1) {
            node_ids[count++] = id;
        } else {
            printf("未找到地标: %s\n", node_name);
        }
    }

    if (count < 2) { printf("错误: TSP需要至少2个地标。\n"); return; }

    double time_w, cost_w;
    printf("请输入时间权重 (0.0-1.0): "); scanf("%lf", &time_w);
    printf("请输入成本权重 (0.0-1.0): "); scanf("%lf", &cost_w);

    printf("\n正在计算TSP路径，请稍候...\n");
    RoutePath* path = solve_tsp(node_ids, count, time_w, cost_w);
    print_route_human_readable(path);
    printf("\n--- JSON格式输出 ---\n");
    print_route_json(path);
    free_route_path(path);
}


int main() {
    initialize_graph_data();

#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    int choice;
    while (1) {
        printf("\n========== 交通网络路径规划系统 ==========\n");
        printf("1. 单点路径规划\n");
        printf("2. 多点旅行规划 (TSP)\n");
        printf("3. 退出\n");
        printf("请选择功能: ");

        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            choice = 0;
        }

        switch (choice) {
            case 1: handle_single_path_planning(); break;
            case 2: handle_tsp_planning(); break;
            case 3: printf("感谢使用！\n"); goto end;
            default: printf("无效输入，请输入1-3之间的数字。\n");
        }
    }

end:
    free_graph_data();
    return 0;
} 
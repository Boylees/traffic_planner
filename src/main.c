/**
 * @file main.c
 * @brief 程序的主入口和用户交互界面。
 * @details 负责初始化程序、处理用户输入、调用核心功能并最终清理资源。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 包含所有模块的头文件
#include "graph.h"
#include "pathfinding.h"
#include "visualization.h"
#include "types.h"
#include "utils.h"

/**
 * @brief 以人类可读的格式打印规划好的路径。
 * @param network 交通网络对象，用于获取节点名称。
 * @param path 要打印的路径。
 */
static void print_route_human_readable(const TrafficNetwork *network, const RoutePath *path)
{
    if (!path || path->segment_count == 0)
    {
        printf("\n> 未能找到有效路径。\n");
        return;
    }
    printf("\n--- 规划结果 ---\n");
    PathSegment *current = path->segments_head;
    while (current)
    {
        const Node *from = traffic_network_get_node_by_id(network, current->from_node_id);
        const Node *to = traffic_network_get_node_by_id(network, current->to_node_id);
        if (from && to)
        {
            printf("  %s --(%s)--> %s\n",
                   from->name,
                   mode_to_string_cn(current->mode),
                   to->name);
        }
        current = current->next;
    }
    printf("--- 总计: 距离 %.1fkm, 时间 %.2fh, 成本 %.2f元 ---\n",
           path->total_distance, path->total_time, path->total_cost);
}

/**
 * @brief 处理单点路径规划的用户交互逻辑。
 * @param network 交通网络对象。
 */
void handle_single_path_planning(const TrafficNetwork *network)
{
    char start_name[100], end_name[100];
    printf("请输入起点地标: ");
    scanf("%99s", start_name);
    printf("请输入终点地标: ");
    scanf("%99s", end_name);

    int start_node_id = traffic_network_find_node_id_by_name(network, start_name);
    int end_node_id = traffic_network_find_node_id_by_name(network, end_name);

    if (start_node_id == -1 || end_node_id == -1)
    {
        printf("错误: 未找到输入的地标名称。\n");
        return;
    }

    double time_w, cost_w;
    printf("请输入时间权重 (0.0-1.0): ");
    scanf("%lf", &time_w);
    printf("请输入成本权重 (0.0-1.0): ");
    scanf("%lf", &cost_w);

    // 调用核心寻路函数
    RoutePath *path = find_shortest_path(network, start_node_id, end_node_id, time_w, cost_w);

    // 输出结果
    print_route_human_readable(network, path);
    generate_html_visualization(network, path);

    // 释放路径资源
    free_route_path(path);
}

/**
 * @brief 处理TSP的用户交互逻辑。
 * @param network 交通网络对象。
 */
void handle_tsp_planning(const TrafficNetwork *network)
{
    int node_ids[20];
    int count = 0;
    char node_name[100];

    printf("请输入要经过的地标列表 (起点为第一个, 输入 'done' 结束):\n");
    while (count < 10)
    {
        printf("地标 %d: ", count + 1);
        scanf("%s", node_name);
        if (strcmp(node_name, "done") == 0)
            break;

        int id = traffic_network_find_node_id_by_name(network, node_name);
        if (id != -1)
        {
            node_ids[count++] = id;
        }
        else
        {
            printf("未找到地标: %s\n", node_name);
        }
    }

    if (count < 2)
    {
        printf("错误: TSP需要至少2个地标。\n");
        return;
    }

    double time_w, cost_w;
    printf("请输入时间权重 (0.0-1.0): ");
    scanf("%lf", &time_w);
    printf("请输入成本权重 (0.0-1.0): ");
    scanf("%lf", &cost_w);

    printf("\n正在计算TSP路径，请稍候...\n");
    RoutePath *path = solve_tsp(network, node_ids, count, time_w, cost_w);

    print_route_human_readable(network, path);
    generate_html_visualization(network, path);
    free_route_path(path);
}

/**
 * @brief 处理顺序路径规划的用户交互逻辑。
 * @param network 交通网络对象。
 */
void handle_sequential_planning(const TrafficNetwork *network)
{
    int node_ids[20];
    int count = 0;
    char node_name[100];

    printf("请输入要依次经过的地标 (输入 'done' 结束):\n");
    while (count < 20)
    {
        printf("地标 %d: ", count + 1);
        scanf("%s", node_name);
        if (strcmp(node_name, "done") == 0)
            break;

        int id = traffic_network_find_node_id_by_name(network, node_name);
        if (id != -1)
        {
            node_ids[count++] = id;
        }
        else
        {
            printf("未找到地标: %s\n", node_name);
        }
    }

    if (count < 2)
    {
        printf("错误: 顺序路径规划需要至少2个地标（一个起点和一个终点）。\n");
        return;
    }

    double time_w, cost_w;
    printf("请输入时间权重 (0.0-1.0): ");
    scanf("%lf", &time_w);
    printf("请输入成本权重 (0.0-1.0): ");
    scanf("%lf", &cost_w);

    printf("\n正在计算顺序路径，请稍候...\n");
    RoutePath *path = find_sequential_path(network, node_ids, count, time_w, cost_w);

    print_route_human_readable(network, path);
    if (path)
    {
        generate_html_visualization(network, path);
    }
    free_route_path(path);
}

// 程序主函数
int main()
{
    // 1. 创建并加载交通网络数据
    // network对象现在是数据的唯一所有者
    TrafficNetwork *network = traffic_network_create("data/nodes.csv");
    if (!network)
    {
        return 1; // 如果加载失败，程序退出
    }

    // 在Windows环境下，设置控制台代码页为UTF-8以正确显示中文
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif

    // 2. 主事件循环
    int choice;
    while (1)
    {
        printf("\n========== 交通网络路径规划系统 ==========\n");
        printf("1. 单点路径规划\n");
        printf("2. 多点旅行规划 (TSP)\n");
        printf("3. 顺序路径规划\n");
        printf("4. 退出\n");
        printf("请选择功能: ");

        // 读取用户输入，并处理无效输入
        if (scanf("%d", &choice) != 1)
        {
            // 清空输入缓冲区，防止无限循环
            while (getchar() != '\n')
                ;
            choice = 0; // 设为无效选项
        }

        // 根据用户选择调用对应的处理函数
        // 注意：network对象作为参数被传递下去
        switch (choice)
        {
        case 1:
            handle_single_path_planning(network);
            break;
        case 2:
            handle_tsp_planning(network);
            break;
        case 3:
            handle_sequential_planning(network);
            break;
        case 4:
            printf("感谢使用！\n");
            goto end; // 跳转到清理步骤
        default:
            printf("无效输入，请输入1-4之间的数字。\n");
        }
    }

end:
    // 3. 释放所有资源
    traffic_network_destroy(network);
    return 0;
}
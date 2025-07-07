#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include "graph.h"
#include "types.h"

/**
 * @brief 根据给定的路径，生成一个包含交互式地图的HTML文件。
 * @details 使用Leaflet.js库在地图上绘制路径的各个节点和线段，并用不同颜色区分交通方式。
 *          生成的文件名为 `route_visualization.html`，会覆盖同名旧文件。
 * 
 * @param network 指向交通网络实例的只读指针，用于获取节点信息。
 * @param path 指向包含路径信息的RoutePath对象的只读指针。
 */
void generate_html_visualization(const TrafficNetwork* network, const RoutePath* path);

#endif // VISUALIZATION_H 
/**
 * @file visualization.c
 * @brief 实现了将规划好的路径输出为HTML交互式地图的功能。
 */
#include "visualization.h"
#include "graph.h"
#include "types.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief 根据交通方式返回对应的颜色字符串（用于地图绘制）。
 * @param mode 交通方式枚举。
 * @return const char* CSS颜色代码字符串。
 */
static const char* get_color_for_mode(TransportMode mode) {
    switch (mode) {
        case DRIVING:           return "#4A90E2"; // 鲜艳的蓝色
        case HIGH_SPEED_RAIL:   return "#50E3C2"; // 青绿色
        case FLIGHT:            return "#F5A623"; // 橙黄色
        case BUS:               return "#7ED321"; // 鲜绿色
        default:                return "#9B9B9B"; // 默认灰色
    }
}

/**
 * @brief 将节点类型枚举转换为字符串，用于JS调用。
 * @param type 节点类型枚举
 * @return const char* 对应的字符串
 */
static const char* node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_TYPE_AIRPORT: return "airport";
        case NODE_TYPE_HSR_STATION: return "hsr";
        default: return "landmark";
    }
}

// generate_html_visualization 函数的实现，接口注释在 visualization.h 中
void generate_html_visualization(const TrafficNetwork* network, const RoutePath* path) {
    // 如果路径无效或为空，则不生成文件
    if (!path || !path->segments_head) {
        printf("Path is empty, not generating visualization file.\n");
        return;
    }

    // 以写入模式打开文件，如果文件已存在则会覆盖
    FILE* fp = fopen("route_visualization.html", "w");
    if (!fp) {
        fprintf(stderr, "Error: Cannot create HTML visualization file.\n");
        return;
    }

    // --- 写入HTML文件头部 ---
    fprintf(fp, "<!DOCTYPE html>\n<html lang=\"zh\">\n<head>\n");
    fprintf(fp, "    <meta charset=\"UTF-8\">\n");
    fprintf(fp, "    <title>路径规划可视化</title>\n");
    fprintf(fp, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(fp, "    <link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\" />\n");
    fprintf(fp, "    <script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\"></script>\n");
    // --- 写入内联CSS样式 ---
    fprintf(fp, "    <style>\n");
    fprintf(fp, "        body { margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, \"Helvetica Neue\", Arial, sans-serif; }\n");
    fprintf(fp, "        #map { height: 100vh; width: 100vw; } /* Make map fill the viewport */\n");
    fprintf(fp, "        .summary-box { position: absolute; top: 10px; left: 10px; z-index: 1000; background: rgba(255,255,255,0.9); padding: 10px 15px; border-radius: 8px; box-shadow: 0 1px 7px rgba(0,0,0,0.3); max-width: 350px; max-height: 90vh; overflow-y: auto; }\n");
    fprintf(fp, "        .summary-box h4 { margin: 0 0 10px; text-align: center; font-weight: bold; color: #000; border-bottom: 1px solid #ccc; padding-bottom: 8px; }\n");
    fprintf(fp, "        .summary-box p { margin: 4px 0; font-size: 13px; color: #333; line-height: 1.4; }\n");
    fprintf(fp, "        .summary-box p b { min-width: 70px; display: inline-block; font-weight: bold; }\n");
    fprintf(fp, "        .summary-box .segment { border-top: 1px dashed #ddd; padding-top: 8px; margin-top: 8px; }\n");
    fprintf(fp, "        .summary-box .total { font-weight: bold; border-top: 2px solid #333; padding-top: 8px; margin-top: 8px; }\n");
    fprintf(fp, "        .legend { padding: 10px; font-size: 14px; background: rgba(255,255,255,0.85); box-shadow: 0 0 15px rgba(0,0,0,0.2); border-radius: 5px; line-height: 1.5; color: #333; }\n");
    fprintf(fp, "        .legend h4 { margin: 0 0 8px; color: #000; text-align: center; font-weight: bold; }\n");
    fprintf(fp, "        .legend .legend-item { display: flex; align-items: center; height: 22px; margin-bottom: 2px;}\n");
    fprintf(fp, "        .legend .legend-item i { width: 18px; height: 18px; margin-right: 8px; opacity: 0.9; flex-shrink: 0; border: 1px solid rgba(0,0,0,0.2);}\n");
    fprintf(fp, "        .leaflet-popup-content-wrapper { border-radius: 5px; }\n");
    fprintf(fp, "        .leaflet-popup-content b { color: #333; }\n");
    fprintf(fp, "        .leaflet-popup-content p { margin: 5px 0; }\n");
    fprintf(fp, "    </style>\n");
    fprintf(fp, "</head>\n<body>\n\n");

    // --- 写入行程摘要 ---
    fprintf(fp, "<div class=\"summary-box\">\n");
    fprintf(fp, "    <h4>行程摘要</h4>\n");
    
    // 写入每一段的详细信息
    PathSegment* current_segment = path->segments_head;
    while (current_segment) {
        const Node* from = traffic_network_get_node_by_id(network, current_segment->from_node_id);
        const Node* to = traffic_network_get_node_by_id(network, current_segment->to_node_id);
        if(from && to) {
            fprintf(fp, "    <div class=\"segment\">\n");
            fprintf(fp, "        <p><b>出发:</b> %s</p>\n", from->name);
            fprintf(fp, "        <p><b>到达:</b> %s</p>\n", to->name);
            fprintf(fp, "        <p><b>方式:</b> %s</p>\n", mode_to_string_cn(current_segment->mode));
            fprintf(fp, "        <p><b>详情:</b> %.1f 公里, %.2f 小时, %.2f 元</p>\n", current_segment->distance_km, current_segment->time_hours, current_segment->cost_yuan);
            fprintf(fp, "    </div>\n");
        }
        current_segment = current_segment->next;
    }

    // 写入总计信息
    fprintf(fp, "    <div class=\"total\">\n");
    fprintf(fp, "        <p><b>总距离:</b> %.1f 公里</p>\n", path->total_distance);
    fprintf(fp, "        <p><b>总时间:</b> %.2f 小时</p>\n", path->total_time);
    fprintf(fp, "        <p><b>总花费:</b> %.2f 元</p>\n", path->total_cost);
    fprintf(fp, "    </div>\n");
    fprintf(fp, "</div>\n\n");
    

    fprintf(fp, "<div id=\"map\"></div>\n\n<script>\n");

    // --- 写入JavaScript代码 ---
    // 1. 初始化地图
    fprintf(fp, "    const map = L.map('map').setView([35.8617, 104.1954], 5);\n");
    // 2. 添加底图图层
    fprintf(fp, "    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {\n");
    fprintf(fp, "        attribution: '&copy; <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors'\n");
    fprintf(fp, "    }).addTo(map);\n\n");

    // --- 新增：加载并绘制中国省界 ---
    fprintf(fp, "    fetch('https://geo.datav.aliyun.com/areas_v3/bound/100000_full.json')\n");
    fprintf(fp, "      .then(res => res.json())\n");
    fprintf(fp, "      .then(data => {\n");
    fprintf(fp, "        L.geoJSON(data, {\n");
    fprintf(fp, "          style: {\n");
    fprintf(fp, "            color: '#666',\n");
    fprintf(fp, "            weight: 1,\n");
    fprintf(fp, "            opacity: 0.8,\n");
    fprintf(fp, "            fillColor: '#888',\n");
    fprintf(fp, "            fillOpacity: 0.1\n");
    fprintf(fp, "          }\n");
    fprintf(fp, "        }).addTo(map);\n");
    fprintf(fp, "      });\n\n");

    // 3. 定义自定义图标
    fprintf(fp, "    const landmarkIcon = new L.Icon({ iconUrl: 'data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCIgZmlsbD0iIzAwNzhmZiIgd2lkdGg9IjMycHgiIGhlaWdodD0iMzJweCI+PHBhdGggZD0iTTEyIDJDOC4xMyAyIDUgNS4xMyA1IDljMCA1LjI1IDcgMTMgNyAxM3M3LTcuNzUgNy0xM0MxOSAxMyAxMiAyem0wIDkuNWMtMS4zOCAwLTIuNS0xLjEyLTIuNS0yLjVzMS4xMi0yLjUgMi41LTIuNSAyLjUgMS4xMiAyLjUgMi41LTEuMTIgMi41LTIuNSAyLjV6Ii8+PC9zdmc+', iconSize: [32, 32], iconAnchor: [16, 32], popupAnchor: [0, -32] });\n");
    fprintf(fp, "    const airportIcon = new L.Icon({ iconUrl: 'data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIGhlaWdodD0iMjRweCIgdmlld0JveD0iMCAwIDI0IDI0IiB3aWR0aD0iMjRweCIgZmlsbD0iIzAwMDAwMCI+PHBhdGggZD0iTTAsMCBIMjRWMEgyNEwwLDAgWiIgZmlsbD0ibm9uZSIvPjxwYXRoIGQ9Ik0yMSw5LjVjMC0uODMtLjY3LTEuNS0xLjUtMS41SDUuNjFMMzgsNkgxNFY0YzAtLjU1LTAuNDUtMS0xLTFIMTAuNWwtMiwySDd2Mi41bC0yLDIvMTAuNSwzLjUgVjIxaDJ2LTJsMS41LTEuNUgyMC41QzIwLjY3LDE2LjUgMjEsMTYuMzMgMjEsMTYuMTZWMTAuNUwyMSw5LjV6Ii8+PC9zdmc+', iconSize: [28, 28], iconAnchor: [14, 14] });\n");
    fprintf(fp, "    const hsrIcon = new L.Icon({ iconUrl: 'data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIGVuYWJsZS1iYWNrZ3JvdW5kPSJuZXcgMCAwIDI0IDI0IiBoZWlnaHQ9IjI0cHgiIHZpZXdCb3g9IjAgMCAyNCAyNCIgd2lkdGg9IjI0cHgiIGZpbGw9IiMwMDAwMDAiPjxnLz48Zz48cGF0aCBkPSJNMiwxNmgyMGMyLjIyLDAsNC0xLjc4LDQtNFY0YzAtMS4zLTAuODEtMi40My0yLTIuODJWNEgyVjkuMTdDMy4xOSw5LjU3LDQsMTAuNyw0LDEycy0wLjgxLDIuNDMtMiwyLjg0VjE2eiBNMTgsOUg2VjVINzhWOUg2djJoMTJWOUwxOCw5eiIvPjxwYXRoIGQ9Ik0xOCwxOFYxM0g2djVDNC4xNywxMywzLDE0Ljc4LDMsMTZoMThjMC0xLjIyLTEuMTctMy0zLTN6IE0xMS41LDE3LjVjLTAuODMsMC0xLjUtMC42Ny0xLjUtMS41czAuNjctMS41LDEuNS0xLjVTMTIuNSwxNS4xNywxMi41LDE2UzEyLjMzLDE3LjUsMTEuNSwxNy41eiBNMTYuNSwxNy41Yy0wLjg0LDAtMS41LTAuNjctMS41LTEuNXMwLjY2LTEuNSwxLjUtMS41czEuNSwwLjY3LDEuNSwxLjVTLTkuODMsMTcuNSwxNi41LDE3LjV6Ii8+PC9nPg0KPC9zdmc+', iconSize: [28, 28], iconAnchor: [14, 14] });\n");
    fprintf(fp, "    function getIcon(nodeType) { switch(nodeType) { case 'airport': return airportIcon; case 'hsr': return hsrIcon; default: return landmarkIcon; } }\n\n");

    // 追踪已绘制的节点，避免在地图上重复添加同一个地点的标记
    bool* drawn_nodes = calloc(traffic_network_get_node_count(network), sizeof(bool));
    if (!drawn_nodes) {
        fprintf(stderr, "Error: Cannot allocate memory for visualization.\n");
        fclose(fp);
        return;
    }

    // 4. 遍历路径，绘制线段和标记
    fprintf(fp, "    const bounds = L.latLngBounds();\n");
    PathSegment* current = path->segments_head;
    while (current) {
        // 获取当前路段的起点和终点节点信息
        const Node* from_node = traffic_network_get_node_by_id(network, current->from_node_id);
        const Node* to_node = traffic_network_get_node_by_id(network, current->to_node_id);

        if (!from_node || !to_node) {
            current = current->next;
            continue; // 如果节点无效，跳过此路段
        }

        // 绘制连接起点和终点的折线，并绑定信息弹窗
        fprintf(fp, "    L.polyline([[%f, %f], [%f, %f]], { color: '%s', weight: 5, opacity: 0.8 }).addTo(map).bindPopup('<b>%s 到 %s</b><p>方式: %s</p><p>距离: %.1f 公里</p><p>时间: %.2f 小时</p><p>花费: %.2f 元</p>');\n",
                from_node->latitude, from_node->longitude,
                to_node->latitude, to_node->longitude,
                get_color_for_mode(current->mode),
                from_node->name, to_node->name,
                mode_to_string_cn(current->mode),
                current->distance_km, current->time_hours, current->cost_yuan);
        
        // 如果起点尚未被绘制，则在地图上添加带自定义图标的标记，并绑定悬浮提示
        if (!drawn_nodes[current->from_node_id]) {
            fprintf(fp, "    L.marker([%f, %f], { icon: getIcon('%s') }).addTo(map).bindTooltip('%s');\n", from_node->latitude, from_node->longitude, node_type_to_string(from_node->type), from_node->name);
            drawn_nodes[current->from_node_id] = true; // 标记为已绘制
        }
        // 对终点做同样的处理
        if (!drawn_nodes[current->to_node_id]) {
            fprintf(fp, "    L.marker([%f, %f], { icon: getIcon('%s') }).addTo(map).bindTooltip('%s');\n", to_node->latitude, to_node->longitude, node_type_to_string(to_node->type), to_node->name);
            drawn_nodes[current->to_node_id] = true;
        }

        // 将当前路段的两个端点加入边界对象
        fprintf(fp, "    bounds.extend([%f, %f]);\n", from_node->latitude, from_node->longitude);
        fprintf(fp, "    bounds.extend([%f, %f]);\n", to_node->latitude, to_node->longitude);
        
        current = current->next;
    }
    
    // 5. 让地图自动缩放和平移，以最佳视野显示所有路径
    fprintf(fp, "\n    if (bounds.isValid()) { map.fitBounds(bounds, { padding: [50, 50] }); }\n\n");
    
    // 6. 添加图例控件
    fprintf(fp, "    const legend = L.control({position: 'bottomright'});\n");
    fprintf(fp, "    legend.onAdd = function (map) {\n");
    fprintf(fp, "        const div = L.DomUtil.create('div', 'info legend');\n");
    fprintf(fp, "        const modes = [{mode: '驾车', color: '%s'}, {mode: '高铁', color: '%s'}, {mode: '飞机', color: '%s'}, {mode: '公交', color: '%s'}];\n", get_color_for_mode(DRIVING), get_color_for_mode(HIGH_SPEED_RAIL), get_color_for_mode(FLIGHT), get_color_for_mode(BUS));
    fprintf(fp, "        div.innerHTML = '<h4>图例</h4>';\n");
    fprintf(fp, "        for (let i = 0; i < modes.length; i++) {\n");
    fprintf(fp, "            div.innerHTML += '<div class=\"legend-item\"><i style=\"background:' + modes[i].color + '\"></i>' + modes[i].mode + '</div>';\n");
    fprintf(fp, "        }\n");
    fprintf(fp, "        return div;\n");
    fprintf(fp, "    };\n");
    fprintf(fp, "    legend.addTo(map);\n");

    // 结束JavaScript脚本和HTML文档
    fprintf(fp, "</script>\n\n</body>\n</html>\n");
    fclose(fp);
    free(drawn_nodes); // 释放用于追踪节点的临时数组

    printf("\nRoute visualization generated: route_visualization.html\n");
} 
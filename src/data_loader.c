#include "traffic_network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- 全局变量定义 ---
Node* graph_nodes = NULL;
int graph_node_count = 0;
CityMeta* city_meta_data = NULL;
int city_meta_count = 0;

// 原始城市数据结构
typedef struct {
    char name[50];
    char landmark[50]; double lat_land, lon_land;
    bool has_airport; char airport[50]; double lat_air, lon_air;
    bool has_railway; char railway[50]; double lat_hsr, lon_hsr;
} RawCityData;

// 50个城市的原始数据（经纬度为近似值，便于示例展示）
static const RawCityData raw_data[] = {
    // 城市   地标名     地标坐标        机场标志          机场坐标            高铁标志        高铁站坐标
    {"北京",   "故宫",      39.9163,116.3972, true,  "首都国际机场",   40.0801,116.5845,  true,  "北京南站",    39.8652,116.3786},
    {"上海",   "外滩",      31.2393,121.4839, true,  "虹桥国际机场",   31.1979,121.3363,  true,  "上海虹桥站",  31.1946,121.3267},
    {"广州",   "广州塔",    23.1065,113.3246, true,  "白云国际机场",   23.3924,113.2988,  true,  "广州南站",    22.9904,113.2642},
    {"深圳",   "世界之窗",  22.5371,113.9715, true,  "宝安国际机场",   22.6392,113.8106,  true,  "深圳北站",    22.6103,114.0303},
    {"成都",   "宽窄巷子",  30.6700,104.0600, true,  "双流国际机场",   30.5785,103.9471,  true,  "成都东站",    30.6593,104.1413},
    {"西安",   "兵马俑",    34.3851,109.2792, true,  "咸阳国际机场",   34.4471,108.7523,  true,  "西安北站",    34.3789,108.9203},
    {"杭州",   "西湖",      30.2460,120.1552, true,  "萧山国际机场",   30.2295,120.4342,  true,  "杭州东站",    30.2931,120.2152},
    {"武汉",   "黄鹤楼",    30.5463,114.2934, true,  "天河国际机场",   30.7831,114.2085,  true,  "武汉站",      30.6100,114.4239},
    {"重庆",   "洪崖洞",    29.5630,106.5516, true,  "江北国际机场",   29.7195,106.6417,  true,  "重庆西站",    29.5085,106.4560},
    {"长沙",   "橘子洲",    28.1947,112.9828, true,  "黄花国际机场",   28.1892,113.2196,  true,  "长沙南站",    28.1518,113.0612},
    // 无高铁站示例
    {"三门峡", "天鹅湖",    34.7726,111.1813, true,  "三门峡机场",     34.5150,111.1000,  false, "",             0,0},
    // 无机场示例
    {"苏州",   "拙政园",    31.3233,120.6267, false, "",               0,0,              true,  "苏州站",      31.3210,120.6190},
    // 无机场/无高铁站示例
    {"丽水",   "缙云仙都",  28.4563,119.9220, false, "",               0,0,              false, "",             0,0},
    {"天津",   "天津之眼",  39.1423,117.1767, true,  "滨海国际机场",   39.1249,117.3624,  true,  "天津西站",    39.1556,117.1593},
    {"南京",   "夫子庙",    32.0293,118.7881, true,  "禄口国际机场",   31.7420,118.8622,  true,  "南京南站",    31.9867,118.7954},
    {"哈尔滨", "中央大街",  45.7567,126.6424, true,  "太平国际机场",   45.6234,126.2503,  true,  "哈尔滨西站",  45.6787,126.6077},
    {"青岛",   "栈桥",      36.0671,120.3826, true,  "胶东国际机场",   36.2715,120.3740,  true,  "青岛北站",    36.1750,120.3730},
    {"乌鲁木齐", "红山公园", 43.8280,87.6170,  true, "地窝堡国际机场", 43.9071,87.4742,  true,  "乌鲁木齐站",  43.7940,87.5650},
    {"拉萨",   "布达拉宫",  29.6510,91.1180,  true, "贡嘎机场",       29.2978,90.9119,   true,  "拉萨站",      29.6390,91.1511},
    {"昆明",   "滇池",      24.8822,102.7123, true,  "长水国际机场",   25.1019,102.9292,  true,  "昆明南站",    24.9196,102.6200},
    {"贵阳",   "甲秀楼",    26.5711,106.7076, true,  "龙洞堡机场",     26.5385,106.8012,  true,  "贵阳北站",    26.6449,106.7087},
    {"兰州",   "中山桥",    36.0613,103.8343, true,  "中川机场",       36.5152,103.6200,  true,  "兰州西站",    36.0570,103.6900},
    {"西宁",   "塔尔寺",    36.5023,101.5692, true,  "曹家堡机场",     36.5276,102.0431,  true,  "西宁站",      36.6285,101.7574},
    {"太原",   "晋祠",      37.7310,112.4700, true,  "武宿机场",       37.7485,112.6283,  true,  "太原南站",    37.7643,112.6640},
    {"郑州",   "少林寺",    34.7466,113.6254, true,  "新郑机场",       34.5190,113.8400,  true,  "郑州东站",    34.7858,113.7312},
    {"石家庄", "正定古城",  38.0428,114.5140, true,  "正定机场",       38.2800,114.6960,  true,  "石家庄站",    38.0423,114.4990},
    {"福州",   "三坊七巷",  26.0858,119.2965, true,  "长乐机场",       25.9342,119.6632,  true,  "福州站",      26.0580,119.3100},
    {"厦门",   "鼓浪屿",    24.4798,118.0894, true,  "高崎机场",       24.5449,118.1270,  true,  "厦门北站",    24.7215,118.0322},
    {"南昌",   "滕王阁",    28.6829,115.8582, true,  "昌北机场",       28.8650,115.8999,  true,  "南昌西站",    28.6466,115.8054},
    {"合肥",   "包公园",    31.8613,117.2856, true,  "新桥机场",       31.9912,116.9740,  true,  "合肥南站",    31.8206,117.3389},
    {"宁波",   "天一阁",    29.8683,121.5440, true,  "栎社机场",       29.8267,121.4619,  true,  "宁波站",      29.8668,121.5443},
    {"济南",   "趵突泉",    36.6759,117.0009, true,  "遥墙机场",       36.8572,117.2145,  true,  "济南西站",    36.6824,116.8752},
    {"沈阳",   "故宫",      41.7957,123.4328, true,  "桃仙机场",       41.6418,123.4840,  true,  "沈阳北站",    41.8138,123.4331},
    {"大连",   "星海广场",  38.8785,121.5500, true,  "周水子机场",     38.9657,121.5381,  true,  "大连北站",    38.9489,121.6226},
    {"海口",   "骑楼老街",  20.0440,110.3249, true,  "美兰机场",       19.9349,110.4584,  true,  "海口东站",    20.0448,110.3612},
    {"三亚",   "亚龙湾",    18.2528,109.5120, true,  "凤凰机场",       18.3026,109.4123,  true,  "三亚站",      18.2625,109.4990},
    {"南宁",   "青秀山",    22.8167,108.3833, true,  "吴圩机场",       22.6083,108.1714,  true,  "南宁东站",    22.8130,108.3730},
    {"桂林",   "漓江",      25.2736,110.2900, true,  "两江机场",       25.2181,110.0392,  true,  "桂林北站",    25.3081,110.3186},
    {"珠海",   "长隆海洋",  22.1163,113.5767, true,  "金湾机场",       22.0064,113.3760,  false, "",             0,0},
    {"澳门",   "大三巴",    22.1987,113.5439, true,  "澳门机场",       22.1496,113.5916,  false, "",             0,0},
    {"香港",   "维多利亚港",22.2830,114.1588, true,  "赤鱲角机场",     22.3080,113.9185,  true,  "西九龙站",    22.3040,114.1600},
    {"温州",   "江心屿",    27.9960,120.6994, true,  "龙湾机场",       27.9127,120.8522,  true,  "温州南站",    27.9300,120.7160},
    {"银川",   "沙湖",      38.4672,106.2737, true,  "河东机场",       38.3223,106.3955,  true,  "银川站",      38.4920,106.1840},
    {"呼和浩特", "大召寺", 40.8118,111.6586, true,  "白塔机场",       40.8514,111.8240,  true,  "呼和浩特东站",40.8218,111.6710},
    {"大庆",   "铁人广场",  46.5977,125.0000, false, "",               0,0,              true,  "大庆站",      46.5974,125.1031},
    {"宜昌",   "三峡大坝",  30.6919,111.2865, true,  "三峡机场",       30.5555,111.4848,  false, "",             0,0},
    {"自贡",   "恐龙博物馆",29.3390,104.7784, false, "",               0,0,              false, "",             0,0},
    {"扬州",   "瘦西湖",    32.3942,119.4358, false, "",               0,0,              true,  "扬州东站",    32.3750,119.4600},
    {"义乌",   "国际商贸城",29.3060,120.0768, false,"",               0,0,              true,  "义乌站",      29.3550,120.0695},
    {"泉州",   "清源山",    24.9151,118.5858, true,  "晋江机场",       24.7964,118.5893,  true,  "泉州站",      24.8960,118.6000},
    {"岳阳",   "岳阳楼",    29.3573,113.1292, false, "",               0,0,              true,  "岳阳东站",    29.4710,113.1120},
    {"九江",   "庐山",      29.7060,116.0019, false, "",               0,0,              true,  "九江站",      29.7040,115.9900}
};


void initialize_graph_data() {
    FILE* fp = fopen("data/nodes.csv", "r");
    if (fp != NULL) {
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
                    city_cap *= 2;
                    city_meta_data = realloc(city_meta_data, city_cap * sizeof(CityMeta));
                }
                city_meta_data[city_meta_count].city_id = city_meta_count;
                strcpy(city_meta_data[city_meta_count].city_name, city_name);
                city_meta_data[city_meta_count].landmark_node_id = -1;
                city_meta_data[city_meta_count].airport_node_id = -1;
                city_meta_data[city_meta_count].hsr_node_id = -1;
                city_meta_count++;
            }

            // ---- 添加节点 ----
            if (graph_node_count >= node_cap) {
                node_cap *= 2;
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
        return; // CSV 加载完成
    }

    // ---- 若 CSV 不存在则退回之前的静态内置数据逻辑 ----
    city_meta_count = sizeof(raw_data) / sizeof(RawCityData);
    // 统计节点数
    graph_node_count = 0;
    for (int i = 0; i < city_meta_count; i++) {
        graph_node_count++; // Landmark
        if (raw_data[i].has_airport) graph_node_count++;
        if (raw_data[i].has_railway) graph_node_count++;
    }
    graph_nodes = malloc(graph_node_count * sizeof(Node));
    city_meta_data = malloc(city_meta_count * sizeof(CityMeta));
    int current_node_id = 0;
    for (int i = 0; i < city_meta_count; i++) {
        city_meta_data[i].city_id = i;
        strcpy(city_meta_data[i].city_name, raw_data[i].name);
        city_meta_data[i].airport_node_id = -1;
        city_meta_data[i].hsr_node_id = -1;
        // Landmark
        city_meta_data[i].landmark_node_id = current_node_id;
        graph_nodes[current_node_id] = (Node){ current_node_id, i, NODE_TYPE_LANDMARK, "", raw_data[i].lat_land, raw_data[i].lon_land };
        strcpy(graph_nodes[current_node_id].name, raw_data[i].landmark);
        current_node_id++;
        if (raw_data[i].has_airport) {
            city_meta_data[i].airport_node_id = current_node_id;
            graph_nodes[current_node_id] = (Node){ current_node_id, i, NODE_TYPE_AIRPORT, "", raw_data[i].lat_air, raw_data[i].lon_air };
            strcpy(graph_nodes[current_node_id].name, raw_data[i].airport);
            current_node_id++;
        }
        if (raw_data[i].has_railway) {
            city_meta_data[i].hsr_node_id = current_node_id;
            graph_nodes[current_node_id] = (Node){ current_node_id, i, NODE_TYPE_HSR_STATION, "", raw_data[i].lat_hsr, raw_data[i].lon_hsr };
            strcpy(graph_nodes[current_node_id].name, raw_data[i].railway);
            current_node_id++;
        }
    }
}

void free_graph_data() {
    free(graph_nodes);
    free(city_meta_data);
    graph_nodes = NULL;
    city_meta_data = NULL;
} 
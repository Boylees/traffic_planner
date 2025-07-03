#include "traffic_network.h"
#include <math.h>

#define EARTH_RADIUS_KM 6371.0
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 将角度转换为弧度
static double deg_to_rad(double degrees) {
    return degrees * M_PI / 180.0;
}

/**
 * @brief 使用哈弗辛公式计算两个经纬度点之间的距离
 * 
 * @param lat1 第一个点的纬度
 * @param lon1 第一个点的经度
 * @param lat2 第二个点的纬度
 * @param lon2 第二个点的经度
 * @return double 返回两点间的距离（公里）
 */
double calculate_distance(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = deg_to_rad(lat1);
    double lon1_rad = deg_to_rad(lon1);
    double lat2_rad = deg_to_rad(lat2);
    double lon2_rad = deg_to_rad(lon2);

    double d_lat = lat2_rad - lat1_rad;
    double d_lon = lon2_rad - lon1_rad;

    double a = sin(d_lat / 2.0) * sin(d_lat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(d_lon / 2.0) * sin(d_lon / 2.0);
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    return EARTH_RADIUS_KM * c;
} 
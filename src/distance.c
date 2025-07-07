/**
 * @file distance.c
 * @brief 提供了地理坐标距离计算的功能。
 */
#include "distance.h"
#include <math.h>

// 地球平均半径（公里），用于计算。
#define EARTH_RADIUS_KM 6371.0

// 为一些可能没有定义 M_PI 的编译器（如 MSVC）提供一个备份定义。
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief 将角度制转换为弧度制。
 * @details 在三角函数计算中，需要使用弧度。
 * @param degrees 角度值。
 * @return double 对应的弧度值。
 */
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
    // 1. 将所有角度制的经纬度转换为弧度制。
    double lat1_rad = deg_to_rad(lat1);
    double lon1_rad = deg_to_rad(lon1);
    double lat2_rad = deg_to_rad(lat2);
    double lon2_rad = deg_to_rad(lon2);

    // 2. 计算纬度和经度的差值。
    double d_lat = lat2_rad - lat1_rad;
    double d_lon = lon2_rad - lon1_rad;

    // 3. 应用Haversine公式的核心部分。
    //    a = sin²(Δφ/2) + cos φ1 ⋅ cos φ2 ⋅ sin²(Δλ/2)
    double a = sin(d_lat / 2.0) * sin(d_lat / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(d_lon / 2.0) * sin(d_lon / 2.0);
    
    // 4. 计算中心角 c。
    //    c = 2 ⋅ atan2(√a, √(1−a))
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

    // 5. 最终距离 d = R ⋅ c
    return EARTH_RADIUS_KM * c;
} 
#ifndef DISTANCE_H
#define DISTANCE_H

/**
 * @brief 使用哈弗辛(Haversine)公式计算两个地理坐标点之间的球面距离。
 * 
 * @param lat1 第一个点的纬度 (单位: 度)。
 * @param lon1 第一个点的经度 (单位: 度)。
 * @param lat2 第二个点的纬度 (单位: 度)。
 * @param lon2 第二个点的经度 (单位: 度)。
 * @return double 返回两点之间的大圆距离 (单位: 公里)。
 */
double calculate_distance(double lat1, double lon1, double lat2, double lon2);

#endif // DISTANCE_H 
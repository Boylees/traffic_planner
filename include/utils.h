#ifndef UTILS_H
#define UTILS_H

#include "types.h"

/**
 * @brief 将交通方式枚举转换为可读的英文字符串。
 * @param mode 交通方式枚举。
 * @return const char* 对应的英文字符串。
 */
const char* mode_to_string(TransportMode mode);

/**
 * @brief 将交通方式枚举转换为可读的中文字符串。
 * @param mode 交通方式枚举。
 * @return const char* 对应的中文字符串。
 */
const char* mode_to_string_cn(TransportMode mode);

#endif // UTILS_H 
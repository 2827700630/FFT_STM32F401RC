#ifndef INC_FFT_H_ // 防止头文件重复包含
#define INC_FFT_H_

#include <stdint.h> // 包含标准整数类型定义

// 定义 FFT 点数 (必须是 2 的幂)
#define FFT_N 1024 // 示例大小, 根据需要调整

// 如果 M_PI 未定义，则定义它
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 定义复数结构体
typedef struct
{
    float real; // 实部
    float imag; // 虚部
} complex_t;

/**
 * @brief 执行基-2 时域抽取快速傅里叶变换 (Radix-2 DIT FFT).
 * @param input_output: 指向复数输入数组的指针 (大小为 FFT_N)。
 *                      输入数据应放在实部，虚部初始化为 0。
 *                      输出将覆盖此数组 (原地计算)。
 * @param n: FFT 的大小 (必须等于 FFT_N 且为 2 的幂)。
 */
void fft_radix2(complex_t *input_output, uint32_t n);

/**
 * @brief 计算复数 FFT 输出的幅度。
 * @param complex_output: 指向复数 FFT 输出数组的指针 (大小为 FFT_N)。
 * @param magnitudes: 指向存储幅度的输出数组的指针 (大小为 FFT_N / 2)。
 *                    通常只需要前 N/2 个幅度值。
 * @param n: FFT 的大小 (必须等于 FFT_N)。
 */
void fft_calculate_magnitudes(complex_t *complex_output, float *magnitudes, uint32_t n);

#endif /* INC_FFT_H_ */
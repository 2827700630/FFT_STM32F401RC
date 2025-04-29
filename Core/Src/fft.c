#include "fft.h"
#include <math.h>   // 包含数学函数库 (用于 sinf, cosf, sqrtf)
#include <string.h> // 包含字符串处理函数库 (用于 memcpy, 可选)

// --- 私有辅助函数 ---

/**
 * @brief 反转一个整数的位。
 * @param x: 要反转的整数。
 * @param bits: 要考虑的位数 (log2(N))。
 * @return 位反转后的整数。
 */
static uint32_t reverse_bits(uint32_t x, uint32_t bits)
{
    uint32_t y = 0; // 初始化结果为 0
    for (uint32_t i = 0; i < bits; i++)
    {
        y <<= 1;   // 结果左移一位，为下一位腾出空间
        if (x & 1) // 检查 x 的最低位是否为 1
        {
            y |= 1; // 如果是 1，则将结果的最低位置 1
        }
        x >>= 1; // x 右移一位，处理下一位
    }
    return y; // 返回位反转后的结果
}

/**
 * @brief 对输入数组执行位反转置换。
 * @param data: 指向复数数据数组的指针。
 * @param n: FFT 的大小 (必须是 2 的幂)。
 * @param log2n: n 的以 2 为底的对数。
 */
static void bit_reversal_permutation(complex_t *data, uint32_t n, uint32_t log2n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        uint32_t j = reverse_bits(i, log2n); // 计算索引 i 的位反转索引 j
        // 确保只交换一次 (当 i < j 时)
        if (i < j)
        {
            // 交换 data[i] 和 data[j]
            complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }
}

// --- 公共函数 ---

/**
 * @brief 执行基-2 时域抽取快速傅里叶变换 (Radix-2 DIT FFT)。
 */
void fft_radix2(complex_t *input_output, uint32_t n)
{
    // --- 输入验证 ---
    if (n == 0 || (n & (n - 1)) != 0)
    {
        // n 必须大于 0 且是 2 的幂 (检查 n & (n-1) 是否为 0)
        return; // 如果输入无效则返回
    }

    // --- 计算 log2(n) ---
    uint32_t log2n = 0;
    uint32_t temp_n = n;
    while (temp_n > 1)
    {
        temp_n >>= 1; // 右移一位相当于除以 2
        log2n++;      // 计数器加 1
    }

    // --- 位反转置换 ---
    // 重新排列输入数据，以便进行蝶形运算
    bit_reversal_permutation(input_output, n, log2n);

    // --- 蝶形运算 ---
    // 从第 1 级到第 log2n 级迭代
    for (uint32_t stage = 1; stage <= log2n; stage++)
    {
        uint32_t m = 1 << stage;  // 当前级的蝶形运算分组大小 (2, 4, 8, ...)
        uint32_t m_half = m >> 1; // 分组大小的一半
        complex_t w_m;            // 旋转因子 W_m (用于当前分组)
        complex_t w;              // 当前蝶形运算的旋转因子 W_m^k

        // 计算当前级的主旋转因子 W_m = exp(-j * 2 * pi / m)
        float angle_m = -2.0f * M_PI / m;
        w_m.real = cosf(angle_m); // 实部
        w_m.imag = sinf(angle_m); // 虚部

        // 初始化第一个蝶形运算的旋转因子 W_m^0 = 1
        w.real = 1.0f;
        w.imag = 0.0f;

        // 遍历每个分组内的蝶形运算 (j 从 0 到 m/2 - 1)
        for (uint32_t j = 0; j < m_half; j++)
        {
            // 遍历具有相同旋转因子的所有分组 (k 从 j 开始，步长为 m)
            for (uint32_t k = j; k < n; k += m)
            {
                complex_t t;                 // 临时变量，用于存储 w * input_output[k + m/2]
                uint32_t k_odd = k + m_half; // 蝶形运算的“奇数”输入索引

                // 计算 t = w * input_output[k_odd] (复数乘法)
                // (a+bi)(c+di) = (ac-bd) + (ad+bc)i
                t.real = w.real * input_output[k_odd].real - w.imag * input_output[k_odd].imag;
                t.imag = w.real * input_output[k_odd].imag + w.imag * input_output[k_odd].real;

                // 计算蝶形运算的输出
                // input_output[k_odd] = input_output[k] - t (复数减法)
                input_output[k_odd].real = input_output[k].real - t.real;
                input_output[k_odd].imag = input_output[k].imag - t.imag;

                // input_output[k] = input_output[k] + t (复数加法)
                input_output[k].real = input_output[k].real + t.real;
                input_output[k].imag = input_output[k].imag + t.imag;
            }
            // 更新旋转因子，用于下一个蝶形运算 (w = w * w_m)
            float w_real_temp = w.real; // 临时存储 w 的实部
            w.real = w.real * w_m.real - w.imag * w_m.imag;
            w.imag = w_real_temp * w_m.imag + w.imag * w_m.real;
        }
    }
}

/**
 * @brief 计算复数 FFT 输出的幅度。
 */
void fft_calculate_magnitudes(complex_t *complex_output, float *magnitudes, uint32_t n)
{
    // 计算前 N/2 个频点的幅度 (根据 FFT 的对称性)
    for (uint32_t i = 0; i < n / 2; i++)
    {
        float real = complex_output[i].real; // 获取实部
        float imag = complex_output[i].imag; // 获取虚部
        // 计算幅度: magnitude = sqrt(real^2 + imag^2)
        // 注意: 有时 FFT 输出需要按 N 或 sqrt(N) 进行缩放以获得实际物理幅度。
        // 这里提供的是原始幅度。如果需要，可以除以 N。
        // magnitudes[i] = sqrtf(real * real + imag * imag);
        magnitudes[i] = sqrtf(real * real + imag * imag) / (float)n;
    }
}
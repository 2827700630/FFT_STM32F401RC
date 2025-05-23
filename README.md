# STM32F401RC FFT 频谱分析仪

这个项目实现了一个基于STM32F401RC单片机的实时FFT频谱分析仪，可以通过网页界面设置信号参数，在STM32上生成对应的模拟信号，执行FFT分析并将结果通过USB虚拟串口传回网页端显示频谱。

## 功能特点

- 使用纯C语言实现完整的基-2 FFT算法，无需依赖外部DSP库
- 通过USB虚拟串口（CDC模式）与电脑通信
- 基于Web Serial API的网页图形界面，实时显示频谱
- 支持动态调整信号参数（频率、幅度、直流偏移）
- 在网页上同时显示频率和FFT索引双轴
- 支持实时峰值频率检测

## 硬件要求

- 开发板：STM32F401RC (或其他兼容STM32F4系列)
- 接口：USB连接（用于虚拟串口通信）
- 可选：LED指示灯（用于状态显示）

## 软件架构

项目由以下几个主要部分组成：

1. **FFT算法实现** (`fft.c`, `fft.h`)
   - 纯C语言实现的基-2 FFT算法
   - 支持任意2的幂次方点数（配置为1024点）
   - 包括位反转、蝶形运算和幅度计算

2. **STM32主程序** (`main.c`)
   - 初始化系统和外设
   - 生成模拟正弦波信号
   - 调用FFT函数执行频谱分析
   - 通过USB发送分析结果

3. **USB通信接口** (`usbd_cdc_if.c`)
   - 处理USB虚拟串口通信
   - 解析来自PC的参数命令
   - 触发FFT重新计算

4. **Web前端** (`index.html`)
   - 使用Web Serial API连接STM32设备
   - 提供参数调整界面（频率、幅度、偏移）
   - 使用Chart.js绘制实时频谱图

## 使用方法

### 烧录程序

1. 使用STM32CubeIDE或其他兼容IDE打开项目
2. 编译项目
3. 将程序烧录到STM32F401RC开发板

### 使用网页界面

1. 将开发板通过USB连接到电脑
2. 在Chrome或Edge浏览器中打开`index.html`文件
3. 点击"连接串口"按钮，选择STM32设备对应的COM口
4. 等待初始频谱显示
5. 调整参数（频率、幅度、直流偏移），点击"发送参数到STM32"
6. 观察更新后的频谱图

## 命令格式

网页和STM32之间通过以下格式的命令进行通信：

- **参数设置命令**（网页 → STM32）：
  ```
  PARAM:<频率>,<幅度>,<偏移>\r\n
  ```
  例如: `PARAM:1000.0,1.0,0.0\r\n`

- **FFT数据**（STM32 → 网页）：
  ```
  --- FFT Magnitudes (F:<频率>Hz A:<幅度> O:<偏移>) ---
  FFT[0]: <幅度值>
  FFT[1]: <幅度值>
  ...
  FFT[511]: <幅度值>
  --- FFT Transmission Complete ---
  ```

## 技术细节

- FFT点数: 1024点
- 采样频率: 48kHz
- 频率分辨率: 46.875 Hz (48000/1024)
- 可分析频率范围: 0-24kHz (奈奎斯特频率)

## 浏览器兼容性

网页界面使用Web Serial API，仅兼容：
- Google Chrome (89+)
- Microsoft Edge (89+)
- 其他Chromium内核浏览器

Firefox和Safari不支持Web Serial API，无法使用网页界面。

## 许可

本项目使用MIT许可证，详见LICENSE文件。

## 作者

[雪豹]

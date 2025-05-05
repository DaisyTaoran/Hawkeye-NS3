#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import matplotlib.pyplot as plt

def plot_throughput(filename, window_size=1):
    """
    绘制网络吞吐量随时间变化的折线图
    
    参数:
        filename (str): 输入文件名
        window_size (int): 滑动窗口大小（用于平滑数据，默认为1即不平滑）
    """
    # 读取并解析数据
    timestamps = []
    packet_sizes = []
    
    with open(filename, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            
            try:
                t, size = line.split(',')
                t = float(t)
                size = int(size)
                timestamps.append(t)
                packet_sizes.append(size)
            except ValueError:
                print 'Warnning: line ', line_num, ' error，has jumped.'
                continue

    # 计算瞬时速率(Mbps)
    rates = []
    time_points = []
    for i in range(1, len(timestamps)):
        delta_t = timestamps[i] - timestamps[i-1]
        # 计算速率: (bytes * 8 bits/byte) / (seconds * 1e6 bits/Mbit) = Mbps
        rate = (packet_sizes[i] * 8) / (delta_t * 1e9)
        rates.append(rate)
        time_points.append(timestamps[i])

    # 可选: 滑动窗口平均平滑数据
    if window_size > 1:
        smoothed_rates = []
        for i in range(len(rates)):
            start = max(0, i - window_size // 2)
            end = min(len(rates), i + window_size // 2 + 1)
            smoothed_rates.append(sum(rates[start:end]) / (end - start))
        rates = smoothed_rates

    # 绘制图表
    plt.figure(figsize=(12, 6))
    plt.plot(time_points, rates, 
             linestyle='-', 
             linewidth=1.5,
             color='steelblue',
             label='FlowRate')
    ''''''
    plt.xlabel('Time (Seconds)', fontsize=12)
    plt.ylabel('FlowRate (Gbps)', fontsize=12)
    plt.title('Flow Rate Over Time', fontsize=14)
    plt.grid(True, linestyle=':', alpha=0.7)
    plt.legend()
    
    # 调整坐标轴范围，忽略后5%的异常值
    ''''''
    if len(rates) > 10:
        sorted_rates = sorted(rates)
        upper = sorted_rates[int(0.995 * len(rates))]
        plt.ylim(0, upper*1.005)
    
    plt.tight_layout()
    
    # 保存图片
    output_file = filename.replace('.txt', '_flowrate.png')
    plt.savefig(output_file, dpi=1000)
    print 'picture has be saved: ', output_file
    
    plt.show()

# 使用示例

plot_throughput('agent_1.txt', window_size=1)
plot_throughput('agent_2.txt', window_size=1)
plot_throughput('agent_5.txt', window_size=1)



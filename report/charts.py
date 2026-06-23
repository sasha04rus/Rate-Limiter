import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rcParams

# Настройка стиля
rcParams['font.size'] = 12
rcParams['figure.figsize'] = (12, 8)
rcParams['figure.dpi'] = 100

# ============================================
# ДАННЫЕ ИЗ БЕНЧМАРКОВ
# ============================================

# 1. Данные алгоритмов (из benchmark_algorithms)
algorithms_data = {
    'Algorithm': ['SlidingWindow', 'LeakingBucket', 'TokenBucket'],
    'Time (ns)': [4.33, 4.04, 4.56],
    'Items/sec (M/s)': [231.157, 247.285, 219.252]
}

# 2. Данные менеджера (один ключ)
manager_single_data = {
    'Algorithm': ['SlidingWindow', 'LeakingBucket', 'TokenBucket'],
    'Time (ns)': [20.8, 22.1, 21.5],
    'Items/sec (M/s)': [48.138, 45.275, 46.481]
}

# 3. Данные масштабирования (много ключей)
scaling_data = {
    'SlidingWindow': {
        'Keys': [1, 10, 100, 1000, 10000],
        'Time (ns)': [22.2, 23.5, 24.3, 36.5, 50.8],
        'Items/sec (M/s)': [45.121, 42.630, 41.139, 27.377, 19.689],
        'BigO': 31.45,
        'RMS': 35
    },
    'LeakingBucket': {
        'Keys': [1, 10, 100, 1000, 10000],
        'Time (ns)': [21.6, 24.3, 23.6, 32.1, 37.5],
        'Items/sec (M/s)': [46.244, 41.241, 42.301, 31.148, 26.684],
        'BigO': 27.82,
        'RMS': 22
    },
    'TokenBucket': {
        'Keys': [1, 10, 100, 1000, 10000],
        'Time (ns)': [22.3, 23.0, 25.3, 33.9, 39.4],
        'Items/sec (M/s)': [44.842, 43.554, 39.522, 29.502, 25.415],
        'BigO': 28.76,
        'RMS': 23
    }
}

# 4. Данные паттернов трафика (из benchmark_patterns)
patterns_data = {
    'Uniform': {
        'Params': ['100/1000', '1000/1000', '1000/10000', '10000/10000'],
        'Time (ns)': [15776, 16360, 163677, 168908],
        'Items/sec (M/s)': [63.427, 61.127, 61.098, 59.207]
    },
    'Burst': {
        'Params': ['100/1000', '1000/1000', '1000/10000', '10000/10000'],
        'Time (ns)': [157476, 163305, 163356, 171744],
        'Items/sec (M/s)': [63.506, 61.240, 61.217, 58.227]
    },
    'Diurnal': {
        'Params': ['100/1000', '1000/1000', '1000/10000', '10000/10000'],
        'Time (ns)': [28696, 28822, 287148, 286197],
        'Items/sec (M/s)': [27.879, 27.758, 27.865, 27.957]
    },
    'SparseBurst': {
        'Params': ['100/1000', '1000/1000', '1000/10000', '10000/10000'],
        'Time (ns)': [145918, 150230, 157566, 163835],
        'Items/sec (M/s)': [61.961, 60.183, 60.236, 57.932]
    },
    'Zombie': {
        'Params': ['100/1000', '1000/1000', '1000/10000', '10000/10000'],
        'Time (ns)': [15828, 15832, 165713, 165714],
        'Items/sec (M/s)': [63.181, 63.163, 60.347, 60.348]
    }
}

# ============================================
# ГРАФИК 1: Сравнение алгоритмов (чистые vs менеджер)
# ============================================

fig, axes = plt.subplots(1, 2, figsize=(14, 6))

# График 1a: Время выполнения
ax1 = axes[0]
algorithms = algorithms_data['Algorithm']
x = np.arange(len(algorithms))
width = 0.35

bars1 = ax1.bar(x - width/2, algorithms_data['Time (ns)'], width, 
                label='Чистый алгоритм', color='#2E86AB', alpha=0.8)
bars2 = ax1.bar(x + width/2, manager_single_data['Time (ns)'], width,
                label='Через менеджер', color='#A23B72', alpha=0.8)

ax1.set_xlabel('Алгоритм', fontsize=12)
ax1.set_ylabel('Время выполнения (ns)', fontsize=12)
ax1.set_title('Сравнение времени выполнения алгоритмов\n(чистый vs через менеджер)', fontsize=14, fontweight='bold')
ax1.set_xticks(x)
ax1.set_xticklabels(algorithms)
ax1.legend()
ax1.grid(True, alpha=0.3)

# Добавляем значения на столбцы
for bar in bars1:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height + 0.2,
             f'{height:.2f}', ha='center', va='bottom', fontsize=10)
for bar in bars2:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height + 0.2,
             f'{height:.1f}', ha='center', va='bottom', fontsize=10)

# График 1b: Пропускная способность
ax2 = axes[1]
bars3 = ax2.bar(x - width/2, algorithms_data['Items/sec (M/s)'], width,
                label='Чистый алгоритм', color='#2E86AB', alpha=0.8)
bars4 = ax2.bar(x + width/2, manager_single_data['Items/sec (M/s)'], width,
                label='Через менеджер', color='#A23B72', alpha=0.8)

ax2.set_xlabel('Алгоритм', fontsize=12)
ax2.set_ylabel('Пропускная способность (M запросов/сек)', fontsize=12)
ax2.set_title('Сравнение пропускной способности\n(чистый vs через менеджер)', fontsize=14, fontweight='bold')
ax2.set_xticks(x)
ax2.set_xticklabels(algorithms)
ax2.legend()
ax2.grid(True, alpha=0.3)

for bar in bars3:
    height = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2., height + 2,
             f'{height:.1f}', ha='center', va='bottom', fontsize=10)
for bar in bars4:
    height = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2., height + 2,
             f'{height:.1f}', ha='center', va='bottom', fontsize=10)

plt.tight_layout()
plt.savefig('algorithm_comparison.png', dpi=300, bbox_inches='tight')
plt.show()

# ============================================
# ГРАФИК 2: Масштабирование (время vs количество ключей)
# ============================================

fig, axes = plt.subplots(1, 2, figsize=(14, 6))

colors = {'SlidingWindow': '#2E86AB', 'LeakingBucket': '#A23B72', 'TokenBucket': '#F18F01'}

# График 2a: Время выполнения
ax1 = axes[0]
for algo, data in scaling_data.items():
    ax1.plot(data['Keys'], data['Time (ns)'], marker='o', linewidth=2, 
             markersize=8, label=algo, color=colors[algo])
    
    # Добавляем значения точек
    for i, (key, time) in enumerate(zip(data['Keys'], data['Time (ns)'])):
        ax1.annotate(f'{time:.1f}', (key, time), 
                    textcoords="offset points", xytext=(0,10), ha='center', fontsize=9)

ax1.set_xlabel('Количество ключей', fontsize=12)
ax1.set_ylabel('Время выполнения (ns)', fontsize=12)
ax1.set_title('Масштабирование: время выполнения\nв зависимости от количества ключей', fontsize=14, fontweight='bold')
ax1.set_xscale('log')
ax1.legend()
ax1.grid(True, alpha=0.3)

# Добавляем O(n) сложность
x_vals = np.array([1, 10000])
for algo, data in scaling_data.items():
    # Экстраполируем O(n) сложность
    o_n = data['BigO'] * x_vals / 10000
    ax1.plot(x_vals, o_n, '--', alpha=0.5, color=colors[algo], 
            label=f'{algo} O(n)')

# График 2b: Пропускная способность
ax2 = axes[1]
for algo, data in scaling_data.items():
    ax2.plot(data['Keys'], data['Items/sec (M/s)'], marker='s', linewidth=2,
             markersize=8, label=algo, color=colors[algo])
    
    for i, (key, speed) in enumerate(zip(data['Keys'], data['Items/sec (M/s)'])):
        ax2.annotate(f'{speed:.1f}', (key, speed), 
                    textcoords="offset points", xytext=(0,10), ha='center', fontsize=9)

ax2.set_xlabel('Количество ключей', fontsize=12)
ax2.set_ylabel('Пропускная способность (M запросов/сек)', fontsize=12)
ax2.set_title('Масштабирование: пропускная способность\nв зависимости от количества ключей', fontsize=14, fontweight='bold')
ax2.set_xscale('log')
ax2.legend()
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('scaling_analysis.png', dpi=300, bbox_inches='tight')
plt.show()

# ============================================
# ГРАФИК 3: Сравнение паттернов трафика
# ============================================

fig, axes = plt.subplots(2, 1, figsize=(14, 12))

# График 3a: Время выполнения паттернов
ax1 = axes[0]
patterns = list(patterns_data.keys())
colors_patterns = ['#2E86AB', '#A23B72', '#F18F01', '#D62828', '#8D6B94']

# Для каждого паттерна берём среднее время
avg_times = [np.mean(data['Time (ns)']) for data in patterns_data.values()]
std_times = [np.std(data['Time (ns)']) for data in patterns_data.values()]

bars = ax1.bar(patterns, avg_times, yerr=std_times, capsize=10,
              color=colors_patterns, alpha=0.8, edgecolor='black', linewidth=1)

ax1.set_xlabel('Паттерн трафика', fontsize=12)
ax1.set_ylabel('Время выполнения (ns)', fontsize=12)
ax1.set_title('Сравнение производительности паттернов трафика\n(среднее время с разбросом)', fontsize=14, fontweight='bold')
ax1.grid(True, alpha=0.3)

# Добавляем значения
for bar, time in zip(bars, avg_times):
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height + 2000,
             f'{time:.0f} ns', ha='center', va='bottom', fontsize=10)

# График 3b: Детальное сравнение всех паттернов
ax2 = axes[1]
x_positions = []
labels = []
all_times = []
all_speeds = []

for i, (pattern, data) in enumerate(patterns_data.items()):
    x_positions.extend([i + j*0.2 for j in range(len(data['Params']))])
    labels.extend([f'{pattern}\n{param}' for param in data['Params']])
    all_times.extend(data['Time (ns)'])
    all_speeds.extend(data['Items/sec (M/s)'])

# Создаём цветовую карту для каждого паттерна
colors_extended = []
for i, pattern in enumerate(patterns_data.keys()):
    colors_extended.extend([colors_patterns[i]] * len(patterns_data[pattern]['Params']))

# Время выполнения
bars2 = ax2.bar(x_positions, all_times, color=colors_extended, alpha=0.8,
                edgecolor='black', linewidth=0.5)

ax2.set_xlabel('Паттерн / Параметры (запросы/итерации)', fontsize=12)
ax2.set_ylabel('Время выполнения (ns)', fontsize=12)
ax2.set_title('Детальное сравнение всех паттернов трафика\n(по параметрам)', fontsize=14, fontweight='bold')
ax2.set_xticks(x_positions)
ax2.set_xticklabels(labels, rotation=45, ha='right', fontsize=8)
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('pattern_comparison.png', dpi=300, bbox_inches='tight')
plt.show()

# ============================================
# ГРАФИК 4: Тепловая карта производительности
# ============================================

fig, ax = plt.subplots(figsize=(12, 8))

# Создаём матрицу для тепловой карты
patterns_names = list(patterns_data.keys())
param_names = ['100/1000', '1000/1000', '1000/10000', '10000/10000']
data_matrix = np.array([[patterns_data[p]['Time (ns)'][i] for i in range(len(param_names))] 
                        for p in patterns_names])

# Нормализуем для лучшей визуализации
data_norm = data_matrix / 1000  # Переводим в микросекунды

im = ax.imshow(data_norm, cmap='YlOrRd', aspect='auto', interpolation='nearest')

# Настройка осей
ax.set_xticks(np.arange(len(param_names)))
ax.set_yticks(np.arange(len(patterns_names)))
ax.set_xticklabels(param_names, fontsize=10)
ax.set_yticklabels(patterns_names, fontsize=10)

# Добавляем значения в ячейки
for i in range(len(patterns_names)):
    for j in range(len(param_names)):
        text = ax.text(j, i, f'{data_norm[i, j]:.1f} µs',
                      ha="center", va="center", color="black", fontsize=9)

ax.set_xlabel('Параметры (запросы/итерации)', fontsize=12)
ax.set_ylabel('Паттерн трафика', fontsize=12)
ax.set_title('Тепловая карта времени выполнения паттернов\n(в микросекундах)', fontsize=14, fontweight='bold')

# Добавляем цветовую шкалу
cbar = plt.colorbar(im, ax=ax)
cbar.set_label('Время выполнения (µs)', fontsize=12)

plt.tight_layout()
plt.savefig('heatmap_patterns.png', dpi=300, bbox_inches='tight')
plt.show()

# ============================================
# ГРАФИК 5: Анализ Big-O и RMS
# ============================================

fig, ax = plt.subplots(figsize=(10, 6))

# Данные для Big-O и RMS
algo_names = list(scaling_data.keys())
big_o_values = [scaling_data[algo]['BigO'] for algo in algo_names]
rms_values = [scaling_data[algo]['RMS'] for algo in algo_names]

x = np.arange(len(algo_names))
width = 0.35

bars1 = ax.bar(x - width/2, big_o_values, width, label='Big-O коэффициент', 
               color='#2E86AB', alpha=0.8)
bars2 = ax.bar(x + width/2, rms_values, width, label='RMS отклонение (%)',
               color='#A23B72', alpha=0.8)

ax.set_xlabel('Алгоритм', fontsize=12)
ax.set_ylabel('Значение', fontsize=12)
ax.set_title('Анализ сложности алгоритмов\n(Big-O коэффициент и RMS отклонение)', fontsize=14, fontweight='bold')
ax.set_xticks(x)
ax.set_xticklabels(algo_names)
ax.legend()
ax.grid(True, alpha=0.3)

# Добавляем значения на столбцы
for bar in bars1:
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2., height + 0.5,
             f'{height:.2f}', ha='center', va='bottom', fontsize=10)
for bar in bars2:
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2., height + 0.5,
             f'{height:.0f}%', ha='center', va='bottom', fontsize=10)

plt.tight_layout()
plt.savefig('big_o_analysis.png', dpi=300, bbox_inches='tight')
plt.show()

# ============================================
# ГРАФИК 6: Overhead менеджера
# ============================================

fig, ax = plt.subplots(figsize=(10, 6))

algorithms = algorithms_data['Algorithm']
overhead = np.array(manager_single_data['Time (ns)']) - np.array(algorithms_data['Time (ns)'])
overhead_percent = (overhead / np.array(algorithms_data['Time (ns)'])) * 100

x = np.arange(len(algorithms))
width = 0.35

bars1 = ax.bar(x - width/2, overhead, width, label='Абсолютный overhead (ns)',
               color='#2E86AB', alpha=0.8)
bars2 = ax.bar(x + width/2, overhead_percent, width, label='Относительный overhead (%)',
               color='#A23B72', alpha=0.8)

ax.set_xlabel('Алгоритм', fontsize=12)
ax.set_ylabel('Overhead', fontsize=12)
ax.set_title('Overhead менеджера\n(абсолютный и относительный)', fontsize=14, fontweight='bold')
ax.set_xticks(x)
ax.set_xticklabels(algorithms)
ax.legend()
ax.grid(True, alpha=0.3)

# Добавляем значения
for bar in bars1:
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2., height + 0.5,
             f'{height:.1f}', ha='center', va='bottom', fontsize=10)
for bar in bars2:
    height = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2., height + 0.5,
             f'{height:.0f}%', ha='center', va='bottom', fontsize=10)

plt.tight_layout()
plt.savefig('manager_overhead.png', dpi=300, bbox_inches='tight')
plt.show()

# ============================================
# ВЫВОД СТАТИСТИКИ
# ============================================

print("=" * 60)
print("СТАТИСТИЧЕСКИЙ АНАЛИЗ БЕНЧМАРКОВ")
print("=" * 60)

print("\n1. АЛГОРИТМЫ (чистые):")
print("-" * 40)
for i, algo in enumerate(algorithms_data['Algorithm']):
    print(f"  {algo}: {algorithms_data['Time (ns)'][i]:.2f} ns, "
          f"{algorithms_data['Items/sec (M/s)'][i]:.2f} M запросов/сек")

print("\n2. АЛГОРИТМЫ (через менеджер, 1 ключ):")
print("-" * 40)
for i, algo in enumerate(manager_single_data['Algorithm']):
    print(f"  {algo}: {manager_single_data['Time (ns)'][i]:.2f} ns, "
          f"{manager_single_data['Items/sec (M/s)'][i]:.2f} M запросов/сек")

print("\n3. OVERHEAD МЕНЕДЖЕРА:")
print("-" * 40)
for i, algo in enumerate(algorithms):
    abs_overhead = manager_single_data['Time (ns)'][i] - algorithms_data['Time (ns)'][i]
    rel_overhead = (abs_overhead / algorithms_data['Time (ns)'][i]) * 100
    print(f"  {algo}: +{abs_overhead:.1f} ns (+{rel_overhead:.1f}%)")

print("\n4. МАСШТАБИРОВАНИЕ (время при 10000 ключей):")
print("-" * 40)
for algo, data in scaling_data.items():
    time_1key = data['Time (ns)'][0]
    time_10k = data['Time (ns)'][-1]
    degradation = (time_10k / time_1key - 1) * 100
    print(f"  {algo}: {time_1key:.1f} ns -> {time_10k:.1f} ns "
          f"({degradation:.1f}% замедление)")

print("\n5. ПАТТЕРНЫ ТРАФИКА (среднее время):")
print("-" * 40)
for pattern, data in patterns_data.items():
    avg_time = np.mean(data['Time (ns)'])
    std_time = np.std(data['Time (ns)'])
    print(f"  {pattern}: {avg_time:.0f} ± {std_time:.0f} ns")

print("\n6. Big-O и RMS анализ:")
print("-" * 40)
for algo, data in scaling_data.items():
    print(f"  {algo}: Big-O = {data['BigO']:.2f}, RMS = {data['RMS']:.0f}%")

print("\n" + "=" * 60)
print("Графики сохранены в текущей директории:")
print("  1. algorithm_comparison.png")
print("  2. scaling_analysis.png")
print("  3. pattern_comparison.png")
print("  4. heatmap_patterns.png")
print("  5. big_o_analysis.png")
print("  6. manager_overhead.png")
print("=" * 60)

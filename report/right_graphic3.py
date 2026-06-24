import matplotlib.pyplot as plt
import numpy as np
from matplotlib import rcParams

rcParams['font.size'] = 11
rcParams['figure.figsize'] = (14, 10)
rcParams['figure.dpi'] = 100


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


fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10))

patterns = list(patterns_data.keys())
colors = ['#2E86AB', '#A23B72', '#F18F01', '#D62828', '#8D6B94']


avg_times = [np.mean(data['Time (ns)']) for data in patterns_data.values()]
std_times = [np.std(data['Time (ns)']) for data in patterns_data.values()]

bars1 = ax1.bar(patterns, avg_times, yerr=std_times, capsize=8,
                color=colors, alpha=0.8, edgecolor='black', linewidth=1.5)

ax1.set_xlabel('Паттерн трафика', fontsize=12)
ax1.set_ylabel('Среднее время выполнения (ns)', fontsize=12)
ax1.set_title('Сравнение производительности паттернов трафика\n(среднее время с разбросом)', 
              fontsize=14, fontweight='bold')
ax1.grid(True, alpha=0.3, axis='y')

for bar, time in zip(bars1, avg_times):
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height + 2000,
             f'{time:.0f} ns', ha='center', va='bottom', fontsize=10, fontweight='bold')

for i, (bar, std) in enumerate(zip(bars1, std_times)):
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2., height - height/2,
             f'±{std:.0f}', ha='center', va='center', fontsize=9, color='white')


param_labels = ['100/1000', '1000/1000', '1000/10000', '10000/10000']
x_positions = []
all_times = []
all_labels = []
bar_colors = []

for i, pattern in enumerate(patterns):
    times = patterns_data[pattern]['Time (ns)']
    # Позиции для каждого паттерна (группировка)
    base_pos = i * (len(param_labels) + 0.5)
    for j, time in enumerate(times):
        x_positions.append(base_pos + j * 0.8)
        all_times.append(time)
        all_labels.append(f'{pattern}\n{param_labels[j]}')
        bar_colors.append(colors[i])

bars2 = ax2.bar(x_positions, all_times, color=bar_colors, 
                alpha=0.8, edgecolor='black', linewidth=0.8, width=0.7)

ax2.set_xlabel('Паттерн / Параметры (запросы/итерации)', fontsize=12)
ax2.set_ylabel('Время выполнения (ns)', fontsize=12)
ax2.set_title('Детальное сравнение всех паттернов трафика\n(по параметрам)', 
              fontsize=14, fontweight='bold')
ax2.grid(True, alpha=0.3, axis='y')

ax2.set_xticks(x_positions)
ax2.set_xticklabels(all_labels, rotation=45, ha='right', fontsize=8)

for bar, time in zip(bars2, all_times):
    if time > 50000:  # Показываем только для больших значений
        ax2.text(bar.get_x() + bar.get_width()/2., time + 5000,
                 f'{time:.0f}', ha='center', va='bottom', fontsize=7, rotation=90)

from matplotlib.patches import Patch
legend_elements = [Patch(facecolor=colors[i], label=patterns[i]) for i in range(len(patterns))]
ax2.legend(handles=legend_elements, loc='upper left', fontsize=10)
group_boundaries = [i * (len(param_labels) + 0.5) - 0.5 for i in range(1, len(patterns))]
for boundary in group_boundaries:
    ax2.axvline(x=boundary, color='gray', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('pattern_comparison.png', dpi=300, bbox_inches='tight')
plt.show()


print("✅ График 'pattern_comparison.png' успешно создан!")
print("\n" + "=" * 60)
print("СТАТИСТИКА ПО ПАТТЕРНАМ")
print("=" * 60)

print("\n1. СРЕДНЕЕ ВРЕМЯ ВЫПОЛНЕНИЯ:")
print("-" * 40)
for pattern, avg, std in zip(patterns, avg_times, std_times):
    print(f"  {pattern}: {avg:.0f} ± {std:.0f} ns")

print("\n2. ДЕТАЛЬНЫЕ ЗНАЧЕНИЯ ПО ПАРАМЕТРАМ:")
print("-" * 40)
for pattern in patterns:
    print(f"\n  {pattern}:")
    for param, time in zip(patterns_data[pattern]['Params'], 
                          patterns_data[pattern]['Time (ns)']):
        print(f"    {param}: {time} ns")

print("\n3. САМЫЙ БЫСТРЫЙ ПАТТЕРН:")
print("-" * 40)
min_idx = np.argmin(avg_times)
print(f"  {patterns[min_idx]}: {avg_times[min_idx]:.0f} ns")

print("\n4. САМЫЙ МЕДЛЕННЫЙ ПАТТЕРН:")
print("-" * 40)
max_idx = np.argmax(avg_times)
print(f"  {patterns[max_idx]}: {avg_times[max_idx]:.0f} ns")

print("\n" + "=" * 60)
import matplotlib.pyplot as plt
from collections import Counter
import matplotlib.patches as mpatches
from collections import defaultdict
import re

def calculate_cfs_weights():
    weights = []
    for nice in range(0, 40):  # nice values 0-39
        weight = 1024 / (1.25 ** (nice - 20))
        weights.append(int(round(weight)))
    return weights

def write_weights_to_file(weights, filename="cfs_weights.txt"):
    with open(filename, "w") as f:
        f.write("static const int cfs_weights[40] = {\n")
        for i, weight in enumerate(weights):
            f.write(f"    {weight}," + ("\n" if (i + 1) % 5 == 0 else " "))
        f.write("\n};\n")

def extract_pids(data: str):
    # Remove header and footer markers
    start_marker = "All PIDs runniing start"
    end_marker = "All PIDs runniing end"
    
    start_index = data.find(start_marker) + len(start_marker)
    end_index = data.find(end_marker)
    
    if start_index == -1 or end_index == -1:
        raise ValueError("Start or end marker not found.")

    pid_data = data[start_index:end_index].strip()

    # Split by whitespace (handles both spaces and newlines)
    pid_list = re.split(r'\s+', pid_data)
    
    # Filter out anything that is not a digit
    pid_list = [pid for pid in pid_list if pid.isdigit()]
    
    return pid_list

def plot_pid_distribution(pid_list):
    pid_counts = Counter(pid_list)
    labels = list(pid_counts.keys())
    sizes = list(pid_counts.values())

    plt.figure(figsize=(8, 8))
    plt.pie(sizes, labels=labels, autopct='%1.1f%%', startangle=140)
    plt.title("CFS scheduler [\% of total runtime]")
    plt.axis('equal')  # Equal aspect ratio ensures pie is drawn as a circle.
    plt.show()

def extract_pids(data: str):
    start_marker = "All PIDs runniing start"
    end_marker = "All PIDs runniing end"
    
    start_index = data.find(start_marker) + len(start_marker)
    end_index = data.find(end_marker)
    
    if start_index == -1 or end_index == -1:
        raise ValueError("Start or end marker not found.")

    pid_data = data[start_index:end_index].strip()
    pid_list = re.split(r'\s+', pid_data)
    pid_list = [pid for pid in pid_list if pid.isdigit()]
    
    return pid_list

def plot_gantt_chart(pids):
    pid_timings = defaultdict(list)
    
    for time_index, pid in enumerate(pids):
        pid_timings[pid].append(time_index)

    fig, ax = plt.subplots(figsize=(12, 6))
    y_labels = list(sorted(set(pids), key=int))
    y_ticks = range(len(y_labels))
    
    color_map = plt.cm.get_cmap('tab10', len(y_labels))
    pid_to_color = {pid: color_map(i) for i, pid in enumerate(y_labels)}

    for i, pid in enumerate(y_labels):
        times = pid_timings[pid]
        for t in times:
            ax.broken_barh([(t, 1)], (i - 0.4, 0.8), facecolors=pid_to_color[pid])
    
    ax.set_yticks(y_ticks)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel("Time ticks",fontsize=14)
    ax.set_ylabel("PID", fontsize=14)
    ax.set_title("CFS scheduler - Gantt Chart",fontsize=18)
    ax.grid(True, axis='x', linestyle='--', linewidth=0.5)

    legend_handles = [mpatches.Patch(color=pid_to_color[pid], label=f"PID {pid}") for pid in y_labels]
    ax.legend(handles=legend_handles, bbox_to_anchor=(1.05, 1), loc='upper left')
    
    plt.tight_layout()
    plt.show()

# Replace this with the actual data string
data = "All PIDs runniing start 4 5 6 4 7 5 4 6 4 5 6 4 5 7 4 6 5 4 4 5 6 7 4 5 6 4 5 8 4 6 5 7 4 4 5 6 4 5 6 4 7 5 4 6 5 4 4 6 5 7 4 5 6 4 8 5 4 6 7 4 5 6 4 5 4 5 6 7 4 5 4 6 4 5 7 6 4 5 4 6 5 4 8 5 7 4 6 4 5 6 4 5 4 7 6 5 4 4 5 6 4 5 7 6 4 5 4 6 5 8 4 7 6 4 5 4 5 6 4 5 7 4 6 5 4 6 4 5 7 4 5 6 4 5 4 6 8 4 5 7 6 4 5 4 6 5 4 7 5 4 6 4 5 6 4 5 7 4 6 5 4 5 6 4 8 7 4 5 6 4 5 4 6 5 7 4 6 4 5 4 5 6 7 4 5 4 6 5 4 6 4 8 5 7 4 5 6 4 5 4 6 7 5 4 6 4 5 4 6 5 7 4 5 6 4 4 5 6 7 8 4 5 4 6 5 4 5 6 7 4 4 5 6 4 5 4 7 6 5 4 6 5 4 4 5 7 6 8 4 5 4 6 5 4 7 6 4 5 4 5 6 4 5 7 6 4 5 4 6 4 5 7 4 6 8 5 4 5 6 4 5 7 4 6 4 5 4 6 5 4 7 5 6 4 4 5 6 4 5 7 8 4 6 5 4 6 5 4 7 4 5 6 4 5 6 4 5 7 4 6 5 4 4 5 6 7 4 5 8 6 4 5 4 6 4 5 7 4 6 5 4 5 6 4 7 5 4 6 4 5 4 6 5 7 4 8 5 6 4 5 4 6 7 4 5 6 4 5 4 6 5 7 4 4 5 6 4 5 6 7 4 5 4 8 6 5 4 4 5 6 7 4 5 6 4 5 4 7 6 5 4 4 6 5 4 5 7 6 4 5 8 4 6 4 5 7 6 4 5 4 5 6 4 5 7 4 6 4 5 6 4 5 4 7 6 5 4 8 5 4 6 4 5 7 6 4 5 4 6 5 4 7 4 5 6 4 5 6 4 5 7 4 6 5 4 8 6 4 5 7 4 5 6 4 5 4 6 5 7 4 6 4 5 4 5 6 4 7 5 4 6 4 5 8 6 4 5 7 4 6 5 4 5 4 6 7 4 5 6 4 5 4 6 5 7 4 5 6 4 8 4 5 6 7 4 5 4 6 5 4 6 4 5 7 4 5 6 4 5 4 6 7 5 4 6 4 5 8 4 5 6 7 4 5 4 6 5 4 7 6 4 5 4 6 5 4 5 7 6 4 4 5 6 4 8 5 7 4 6 5 4 5 6 4 4 5 7 6 4 5 4 6 5 4 7 6 5 4 4 5 6 8 4 5 7 4 6 5 4 6 4 5 7 4 5 6 4 5 6 4 5 7 4 6 4 5 4 6 5 8 4 7 5 6 4 5 4 6 4 5 7 4 6 5 4 5 6 4 7 4 5 6 4 5 4 6 8 5 7 4 6 5 4 4 5 6 7 4 5 6 4 5 4 6 5 7 4 4 5 6 4 5 6 4 7 8 5 4 6 4 5 4 6 5 7 4 5 6 4 5 4 6 7 4 5 4 6 5 4 5 6 7 8 4 5 4 6 4 5 7 6 4 5 4 6 5 4 4 5 7 6 4 5 6 4 5 4 7 6 8 5 4 4 5 6 4 5 7 6 4 5 4 6 5 4 7 4 6 5 4 5 6 4 5 7 4 6 8 4 5 6 4 5 7 4 5 6 4 5 4 6 4 5 7 6 4 5 4 6 5 4 7 5 4 8 6 4 5 6 4 5 7 4 6 5 4 4 5 6 7 4 5 6 4 5 4 6 5 7 4 8 6 4 5 4 5 6 4 7 5 4 6 5 4 6 4 5 7 4 5 6 4 5 4 6 7 4 5 8 6 4 5 4 6 5 7 4 5 4 6 4 5 6 7 4 5 4 6 5 4 5 6 7 4 4 5 8 6 4 5 4 7 6 5 4 6 4 5 4 5 7 6 4 5 4 6 5 4 7 6 4 5 8 4 5 6 4 5 7 4 6 5 4 6 4 5 7 4 6 5 4 5 6 4 4 5 7 6 4 5 8 4 6 5 4 All PIDs runniing end"

pids = extract_pids(data)
plot_pid_distribution(pids)


if __name__ == "__main__":
    pids = extract_pids(data)
    plot_gantt_chart(pids)
    #pids = extract_pids(data)
    #plot_pid_distribution(pids)
    '''
    weights = calculate_cfs_weights()
    write_weights_to_file(weights)
    print("CFS weights generated and saved to 'cfs_weights.txt'.")
    '''
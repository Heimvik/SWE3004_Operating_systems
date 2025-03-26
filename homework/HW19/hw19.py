import matplotlib.pyplot as plt
import subprocess

PAGES_MAX = int(1e5)

pageTesterProgram = "./hw19"  # Path to the compiled C program
cpu_affinity_command = ["taskset", "-c", "0", pageTesterProgram] #Lock it onto cpu 0 (affinity)
x_pages = []
y_time = []

base = 1.5
exp = 0
while True:
    exp += 1
    pages = int(round(base**exp,0))
    command = cpu_affinity_command + [str(pages)]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    x_pages.append(pages)
    y_time.append(float(result.stdout.strip()))
    print("Done with", pages, "pages\n")
    if(PAGES_MAX < pages):
        break

print("Plotting data:", x_pages, y_time)
plt.plot(x_pages, y_time, marker='o', linestyle='-', color='b')
plt.xlabel("Number of pages")
plt.ylabel("Average access time per page(us)")
plt.semilogx()
plt.grid(True)
plt.title("Time to access pages")
plt.show()
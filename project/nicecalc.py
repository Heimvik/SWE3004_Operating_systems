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

if __name__ == "__main__":
    weights = calculate_cfs_weights()
    write_weights_to_file(weights)
    print("CFS weights generated and saved to 'cfs_weights.txt'.")
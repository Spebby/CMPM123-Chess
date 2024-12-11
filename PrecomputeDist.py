import math

def calculate_distances():
    distances = []

    for square in range(64):
        y = square // 8
        x = square - y * 8

        north = 7 - y
        south = y
        west = x
        east = 7 - x

        dist = [0] * 8
        dist[0] = north
        dist[1] = east
        dist[2] = south
        dist[3] = west
        dist[4] = min(north, east)
        dist[5] = min(south, east)
        dist[6] = min(south, west)
        dist[7] = min(north, west)

        distances.append((square, dist))

    return distances

def write_to_file(filename, distances):
    with open(filename, 'w') as file:
        file.write("int _dist[64][8] = {\n")
        for i, dist in distances:
            dist_str = ', '.join(map(str, dist))
            file.write(f"    {{ {dist_str} }},\n")
        file.write("};\n")

def main():
    distances = calculate_distances()
    write_to_file("distances.cpp", distances)

if __name__ == "__main__":
    main()

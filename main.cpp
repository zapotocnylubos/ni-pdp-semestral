#include <iostream>
#include <string>
#include <vector>
#include <fstream>

struct Graph {
    int size;
    std::vector<std::vector<int>> graph;

    explicit Graph(std::vector<std::vector<int>> graph, int size) : graph(std::move(graph)), size(size) {}

    explicit Graph(const std::string &file) {
        std::ifstream infile(file);

        if (infile.fail()) {
            throw std::invalid_argument("File does not exist");
        }

        infile >> size;

        graph = std::vector<std::vector<int>>(size, std::vector<int>(size, 0));

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                infile >> graph[i][j];
            }
        }

        infile.close();
    }
};

int min = std::numeric_limits<int>::max();

void DFS_BB(const Graph &g, int maxPartitionSize, std::vector<bool> &vec, int index, int count) {
    // Pokud jsme vygenerovali tři true hodnoty, vypíšeme aktuální kombinaci
    if (count == maxPartitionSize) {
        for (auto &&i: vec) {
            std::cout << i << " ";
        }
        std::cout << " -> ";
        int sum = 0;
        for (int i = 0; i < g.size; i++) {
            if (!vec[i]) continue;

            for (int j = 0; j < g.size; j++) {
                if (vec[j]) continue;
                sum += g.graph[i][j];
            }
        }
        if (sum < min) {
            min = sum;
        }
        std::cout << sum << std::endl;
        return;
    }

    // Pokud jsme na konci vektoru, neděláme nic
    if (index == vec.size()) {
        return;
    }

    // Nastavíme aktuální prvek na true a zavoláme rekurzivně funkci pro další index s počtem true hodnot o jedna větším
    vec[index] = true;
    DFS_BB(g, maxPartitionSize, vec, index + 1, count + 1);

    // Nastavíme aktuální prvek na false a zavoláme rekurzivně funkci pro další index s počtem true hodnot stejným
    vec[index] = false;
    DFS_BB(g, maxPartitionSize, vec, index + 1, count);
}

void DFS_BB(const Graph &g, int maxPartitionSize) {
    auto cut = std::vector(g.size, false);
    DFS_BB(g, maxPartitionSize, cut, 0, 0);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: ./a.out [maxPartitionSize] [file]" << std::endl;
        return 1;
    }

    auto maxPartitionSize = std::stoi(argv[1]);
    auto file = argv[2];

    auto g = Graph(file);

    auto cut = std::vector<bool>(g.size, false);

    DFS_BB(g, maxPartitionSize);

    std::cout << "Global minimum is: " << min << std::endl;
    return 0;
}

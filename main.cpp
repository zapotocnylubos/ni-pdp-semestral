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

    [[nodiscard]] int cutWeight(const std::vector<bool> &cut) const {
        int weight = 0;
        for (int i = 0; i < size; i++) {
            if (!cut[i]) continue;

            for (int j = 0; j < size; j++) {
                if (cut[j]) continue;
                weight += graph[i][j];
            }
        }
        return weight;
    }
};

void DFS_BB(const Graph &graph, int maxPartitionSize,
            std::vector<bool> &cut, int index, int count, int &bestWeight) {
    // end stop of the recursion
    if (index > graph.size) {
        return;
    }

    // this cannot be better solution
//    if (currentWeight >= bestWeight) {
//        return;
//    }

    if (count == maxPartitionSize) {
        // count this vertex cut
        int currentWeight = graph.cutWeight(cut);

        if (currentWeight < bestWeight) {
            bestWeight = currentWeight;
        }

        for (auto i: cut) {
            std::cout << i << " ";
        }

        std::cout << " -> " << currentWeight << std::endl;
        return;
    }

    // try without this vertex
    DFS_BB(graph, maxPartitionSize, cut, index + 1, count, bestWeight);

    // try with this vertex
    cut[index] = true;

    // try with this vertex
    DFS_BB(graph, maxPartitionSize, cut, index + 1, count + 1, bestWeight);

    // restore the status of the cut
    cut[index] = false;
}

int DFS_BB(const Graph &g, int maxPartitionSize) {
    auto cut = std::vector(g.size, false);
//    for (int i = 0; i < maxPartitionSize - 1; i++) {
//        cut[i] = true;
//    }
//
//    int weight = 0;
//    for (int i = 0; i < g.size; i++) {
//        for (int j = 0; j < g.size; j++) {
//            if (cut[j]) {
//                continue;
//            }
//            weight += g.graph[i][j];
//        }
//    }

//    int bestWeight = weight;
    int bestWeight = std::numeric_limits<int>::max();
    DFS_BB(g, maxPartitionSize, cut, 0, 0, bestWeight);
    return bestWeight;
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

    int bestWeight = DFS_BB(g, maxPartitionSize);

    std::cout << "Global minimum is: " << bestWeight << std::endl;
    return 0;
}

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

    [[nodiscard]] int vertexWeight(const std::vector<bool> &cut, int vertex) const {
        int weight = 0;
        for (int i = 0; i < size; i++) {
            if (cut[vertex] != cut[i]) {
                weight += graph[vertex][i];
            }
        }
        return weight;
    }

    [[nodiscard]] int cutWeight(const std::vector<bool> &cut) const {
        int weight = 0;
        for (int i = 0; i < size; i++) {
            if (cut[i]) {
                weight += vertexWeight(cut, i);
            }
        }
        return weight;
    }
};

unsigned long long resursiveCounter;

void DFS_BB(const Graph &graph, int maxPartitionSize,
            std::vector<bool> &cut, int index, int count, int &bestWeight) {
    resursiveCounter++;

    // end stop of the recursion
    if (index >= graph.size) {
        return;
    }

    // count this vertex cut
    int currentWeight = graph.cutWeight(cut);

    // this cannot be better solution
//    if (currentWeight >= bestWeight) {
//        return;
//    }

    if (count == maxPartitionSize) {

        if (currentWeight < bestWeight) {
            bestWeight = currentWeight;
        }

        for (auto i: cut) {
            std::cout << i << " ";
        }

        std::cout << " -> " << currentWeight << std::endl;
        return;
    }

    // try with this vertex
    cut[index] = true;

    // try with this vertex
    DFS_BB(graph, maxPartitionSize, cut, index + 1, count + 1, bestWeight);

    // restore the status of the cut
    cut[index] = false;

    // try without this vertex
    DFS_BB(graph, maxPartitionSize, cut, index + 1, count, bestWeight);
}

int DFS_BB(const Graph &g, int maxPartitionSize) {
    auto cut = std::vector<bool>(g.size, false);
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
    std::cout << "Count of recursive calls: " << resursiveCounter << std::endl;
    return 0;
}

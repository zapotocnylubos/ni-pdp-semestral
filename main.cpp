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
        int cutSize = (int) cut.size();
        int weight = 0;
        for (int i = 0; i < cutSize; i++) {
            if (cut[vertex] == cut[i]) continue;
            weight += graph[vertex][i];
        }
        return weight;
    }

    [[nodiscard]] int cutWeight(const std::vector<bool> &cut) const {
        int cutSize = (int) cut.size();
        int weight = 0;
        for (int i = 0; i < cutSize; i++) {
            if (!cut[i]) continue;
            weight += vertexWeight(cut, i);
        }
        return weight;
    }
};

unsigned long long recursiveCounter;
unsigned long long upperBoundCounter;
unsigned long long lowerBoundCounter;

void DFS_BB(const Graph &graph, int maxPartitionSize,
            std::vector<bool> &cut, int count, int index,
            int currentWeight, int &bestWeight) {
    recursiveCounter++;

    // end stop of the recursion
    if (index > graph.size) {
        return;
    }

    // this cannot be better solution
    if (currentWeight >= bestWeight) {
        upperBoundCounter++;
        return;
    }

    // end stop of the recursion
    if (count > maxPartitionSize) {
        return;
    }

    if (index == graph.size) {
        if (count != maxPartitionSize) {
            return;
        }

        if (currentWeight < bestWeight) {
            bestWeight = currentWeight;
        }

        for (auto i: cut) {
            std::cout << i << " ";
        }

        std::cout << "-> " << currentWeight << std::endl;
        return;
    }

    //generate lower bound for vertex cut
    int lowerBound = 0;
    for (int i = index + 1; i < graph.size; i++) {
        // here problem
        cut.push_back(true);
        int with = graph.vertexWeight(cut, i);
        cut.pop_back();

        cut.push_back(false);
        int without = graph.vertexWeight(cut, i);
        cut.pop_back();

        lowerBound += std::min(with, without);
    }

//    if (currentWeight + lowerBound >= bestWeight) {
//        lowerBoundCounter++;
//        return;
//    }

    // try without this vertex
    cut.push_back(false);

    // try without this vertex (need to extend current cut)
    DFS_BB(graph, maxPartitionSize,
           cut, count, index + 1,
           currentWeight + graph.vertexWeight(cut, index), bestWeight);

    // restore the status of the cut
    cut.pop_back();

    // try with this vertex
    cut.push_back(true);

    // try with this vertex (need to extend current cut)
    DFS_BB(graph, maxPartitionSize,
           cut, count + 1, index + 1,
           currentWeight + graph.vertexWeight(cut, index), bestWeight);

    // restore the status of the cut
    cut.pop_back();
}

int DFS_BB(const Graph &g, int maxPartitionSize) {
    std::vector<bool> cut;
    int bestWeight = std::numeric_limits<int>::max();

    DFS_BB(g, maxPartitionSize, cut, 0, 0, 0, bestWeight);
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

    std::cout << "Count of recursive calls: " << recursiveCounter << std::endl;
    std::cout << "Count of upper bound cuts: " << upperBoundCounter << std::endl;
    std::cout << "Count of lower bound cuts: " << lowerBoundCounter << std::endl;
    return 0;
}

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

    [[nodiscard]] int vertexWeight(const bool *cut, int cutSize, int vertex) const {
        int weight = 0;
        for (int i = 0; i < cutSize; i++) {
            if (cut[vertex] == cut[i]) continue;
            weight += graph[vertex][i];
        }
        return weight;
    }

    [[nodiscard]] int cutLowerBound(bool *cut, int from) const {
        //generate lower bound for vertex cut
        int lowerBound = 0;
        for (int vertex = from; vertex < size; vertex++) {
            // try without this vertex
            int without = vertexWeight(cut, from, vertex);

            // try with this vertex
            cut[vertex] = true;
            int with = vertexWeight(cut, from, vertex);

            // restore cut
            cut[vertex] = false;

            // sum up minimums of the possible states
            lowerBound += std::min(with, without);
        }
        return lowerBound;
    }

//    [[nodiscard]] int cutWeight(const std::vector<bool> &cut) const {
//        int cutSize = (int) cut.size();
//        int weight = 0;
//        for (int i = 0; i < cutSize; i++) {
//            if (!cut[i]) continue;
//            weight += vertexWeight(cut, i);
//        }
//        return weight;
//    }
};

unsigned long long recursiveCounter;
unsigned long long upperBoundCounter;
unsigned long long lowerBoundCounter;

void DFS_BB(const Graph &graph, int maxPartitionSize,
            bool *cut, int count, int index,
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

        for (int i = 0; i < index; i++) {
            std::cout << cut[i] << " ";
        }

        std::cout << "-> " << currentWeight << std::endl;
        return;
    }

    //generate lower bound for vertex cut
    int lowerBound = graph.cutLowerBound(cut, index);

    if (currentWeight + lowerBound >= bestWeight) {
        lowerBoundCounter++;
        return;
    }

    // try with this vertex
    cut[index] = true;

    // try with this vertex (need to extend current cut)
    DFS_BB(graph, maxPartitionSize,
           cut, count + 1, index + 1,
           currentWeight + graph.vertexWeight(cut, index, index), bestWeight);

    // restore the status of the cut
    cut[index] = false;

    // try without this vertex (need to extend current cut)
    DFS_BB(graph, maxPartitionSize,
           cut, count, index + 1,
           currentWeight + graph.vertexWeight(cut, index, index), bestWeight);
}

int DFS_BB(const Graph &g, int maxPartitionSize) {
    auto cut = new bool[g.size];// std::vector<bool>(g.size, false);
    int bestWeight = std::numeric_limits<int>::max();

    DFS_BB(g, maxPartitionSize, cut, 0, 0, 0, bestWeight);
    delete[] cut;
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

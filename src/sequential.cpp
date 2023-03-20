#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>

struct Graph {
    int size;
    int **graph;

    explicit Graph(int **graph, int size) : graph(), size(size) {}

    explicit Graph(const std::string &file) {
        std::ifstream infile(file);

        if (infile.fail()) {
            throw std::invalid_argument("File does not exist");
        }

        infile >> size;

        graph = new int *[size];
        for (int i = 0; i < size; i++) {
            graph[i] = new int[size];
        }

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                infile >> graph[i][j];
            }
        }

        infile.close();
    }

    ~Graph() {
        for (int i = 0; i < size; i++) {
            delete[] graph[i];
        }
        delete[] graph;
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

    [[nodiscard]] int cutWeight(const bool *cut, int cutSize) const {
        int weight = 0;
        for (int i = 0; i < cutSize; i++) {
            if (!cut[i]) continue;
            weight += vertexWeight(cut, cutSize, i);
        }
        return weight;
    }
};

unsigned long long recursiveCounter;
unsigned long long upperBoundCounter;
unsigned long long lowerBoundCounter;

unsigned long long indexBiggerStopCounter;
unsigned long long countBiggerStopCounter;
unsigned long long countWeightNoImprovementStopCounter;
unsigned long long countWeightImprovementStopCounter;

// Pro dalsi ukol (paralelismus) se nesmi nic predavat referenci/pointerem
void DFS_BB(const Graph &graph, int maxPartitionSize,
            bool *cut, int count, int index,
            int currentWeight, int &bestWeight) {
    recursiveCounter++;

    // this cannot be better solution
    if (currentWeight >= bestWeight) {
        upperBoundCounter++;
        return;
    }

    // end stop of the recursion
//    if (count > maxPartitionSize) {
//        countBiggerStopCounter++;
//        return;
//    }

    if (count == maxPartitionSize) {
        // currentWeight was pre-computed for whole cut
        if (currentWeight >= bestWeight) {
            countWeightNoImprovementStopCounter++;
            return;
        }

        countWeightImprovementStopCounter++;

        bestWeight = currentWeight;

        for (int i = 0; i < graph.size; i++) {
            std::cout << cut[i] << " ";
        }

        std::cout << "-> " << currentWeight << std::endl;
        return;
    }

    // end stop of the recursion
//    if (index == graph.size) {
//        indexBiggerStopCounter++;
//        return;
//    }

    // end stop of the recursion
    if (index + 1 == graph.size) {
//        indexBiggerStopCounter++;
        return;
    }

    //generate lower bound for vertex cut
    int lowerBound = graph.cutLowerBound(cut, index);

    // there is no possibility for a better solution
    if (currentWeight + lowerBound >= bestWeight) {
        lowerBoundCounter++;
        return;
    }

    // try with this vertex
    cut[index] = true;

    // calculate new weight
    int nextWeight = currentWeight + graph.vertexWeight(cut, index, index);

    // this can be better solution
    if (nextWeight < bestWeight) {
        if (count + 1 == maxPartitionSize && nextWeight < bestWeight) {
            // pre-compute currentWeight for whole cut
            nextWeight = graph.cutWeight(cut, graph.size);

            if (nextWeight < bestWeight) {
                // try with this vertex (need to extend current cut)
                DFS_BB(graph, maxPartitionSize, cut, count + 1, index + 1, nextWeight, bestWeight);
            }
        } else {
            DFS_BB(graph, maxPartitionSize, cut, count + 1, index + 1, nextWeight, bestWeight);
        }
    }

    // restore the status of the cut
    cut[index] = false;

    // calculate new weight
    nextWeight = currentWeight + graph.vertexWeight(cut, index, index);

    // this cannot be better solution
    if (nextWeight >= bestWeight) {
        return;
    }

    // try without this vertex (need to extend current cut)
    DFS_BB(graph, maxPartitionSize, cut, count, index + 1, nextWeight, bestWeight);
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

    int bestWeight = DFS_BB(g, maxPartitionSize);

    std::cout << "Global minimum is: " << bestWeight << std::endl;

    std::cout << "Count of recursive calls: " << recursiveCounter << std::endl;
    std::cout << "Count of upper bound cuts: " << upperBoundCounter << std::endl;
    std::cout << "Count of lower bound cuts: " << lowerBoundCounter << std::endl;
    std::cout << "Count of bigger index recursion stops: " << indexBiggerStopCounter << std::endl;
    std::cout << "Count of bigger count recursion stops: " << countBiggerStopCounter << std::endl;
    std::cout << "Count of weight no improvement recursion stops: " << countWeightNoImprovementStopCounter << std::endl;
    std::cout << "Count of weight improvement recursion stops: " << countWeightImprovementStopCounter << std::endl;
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <chrono>

#include <omp.h>

struct Cut {
    bool *data;
    int size;

    explicit Cut(int size) : data(new bool[size]), size(size) {
        for (int i = 0; i < size; i++) {
            data[i] = false;
        }
    };

    Cut(const Cut &cut) : data(new bool[cut.size]), size(cut.size) {
//        #pragma omp parallel for default(none) shared(cut)
        for (int i = 0; i < cut.size; i++) {
            data[i] = cut[i];
        }
    }

    ~Cut() {
        delete[] data;
    }

    bool &operator[](int index) const {
        return data[index];
    }
};

struct Graph {
    int **data;
    int size;

    explicit Graph() : data(nullptr), size(0) {}

    explicit Graph(const std::string &file) {
        std::ifstream infile(file);

        if (infile.fail()) {
            throw std::invalid_argument("File does not exist");
        }

        infile >> size;

        data = new int *[size];
        for (int i = 0; i < size; i++) {
            data[i] = new int[size];
        }

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                infile >> data[i][j];
            }
        }

        infile.close();
    }

    Graph(const Graph &graph) : data(new int *[graph.size]), size(graph.size) {
        for (int i = 0; i < size; i++) {
            data[i] = new int[size];
            for (int j = 0; j < size; j++) {
                data[i][j] = graph[i][j];
            }
        }
    }

    ~Graph() {
        for (int i = 0; i < size; i++) {
            delete[] data[i];
        }
        delete[] data;
    }

    int *operator[](int index) const {
        return data[index];
    }

    [[nodiscard]] int vertexWeight(const Cut &cut, int cutSize, int vertex) const {
        int weight = 0;

//        #pragma omp parallel for default(none) shared(cut, cutSize, vertex) reduction(+:weight)
        for (int i = 0; i < cutSize; i++) {
            if (cut[vertex] == cut[i]) continue;
            weight += data[vertex][i];
        }

        return weight;
    }

    [[nodiscard]] int cutLowerBound(const Cut &cut, int from) const {
        // generate lower bound for vertex cut
        int lowerBound = 0;

//        #pragma omp parallel default(none) shared(from) firstprivate(cut) reduction(+:lowerBound)
        for (int vertex = from; vertex < size; vertex++) {
            int with, without;

            // try without this vertex
            without = vertexWeight(cut, from, vertex);

            // try with this vertex
            cut[vertex] = true;
            with = vertexWeight(cut, from, vertex);
            cut[vertex] = false;

            // sum up minimums of the possible states
            lowerBound += std::min(with, without);
        }

        return lowerBound;
    }

    [[nodiscard]] int cutWeight(const Cut &cut) const {
        int weight = 0;

        for (int i = 0; i < cut.size; i++) {
            if (!cut[i]) continue;
            weight += vertexWeight(cut, cut.size, i);
        }

        return weight;
    }
};

unsigned long long recursiveCounter;
unsigned long long upperBoundCounter;
unsigned long long lowerBoundCounter;

Graph *graph;
int maxPartitionSize, bestWeight;

void DFS_BB(const Cut &cut, int count, int index, int currentWeight) {
    #pragma omp atomic update
    recursiveCounter++;

    // end stop of the recursion
    if (index > graph->size) {
        return;
    }

    // this cannot be better solution
    if (currentWeight >= bestWeight) {
        #pragma omp atomic update
        upperBoundCounter++;
        return;
    }

    // end stop of the recursion
    if (count > maxPartitionSize) {
        return;
    }

    if (index == graph->size) {
        if (count != maxPartitionSize) {
            return;
        }

        #pragma omp critical
        {
            if (currentWeight < bestWeight) {
                bestWeight = currentWeight;
            }
        }

        for (int i = 0; i < index; i++) {
            std::cout << cut[i] << " ";
        }

        std::cout << "-> " << currentWeight << std::endl;
        return;
    }

    //generate lower bound for vertex cut
    int lowerBound = graph->cutLowerBound(cut, index);

    if (currentWeight + lowerBound >= bestWeight) {
        #pragma omp atomic update
        lowerBoundCounter++;
        return;
    }

    // try with this vertex (need to extend current cut)
    #pragma omp task default(none) firstprivate(graph, cut, count, index, currentWeight)
    {
        // try with this vertex
        cut[index] = true;

        // compute weight with this vertex
        int nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

        DFS_BB(cut, count + 1, index + 1, nextWeight);
    }

    // try without this vertex (need to extend current cut)
    #pragma omp task default(none) firstprivate(graph, cut, count, index, currentWeight)
    {
        // restore the status of the cut
        cut[index] = false;

        // compute weight without this vertex
        int nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

        DFS_BB(cut, count, index + 1, nextWeight);
    }
}

int DFS_BB() {
    bestWeight = std::numeric_limits<int>::max();

    #pragma omp parallel default(none) firstprivate(graph)
    {
        #pragma omp single
        DFS_BB(Cut(graph->size), 0, 0, 0);
    }

    return bestWeight;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: ./a.out [maxPartitionSize] [file]" << std::endl;
        return 1;
    }

    maxPartitionSize = std::stoi(argv[1]);
    auto file = argv[2];

    std::cout << "Max partition size: " << maxPartitionSize << std::endl;
    std::cout << "File: " << file << std::endl;

    graph = new Graph(file);

    std::cout << "OMP available processors: " << omp_get_num_procs() << std::endl;
    std::cout << "OMP available threads: " << omp_get_max_threads() << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    bestWeight = DFS_BB();

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Global minimum is: " << bestWeight << std::endl;

    std::cout << "Time: " << duration.count() << "ms" << std::endl;
    std::cout << "Count of recursive calls: " << recursiveCounter << std::endl;
    std::cout << "Count of upper bound cuts: " << upperBoundCounter << std::endl;
    std::cout << "Count of lower bound cuts: " << lowerBoundCounter << std::endl;
    return 0;
}

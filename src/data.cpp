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

    friend std::ostream &operator<<(std::ostream &os, const Cut &cut) {
        for (int i = 0; i < cut.size; i++) {
            os << cut[i] << " ";
        }
        return os;
    }
};

struct State {
    Cut cut;
    int count;
    int index;
    int currentWeight;
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

        for (int i = 0; i < cutSize; i++) {
            if (cut[vertex] == cut[i]) continue;
            weight += data[vertex][i];
        }

        return weight;
    }

    [[nodiscard]] int cutLowerBound(const Cut &cut, int from) const {
        // generate lower bound for vertex cut
        int lowerBound = 0;

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

void DFS_BB(const State &state) {
    const Cut &cut = state.cut;
    int count = state.count;
    int index = state.index;
    int currentWeight = state.currentWeight;

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

        std::cout << cut << "-> " << currentWeight << std::endl;
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
    // try with this vertex
    cut[index] = true;

    // compute weight with this vertex
    int nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

    DFS_BB({cut, count + 1, index + 1, nextWeight});

    // try without this vertex (need to extend current cut)
    // restore the status of the cut
    cut[index] = false;

    // compute weight without this vertex
    nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

    DFS_BB({cut, count, index + 1, nextWeight});
}

int DFS_BB() {
    bestWeight = std::numeric_limits<int>::max();

    auto initialStates = std::vector<State>();
    initialStates.push_back({Cut(graph->size), 0, 0, 0});

//    const int initialStatesSufficientCount = 100;

    for (int a = 0 ; a < 5; a++) {
        int k = (int) initialStates.size();
        for (int l = a; l < k; l++) {
            auto state = initialStates[l];
            for (int i = state.index; i < graph->size - maxPartitionSize; i++) {
//                if (initialStates.size() > )
                Cut cut = state.cut;
                cut[i] = true;
                int weight = state.currentWeight + graph->vertexWeight(cut, graph->size, i);
                initialStates.push_back({cut, state.count + 1, state.index + i + 1, weight});
            }
        }
    }

    for (const auto &state: initialStates) {
        auto cut = state.cut;
        std::cout << cut << std::endl;
    }

//    return 0;

    #pragma omp parallel for num_threads(4) schedule(dynamic) default(none) shared(initialStates)
    for (const auto &state: initialStates) {
//        DFS_BB({Cut(graph->size), 0, 0, 0});
        DFS_BB(state);
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

    graph = new Graph(file);

    auto start = std::chrono::high_resolution_clock::now();

    bestWeight = DFS_BB();

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Global minimum is: " << bestWeight << std::endl;

    std::cout << "Time: " << duration.count() << "ms" << std::endl;
    std::cout << "Count of recursive calls: " << recursiveCounter << std::endl;
    std::cout << "Count of upper bound cuts: " << upperBoundCounter << std::endl;
    std::cout << "Count of lower bound cuts: " << lowerBoundCounter << std::endl;
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <chrono>
#include <queue>
#include <algorithm>
#include <cmath>

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

    Cut& operator=(const Cut& cut) {
        if (this != &cut) {
            delete[] data;
            data = new bool[cut.size];
            size = cut.size;
            for (int i = 0; i < cut.size; i++) {
                data[i] = cut[i];
            }
        }
        return *this;
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

    State(const Cut &cut, int count, int index, int currentWeight) :
            cut(cut), count(count),
            index(index), currentWeight(currentWeight) {}

    State& operator=(const State& state) {
        if (this != &state) {
            cut = state.cut;
            count = state.count;
            index = state.index;
            currentWeight = state.currentWeight;
        }
        return *this;
    }

    bool operator<(const State &state) const {
        return currentWeight < state.currentWeight;
    }

    friend std::ostream &operator<<(std::ostream &os, const State &state) {
        os << state.cut << '[' << state.count << ", " << state.index << ", " << state.currentWeight << ']';
        return os;
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

    // end stop of the recursion
    if (count > maxPartitionSize) {
        return;
    }

    // this cannot be better solution
    if (currentWeight >= bestWeight) {
        #pragma omp atomic update
        upperBoundCounter++;
        return;
    }

    if (index == graph->size) {
        if (count != maxPartitionSize) {
            return;
        }

        if (currentWeight >= bestWeight) {
            return;
        }

        #pragma omp critical
        {
            if (currentWeight < bestWeight) {
                bestWeight = currentWeight;
            }
        }

        std::cout << state << "-> " << currentWeight << std::endl;
        return;
    }

    // pre-end stop of the recursion
    // moving any way is not possible
    if (index + 1 > graph->size) {
        return;
    }

    //generate lower bound for vertex cut
    int lowerBound = graph->cutLowerBound(cut, index);

    if (currentWeight + lowerBound >= bestWeight) {
        #pragma omp atomic update
        lowerBoundCounter++;
        return;
    }

    // try with this vertex
    cut[index] = true;

    // compute weight with this vertex
    int nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

    if (nextWeight < bestWeight && count + 1 <= maxPartitionSize) {
        DFS_BB(State(cut, count + 1, index + 1, nextWeight));
    }

    // try without this vertex
    cut[index] = false;

    // compute weight without this vertex
    nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

    if (nextWeight < bestWeight) {
        DFS_BB(State(cut, count, index + 1, nextWeight));
    }
}

int DFS_BB(int countOfInitialStates = (int) std::pow(2, 5)) {
    bestWeight = std::numeric_limits<int>::max();

    auto initialStatesQ = std::queue<State>();
    initialStatesQ.push(State(Cut(graph->size), 0, 0, 0));

    int createdInitialStates = 0;

    while (createdInitialStates < countOfInitialStates) {
        auto state = initialStatesQ.front();
        initialStatesQ.pop();
        createdInitialStates--;

        int withoutAccumulator = state.currentWeight;

        for (int i = state.index; i < graph->size - maxPartitionSize + state.count; i++) {
            Cut cut = state.cut;

            // try with this vertex
            cut[i] = true;
            int nextWeight = withoutAccumulator + graph->vertexWeight(cut, i, i);
            initialStatesQ.push(State(cut, state.count + 1, i + 1, nextWeight));

            createdInitialStates++;

            // try without this vertex
            cut[i] = false;
            nextWeight = (withoutAccumulator += graph->vertexWeight(cut, i, i));
            initialStatesQ.push(State(cut, state.count, i + 1, nextWeight));

            createdInitialStates++;
        }
    }

    auto initialStates = std::vector<State>();
    while (!initialStatesQ.empty()) {
        initialStates.push_back(initialStatesQ.front());
        initialStatesQ.pop();
    }

    for (const auto& state: initialStates) {
        std::cout << state << std::endl;
    }

    std::cout << std::endl << std::endl << std::endl;

    std::sort(initialStates.begin(), initialStates.end());

    // print all initial states
    for (const auto& state: initialStates) {
        std::cout << state << std::endl;
    }

    std::cout << std::endl << std::endl << std::endl;

    #pragma omp parallel for num_threads(4) schedule(static, 4) default(none) shared(bestWeight, initialStates)
    for (const auto &state: initialStates) {
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
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Global minimum is: " << bestWeight << std::endl;

    std::cout << "Time: " << duration.count() << "ms" << std::endl;
    std::cout << "Count of recursive calls: " << recursiveCounter << std::endl;
    std::cout << "Count of upper bound cuts: " << upperBoundCounter << std::endl;
    std::cout << "Count of lower bound cuts: " << lowerBoundCounter << std::endl;
    return 0;
}

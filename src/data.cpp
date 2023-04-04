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

    explicit Cut(const Cut &cut) : data(new bool[cut.size]), size(cut.size) {
        for (int i = 0; i < cut.size; i++) {
            data[i] = cut[i];
        }
    }

    ~Cut() {
        delete[] data;
    }

    Cut &operator=(const Cut &cut) {
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


    State &operator=(const State &state) {
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

            // try with this vertex
            cut[vertex] = true;
            with = vertexWeight(cut, from, vertex);

            // try without this vertex
            cut[vertex] = false;
            without = vertexWeight(cut, from, vertex);

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

void DFS_BB(State state) {
    Cut &cut = state.cut;
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

    if (nextWeight < bestWeight) {
        state.count = count + 1;
        state.index = index + 1;
        state.currentWeight = nextWeight;

        DFS_BB(state);
    }

    // try without this vertex
    cut[index] = false;

    // compute weight without this vertex
    nextWeight = currentWeight + graph->vertexWeight(cut, index, index);

    if (nextWeight < bestWeight) {
        state.count = count;
        state.index = index + 1;
        state.currentWeight = nextWeight;

        DFS_BB(state);
    }
}

int DFS_BB() {
    bestWeight = std::numeric_limits<int>::max();

    auto initialStates = std::vector<State *>();
    auto initialStatesQ = std::queue<State *>();

    initialStatesQ.push(new State(Cut(graph->size), 0, 0, 0));

    while (!initialStatesQ.empty() && initialStatesQ.front()->index < (2 * maxPartitionSize) / 3) {
        auto state = *initialStatesQ.front();
        initialStatesQ.pop();

        auto cut = Cut(state.cut);

        int nextWeight = state.currentWeight + graph->vertexWeight(cut, state.index, state.index);
        initialStatesQ.push(new State(cut, state.count, state.index + 1, nextWeight));

        if (state.count + 1 > maxPartitionSize) continue;

        cut = Cut(state.cut);
        cut[state.index] = true;

        nextWeight = state.currentWeight + graph->vertexWeight(cut, state.index, state.index);
        initialStatesQ.push(new State(cut, state.count + 1, state.index + 1, nextWeight));
    }

    while (!initialStatesQ.empty()) {
        initialStates.push_back(initialStatesQ.front());
        initialStatesQ.pop();
    }

    std::cout << initialStates.size() << std::endl;

    std::sort(initialStates.begin(), initialStates.end());

    #pragma omp parallel for num_threads(4) schedule(dynamic) default(none) shared(initialStates)
    for (const auto state: initialStates) {
        DFS_BB(*state);
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

    delete graph;

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Global minimum is: " << bestWeight << std::endl;

    std::cout << "Time: " << duration.count() << "ms" << std::endl;
    std::cout << "Count of recursive calls: " << recursiveCounter << std::endl;
    std::cout << "Count of upper bound cuts: " << upperBoundCounter << std::endl;
    std::cout << "Count of lower bound cuts: " << lowerBoundCounter << std::endl;
    return 0;
}

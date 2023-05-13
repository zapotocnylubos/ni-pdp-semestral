#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <chrono>
#include <queue>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include <omp.h>
#include <mpi.h>

enum MPITag {
    SLAVE_INITIALIZED,
    SLAVE_INITIALIZE_GRAPH_SIZE,
    SLAVE_INITIALIZE_GRAPH,
    SLAVE_TASK,
    SLAVE_JOB_STATE,
    SLAVE_JOB_CUT,
    SLAVE_AVAILABLE,
    SLAVE_RESULT
};

enum MPITask {
    JOB,
    SYNC,
    TERMINATE
};


struct Cut {
    int size;
    bool *data;

    explicit Cut(int size) : size(size), data(new bool[size]) {
        for (int i = 0; i < size; i++) {
            data[i] = false;
        }
    };

    Cut(int size, const bool *data) : size(size), data(new bool[size]) {
        for (int i = 0; i < size; i++) {
            this->data[i] = data[i];
        }
    }

    Cut(const Cut &cut) : size(cut.size), data(new bool[cut.size]) {
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
            size = cut.size;
            data = new bool[cut.size];
            for (int i = 0; i < cut.size; i++) {
                data[i] = cut[i];
            }
        }
        return *this;
    }

    bool &operator[](int index) const {
        return data[index];
    }

    [[nodiscard]] bool *flatten() const {
        bool *result = new bool[size];
        for (int i = 0; i < size; i++) {
            result[i] = data[i];
        }
        return result;
    }

    friend std::ostream &operator<<(std::ostream &os, const Cut &cut) {
        for (int i = 0; i < cut.size; i++) {
            os << cut[i] << " ";
        }
        return os;
    }
};

struct State {
    int count;
    int index;
    int currentWeight;
    Cut cut;

    State(int count, int index, int currentWeight, const Cut &cut) :
            count(count), index(index),
            currentWeight(currentWeight), cut(cut) {}

    explicit State(int *data) : count(data[0]), index(data[1]), currentWeight(data[2]), cut(data[3]) {}

    State &operator=(const State &state) {
        if (this != &state) {
            count = state.count;
            index = state.index;
            currentWeight = state.currentWeight;
            cut = state.cut;
        }
        return *this;
    }

    bool operator<(const State &state) const {
        return currentWeight < state.currentWeight;
    }

    friend std::ostream &operator<<(std::ostream &os, const State &state) {
        return os << state.cut;
    }
};

struct Graph {
    int size = 0;
    int **data = nullptr;

    explicit Graph() : size(0), data(nullptr) {}

    explicit Graph(const std::string &file) {
        std::ifstream infile(file);

        if (infile.fail()) {
            throw std::invalid_argument("File " + file + " does not exist");
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

    Graph(int size, int **data) : size(size), data(data) {}

    Graph(int size, const int *data) : size(size), data(new int *[size]) {
        for (int i = 0; i < size; i++) {
            this->data[i] = new int[size];
            for (int j = 0; j < size; j++) {
                this->data[i][j] = data[i * size + j];
            }
        }
    }

    Graph(const Graph &graph) : size(graph.size), data(new int *[graph.size]) {
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

    [[nodiscard]] int *flatten() const {
        int *flat = new int[size * size];
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                flat[i * size + j] = data[i][j];
            }
        }
        return flat;
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

    friend std::ostream &operator<<(std::ostream &os, const Graph &graph) {
        for (int i = 0; i < graph.size; i++) {
            for (int j = 0; j < graph.size; j++) {
                os << graph[i][j] << " ";
            }
            os << std::endl;
        }
        return os;
    }
};

Graph *graph;
int maxPartitionSize, bestWeight = std::numeric_limits<int>::max();

void DFS_BB(State state) {
    Cut &cut = state.cut;
    int count = state.count;
    int index = state.index;
    int currentWeight = state.currentWeight;

    // end stop of the recursion
    if (index > graph->size) {
        return;
    }

    // this cannot be better solution
    if (currentWeight >= bestWeight) {
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

        return;
    }

    //generate lower bound for vertex cut
    int lowerBound = graph->cutLowerBound(cut, index);

    if (currentWeight + lowerBound >= bestWeight) {
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

std::vector<State> BFS_Expansion(
        const State &initialState = State(0, 0, 0, Cut(graph->size)),
        int depth = (2 * maxPartitionSize) / 3
) {
    auto initialStates = std::vector<State>();
    auto initialStatesQ = std::queue<State>();

    initialStatesQ.push(initialState);

    while (!initialStatesQ.empty() && initialStatesQ.front().index < depth) {
        auto state = initialStatesQ.front();
        initialStatesQ.pop();

        auto cut = Cut(state.cut);

        int nextWeight = state.currentWeight + graph->vertexWeight(cut, state.index, state.index);
        initialStatesQ.push(State(state.count, state.index + 1, nextWeight, cut));

        if (state.count + 1 > maxPartitionSize) continue;

        cut = Cut(state.cut);
        cut[state.index] = true;

        nextWeight = state.currentWeight + graph->vertexWeight(cut, state.index, state.index);
        initialStatesQ.push(State(state.count + 1, state.index + 1, nextWeight, cut));
    }

    while (!initialStatesQ.empty()) {
        initialStates.push_back(initialStatesQ.front());
        initialStatesQ.pop();
    }

    std::sort(initialStates.begin(), initialStates.end());

    return initialStates;
}

void DFS_BB_master(int processes) {
    auto initialStates = BFS_Expansion();

    std::cout << "Initial states: " << initialStates.size() << std::endl;

    while (!initialStates.empty()) {
        auto state = initialStates.back();
        initialStates.pop_back();

        int slave;
        MPI_Recv(&slave, 1, MPI_INT, MPI_ANY_SOURCE, MPITag::SLAVE_AVAILABLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int task = MPITask::JOB;
        MPI_Send(&task, 1, MPI_INT, slave, MPITag::SLAVE_TASK, MPI_COMM_WORLD);

        int stateBuffer[3] = {state.count, state.index, state.currentWeight};
        MPI_Send(stateBuffer, 3, MPI_INT, slave, MPITag::SLAVE_JOB_STATE, MPI_COMM_WORLD);

        MPI_Send(state.cut.data, graph->size, MPI_C_BOOL, slave, MPITag::SLAVE_JOB_CUT, MPI_COMM_WORLD);
    }

    std::cout << "Sending terminate signal to slaves" << std::endl;

    for (int slave = 1; slave < processes; slave++) {
        int task = MPITask::TERMINATE;
        MPI_Send(&task, 1, MPI_INT, slave, MPITag::SLAVE_TASK, MPI_COMM_WORLD);
    }

    std::cout << "Waiting for results from slaves" << std::endl;

    for (int slave = 1; slave < processes; slave++) {
        int result;
        MPI_Recv(&result, 1, MPI_INT, slave, MPITag::SLAVE_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (result < bestWeight) {
            bestWeight = result;
        }
    }
}

void DFS_BB_slave(int rank) {
    int stateBuffer[3];
    bool cutBuffer[graph->size];

    while (true) {
        MPI_Send(&rank, 1, MPI_INT, 0, MPITag::SLAVE_AVAILABLE, MPI_COMM_WORLD);

        int task;
        MPI_Recv(&task, 1, MPI_INT, 0, MPITag::SLAVE_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (task == MPITask::JOB) {
            MPI_Recv(stateBuffer, 3, MPI_INT, 0, MPITag::SLAVE_JOB_STATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            MPI_Recv(cutBuffer, graph->size, MPI_C_BOOL, 0, MPITag::SLAVE_JOB_CUT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            auto state = State(stateBuffer[0], stateBuffer[1], stateBuffer[2], Cut(graph->size, cutBuffer));
            auto initialStates = BFS_Expansion(state, state.index + 2);

            #pragma omp parallel for schedule(dynamic) default(none) shared(initialStates)
            for (const auto &state: initialStates) {
                DFS_BB(state);
            }
        }

        if (task == MPITask::TERMINATE) {
            std::cout << "[" << std::setw(3) << rank << "] Terminating" << std::endl;
            MPI_Send(&bestWeight, 1, MPI_INT, 0, MPITag::SLAVE_RESULT, MPI_COMM_WORLD);
            break;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cout << "Usage: ./a.out [maxPartitionSize] [file]" << std::endl;
        return 1;
    }

    maxPartitionSize = std::stoi(argv[1]);
    auto file = argv[2];

    int MPIError = MPI_Init(&argc, &argv);

    if (MPIError != 0) {
        std::cerr << std::endl << "MPI_Init returned nonzero error code" << std::endl;
        return 1;
    }

    int processes;
    MPI_Comm_size(MPI_COMM_WORLD, &processes);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    std::cout << "[" << std::setw(3) << rank << "] OMP available cores: " << omp_get_num_procs() << std::endl;

    if (rank == 0) {
        std::cout << "MPI processes: " << processes << std::endl;

        graph = new Graph(file);

        int flatGraphBufferSize = graph->size * graph->size;
        int *flatGraphBuffer = graph->flatten();

        for (int i = 1; i < processes; i++) {
            int slave;

            MPI_Recv(&slave, 1, MPI_INT, i,
                     MPITag::SLAVE_INITIALIZED, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            std::cout << "[" << std::setw(3) << slave << "] Initialized" << std::endl;

            MPI_Send(&graph->size, 1, MPI_INT, slave,
                     MPITag::SLAVE_INITIALIZE_GRAPH_SIZE, MPI_COMM_WORLD);

            MPI_Send(flatGraphBuffer, flatGraphBufferSize, MPI_INT, slave,
                     MPITag::SLAVE_INITIALIZE_GRAPH, MPI_COMM_WORLD);
        }

        auto start = std::chrono::high_resolution_clock::now();

        DFS_BB_master(processes);

        delete graph;

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

        std::cout << "Global minimum is: " << bestWeight << std::endl;

        std::cout << "Time: " << duration.count() << "ms" << std::endl;
    } else {
        MPI_Send(&rank, 1, MPI_INT, 0,
                 MPITag::SLAVE_INITIALIZED, MPI_COMM_WORLD);

        int graphSize;
        MPI_Recv(&graphSize, 1, MPI_INT, 0,
                 MPITag::SLAVE_INITIALIZE_GRAPH_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int flatGraphBufferSize = graphSize * graphSize;
        int *flatGraphBuffer = new int[flatGraphBufferSize];
        MPI_Recv(flatGraphBuffer, flatGraphBufferSize, MPI_INT, 0,
                 MPITag::SLAVE_INITIALIZE_GRAPH, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "[" << std::setw(3) << rank << "] Received graph" << std::endl;

        graph = new Graph(graphSize, flatGraphBuffer);

        delete[] flatGraphBuffer;

        DFS_BB_slave(rank);

        delete graph;
    }

    MPI_Finalize();
    return 0;
}

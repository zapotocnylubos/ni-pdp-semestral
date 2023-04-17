cmake_minimum_required(VERSION 3.17)
project(ni_pdp_semestral)

set(CMAKE_CXX_STANDARD 20)

# Find OpenMP library
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

# Add executables
add_executable(ni_pdp_sequential src/sequential.cpp)
add_executable(ni_pdp_task src/task.cpp)
add_executable(ni_pdp_data src/data.cpp)
add_executable(ni_pdp_mpi src/mpi.cpp)

# Link OpenMP library to executables
target_link_libraries(ni_pdp_task PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(ni_pdp_data PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(ni_pdp_mpi PRIVATE MPI::MPI_CXX)

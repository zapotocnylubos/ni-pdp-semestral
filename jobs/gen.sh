#!/bin/sh
set -e

combinations=(
  '5 10_5'
  '5 10_6b'
  '7 20_7'
  '10 20_7'
  '10 20_12'
  '10 30_10'
  '15 30_10'
  '15 30_20'
  '15 40_8'
  '20 40_8'
  '20 40_15'
  '20 40_25'
)

for combination in "${combinations[@]}"; do
  set -- $combination
  suite=$1__$2

  mkdir -p suite/$suite

  mkdir -p suite/$suite/sequential
  sed \
    -e "s|PARAM_A|$1|g" \
    -e "s|GRAPH_SIZE|$2|g" \
    sequential_job.sh > suite/$suite/sequential/sequential_job.sh

  for threads in `seq 2 2 20`; do
    mkdir -p suite/$suite/task/$threads
    sed \
      -e "s|PARAM_A|$1|g" \
      -e "s|GRAPH_SIZE|$2|g" \
      -e "s|/home/zapotlub/pdp/ni_pdp_|OMP_NUM_THREADS=$threads /home/zapotlub/pdp/ni_pdp_|g" \
      task_job.sh > suite/$suite/task/$threads/task_job.sh
  done

  for threads in `seq 2 2 20`; do
    mkdir -p suite/$suite/data/$threads
    sed \
      -e "s|PARAM_A|$1|g" \
      -e "s|GRAPH_SIZE|$2|g" \
      -e "s|/home/zapotlub/pdp/ni_pdp_|OMP_NUM_THREADS=$threads /home/zapotlub/pdp/ni_pdp_|g" \
      data_job.sh > suite/$suite/data/$threads/data_job.sh
  done

  for threads in `seq 4 4 20`; do
    mkdir -p suite/$suite/mpi/$threads
    sed \
      -e "s|PARAM_A|$1|g" \
      -e "s|GRAPH_SIZE|$2|g" \
      -e "s|/home/zapotlub/pdp/ni_pdp_|OMP_NUM_THREADS=$threads /home/zapotlub/pdp/ni_pdp_|g" \
      mpi_job.sh > suite/$suite/mpi/$threads/mpi_job.sh
  done
done

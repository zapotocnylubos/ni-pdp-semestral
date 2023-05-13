#!/bin/sh
set -e

for threads in `seq 2 2 20`; do
  mkdir -p task/$threads
  sed "s|/home/zapotlub|OMP_NUM_THREADS=$threads /home/zapotlub|g" task_job.sh > task/$threads/task_job.sh
done

for threads in `seq 2 2 20`; do
  mkdir -p data/$threads
  sed "s|/home/zapotlub|OMP_NUM_THREADS=$threads /home/zapotlub|g" data_job.sh > data/$threads/data_job.sh
done

for threads in `seq 2 2 20`; do
  mkdir -p mpi/$threads
  sed "s|/home/zapotlub|OMP_NUM_THREADS=$threads /home/zapotlub|g" mpi_job.sh > mpi/$threads/mpi_job.sh
done

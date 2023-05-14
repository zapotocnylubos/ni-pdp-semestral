#!/bin/bash
set -e

generate_suite_type() {
  suite=$1
  type=$2

  mkdir -p suite/$suite

  if [[ -z "$type" || "$type" == "sequential" ]]; then
    mkdir -p suite/$suite/sequential
    cd suite/$suite/sequential
    qrun2 20c 1 pdp_serial /home/zapotlub/pdp/jobs/suite/$suite/sequential/sequential_job.sh
    cd -
  fi

  if [[ -z "$type" || "$type" == "task" ]]; then
    mkdir -p suite/$suite/task
    for threads in `seq 2 2 20`; do
      mkdir -p suite/$suite/task/$threads
      cd suite/$suite/task/$threads
      qrun2 20c 1 pdp_fast /home/zapotlub/pdp/jobs/suite/$suite/task/$threads/task_job.sh
      cd -
    done
  fi

  if [[ -z "$type" || "$type" == "data" ]]; then
    mkdir -p suite/$suite/data
    for threads in `seq 2 2 20`; do
      mkdir -p suite/$suite/data/$threads
      cd suite/$suite/data/$threads
      qrun2 20c 1 pdp_fast /home/zapotlub/pdp/jobs/suite/$suite/data/$threads/data_job.sh
      cd -
    done
  fi

  if [[ -z "$type" || "$type" == "mpi" ]]; then
    mkdir -p suite/$suite/mpi
    for nodes in {2..4}; do
      mkdir -p suite/$suite/mpi/$nodes
      for threads in `seq 4 4 20`; do
        mkdir -p suite/$suite/mpi/$nodes/$threads
        cd suite/$suite/mpi/$nodes/$threads
        qrun2 20c $nodes pdp_fast /home/zapotlub/pdp/jobs/suite/$suite/mpi/$threads/mpi_job.sh
        cd -
      done
    done
  fi
}

[[ -z "$1" ]] && { echo "Parameter 1 is empty" ; exit 1; }

generate_suite_type $1 $2

#!/bin/bash
set -e

suite=5__10_5

qrun2 1c 1 pdp_serial /home/zapotlub/pdp/jobs/suite/$suite/sequential_job.sh

for cores in {1..20}; do
  qrun2 20c 1 pdp_fast /home/zapotlub/pdp/jobs/suite/$suite/task_job.sh
done

for cores in {1..20}; do
  qrun2 20c 1 pdp_fast /home/zapotlub/pdp/jobs/suite/$suite/data_job.sh
done

for nodes in {2..4}; do
  for cores in {1..20}; do
    qrun2 20c $nodes pdp_fast /home/zapotlub/pdp/jobs/suite/$suite/mpi_job.sh
  done
done

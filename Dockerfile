FROM ubuntu:22.04

RUN apt-get update
RUN apt-get install -y tzdata
RUN apt-get install -y build-essential git cmake gdb clang make g++ valgrind
RUN apt-get install -y libomp-dev
RUN apt-get install -y openmpi-bin openmpi-doc libopenmpi-dev

# perf profiler
RUN apt-get install -y linux-tools-generic && \
    cp /usr/lib/linux-tools-*/perf /usr/bin/perf

RUN echo kernel.perf_event_paranoid=1 >> /etc/sysctl.d/99-perf.conf && \
    echo kernel.kptr_restrict=0 >> /etc/sysctl.d/99-perf.conf

WORKDIR /Users/zapotocnylubos/Documents/ni-pdp-semestral

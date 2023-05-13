#!/bin/bash

qstat | grep zapotlub | cut -d' ' -f2 | xargs -I{} qdel {};

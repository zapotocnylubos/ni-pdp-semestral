#!/bin/sh

#  ===========================================================================
# |                                                                           |
# |             COMMAND FILE FOR SUBMITTING SGE JOBS                          |
# |                                                                           |
# |                                                                           |
# | SGE keyword statements begin with #$                                      |
# |                                                                           |
# | Comments begin with #                                                     |
# | Any line whose first non-blank character is a pound sign (#)              |
# | and is not a SGE keyword statement is regarded as a comment.              |
#  ===========================================================================

# Request Bourne shell as shell for job
#$ -S /bin/sh

# Execute the job from the current working directory.
#$ -cwd

# Defines  or  redefines  the  path used for the standard error stream of the job.
#$ -e .

# The path used for the standard output stream of the job.
#$ -o .

# Do not change.
#$ -pe ompi 1

OMP_NUM_THREADS=10 /home/zapotlub/pdp/ni_pdp_task 20 /home/zapotlub/pdp/tests/provided/40_8/in.txt

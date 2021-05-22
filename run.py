#! /usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys

argLength = len(sys.argv)

if argLength == 2:
    instance_file = sys.argv[1]
    cmd = "./exact_cover_seq --in " + instance_file
elif argLength == 3:
    num_machines = sys.argv[1]
    instance_file = sys.argv[2]
    cmd = "mpirun -np " + num_machines + " ./exact_cover_mpi --in " + instance_file
elif argLength == 4:
    num_machines = sys.argv[1]
    instance_file = sys.argv[2]
    if sys.argv[3] == "omp":
        cmd = "mpirun -np " + num_machines + " ./exact_cover_para --in " + instance_file
    else:
        quit()
else:
    quit()

print(cmd)
os.system(cmd)

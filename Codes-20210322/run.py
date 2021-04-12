#! /usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys

argLength = len(sys.argv)

if argLength == 2:
    instance_file = sys.argv[1]
    cmd = "./exact_cover --in " + instance_file
    print(cmd)
elif argLength == 3:
    num_machines = sys.argv[1]
    instance_file = sys.argv[2]
    cmd = "mpirun -np " + num_machines + " exact_cover --in " + instance_file
    print(cmd)
else:
    quit()

os.system(cmd)

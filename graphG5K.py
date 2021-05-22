#! /usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys
import time
import matplotlib.pyplot as plt
import numpy as np
import math
import operator
from collections import Counter
import json
import multiprocessing

cmd = ""
program = ""
step = 4

# file in which we save graph data
graphs = open("graphs/graphs.txt", "w")

# instances
list_instances = [
    "bell12.ec",
    "bell13.ec",
    "bell14.ec",
    "matching8.ec",
    "matching9.ec",
    "matching10.ec",
    "pentomino_6_10.ec",
    "pento_plus_tetra_2x4x10.ec"
]

def start_program(iter_min, iter_max, step, instance_file):
    next = 1
    for i in range(iter_min, iter_max + 1, step):
        cmd = "mpirun -np " + str(i) + " --mca btl_base_warn_component_unused 0 -machinefile $OAR_NODEFILE " + program + " --in instances/" + instance_file + " --progress-report 0 >> graphs/data.txt"
        os.system(cmd)
        print(cmd)
        while(len(open('graphs/data.txt').readlines()) < next):
            time.sleep(0.01)
        next+=1
    return next

def fill_dico(fichier, nb_lines):
    dico=dict()
    fichier.seek(0)
    for l in range(nb_lines - 1):
        line = fichier.readline()
        elems = line.split(" ")
        dico[2 + (step * l)] = float(elems[5].split('s')[0])
    return dico

def graphs_omp(i):
    # on récupère le temps séquentiel
    instance_file = list_instances[i]
    fichier = open("graphs/data.txt", "r+")
    fichier.truncate(0)
    cmd = "./exact_cover_seq --in instances/" + instance_file + " --progress-report 0 >> graphs/data.txt"
    print(cmd)
    os.system(cmd)
    
    while(len(open('graphs/data.txt').readlines()) < 1):
        time.sleep(0.01)
    
    line = fichier.readline()
    elems = line.split(" ")
    t_seq = float(elems[5].split('s')[0])
    print("temps séqentiel = {}\n".format(t_seq))
    fichier.truncate(0)

    # lancement des commandes
    dicos = []
    nb_lines = start_program(iter_min, iter_max, step, instance_file)
    # remplir le dico
    d = fill_dico(fichier, nb_lines)
    # ajouter à la collection de dicos
    dicos.append(d)

    # write
    graphs.write(str(t_seq) + " " + json.dumps(dicos) + " " + instance_file + '\n')
    print(graphs)

    fichier.truncate(0)
    fichier.close()

def launch_graph(i, n):
    # on récupère le temps séquentiel
    instance_file = list_instances[i]
    fichier = open("graphs/data.txt", "r+")
    fichier.truncate(0)
    cmd = "./exact_cover_seq --in instances/" + instance_file + " --progress-report 0 >> graphs/data.txt"
    print(cmd)
    os.system(cmd)
    
    while(len(open('graphs/data.txt').readlines()) < 1):
        time.sleep(0.01)
    
    line = fichier.readline()
    elems = line.split(" ")
    t_seq = float(elems[5].split('s')[0])
    print("temps séqentiel = {}\n".format(t_seq))
    fichier.truncate(0)

    # lancement des commandes
    dicos = []
    nb_lines = start_program(iter_min, iter_max, step, instance_file)
    # remplir le dico
    d = fill_dico(fichier, nb_lines)
    # ajouter à la collection de dicos
    dicos.append(d)

    # write
    graphs.write(str(t_seq) + " " + json.dumps(dicos) + " " + instance_file + '\n')
    print(graphs)

    fichier.truncate(0)
    fichier.close()

argLength = len(sys.argv)
num_machines = 0
max_threads = 0
local = True

if argLength == 2:
    if sys.argv[1] == "omp":
        program = "exact_cover_omp"
        max_threads = multiprocessing.cpu_count()
        cmd = "make omp=1"
if argLength == 3:
    local = False
    if sys.argv[1] == "mpi":
        program = "exact_cover_mpi"
        cmd = "make mpi=1"
    elif sys.argv[1] == "para":
        program = "exact_cover_para"
        cmd = "make para=1"
    else:
        quit()
    num_machines = sys.argv[2]
    if(int(num_machines) <= 2):
        print("Erreur, il faut indiquer un nombre de travailleurs >= 3.\n")
        quit()
else:
    quit()

if !os.path.exists("graphs"):
    os.system("mkdir graphs")
if !os.path.exists("graphs/mpi"):
    os.system("mkdir graphs/mpi")
if !os.path.exists("graphs/omp"):
    os.system("mkdir graphs/omp")
if !os.path.exists("graphs/para"):
    os.system("mkdir graphs/para")

os.system("make")
os.system(cmd)

iter_min = 3
iter_max = int(num_machines)

for i in range(len(list_instances)):
    print("Start: {}".format(list_instances[i]))
    if local:
        graphs_omp(i)
    else:
        launch_graph(i, num_machines)
    print("End: {}\n".format(list_instances[i]))

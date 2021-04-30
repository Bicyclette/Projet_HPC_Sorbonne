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

# instances
list_instances = [
    "bell12.ec",
    "bell13.ec",
    "bell14.ec",
    "matching8.ec",
    "matching9.ec",
    "matching10.ec",
    "pentomino_6_10.ec",
    "pento_plus_tetra_2x4x10.ec",
    "pento_plus_tetra_8x8_secondary.ec",
]

def start_program(iter_min, iter_max, step, instance_file):
    next = 1
    for i in range(iter_min, iter_max + 1, step):
        cmd = "mpirun -np " + str(i) + " ./exact_cover_mpi --in instances/" + instance_file + " --progress-report 0 >> graphs/data.txt"
        os.system(cmd)
        print(cmd)
        while(len(open('graphs/data.txt').readlines()) < next):
            time.sleep(0.01)
        next+=1
    return next

def fill_dico(fichier, nb_lines):
    dico=dict()
    fichier.seek(0)
    for i in range(1,nb_lines):
        line = fichier.readline()
        elems = line.split(" ")
        dico[i] = float(elems[5].split('s')[0])
    return dico

def draw_fig(dico, file, n, t_seq):
    title = "Temps d'execution en fonction du nombre de processeurs\n Instance : {}\n".format(file)
    fig, ax = plt.subplots(figsize=(10,10))
    plt.title(title)
    dico_list = sorted(dico.items())
    x = []
    y = []
    for k,v in dico.items():
        x.append(k)
        y.append(round(v, 3))
        plt.scatter(int(k), float(v), c="blue")
    plt.grid(True, "both")
    plt.minorticks_on()

    for cx, cy in zip(x, y):
        plt.text(cx, cy, '({}, {})'.format(cx, cy))
    plt.plot(x,y)
    
    plt.xlabel('Nombre de travailleurs')
    plt.ylabel('Temps d\'execution (s)')
    plt.xticks(range(1, int(n)))
    plt.axhline(t_seq, c="red", label = "Temps séquentiel = {}s".format(round(t_seq, 3)))
    plt.legend(fontsize=20)
    plt.savefig("graphs/mpi/{}".format(file.split(".")[0]))

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

    # boucle de lancement des commandes
    dicos = []
    iterations = 1
    if i == len(list_instances) - 1:
        iterations = 1
    for i in range(iterations):
        print("instance : " + instance_file + ", mesure numéro " + str(i) + '\n')
        nb_lines = start_program(iter_min, iter_max, step, instance_file)
        # remplir le dico
        d = fill_dico(fichier, nb_lines)
        # ajouter à la collection de dicos
        dicos.append(d)

    # moyenne
    total = sum(map(Counter, dicos), Counter())
    dico = {k : v/iterations for k,v in total.items()}

    # affichage
    draw_fig(dico, instance_file, n, t_seq)

    fichier.truncate(0)
    fichier.close()

argLength = len(sys.argv)
num_machines = 0
if argLength == 2:
    num_machines = sys.argv[1]
    if(int(num_machines) <= 2):
        print("Erreur, il faut indiquer un nombre de travailleurs >= 3\n")
        quit()
else:
    quit()

os.system("mkdir graphs")
os.system("mkdir graphs/mpi")
os.system("mkdir graphs/omp")
os.system("mkdir graphs/para")

cmd = "make"
cmd = "make mpi=1"
#cmd = "make omp=1"
#cmd = "make para=1"
os.system(cmd)

iter_min = 3
iter_max = int(num_machines)
step = 2

for i in range(len(list_instances)):
    print("Start: {}".format(list_instances[i]))
    launch_graph(i, num_machines)
    print("End: {}\n".format(list_instances[i]))

#! /usr/bin/python3
# -*- coding: utf-8 -*-

import os
import sys
import matplotlib.pyplot as plt
import json

def draw_fig_temps_exec(dico, file, n, t_seq):
    title = ""
    if para:
        title = "INSTANCE : {} (36 threads par machine)\nTemps séquentiel = {}s".format(file, t_seq)
    else:
        title = "INSTANCE : {}\nTemps séquentiel = {}s".format(file, t_seq)
    fig, ax = plt.subplots(figsize=(10,10))
    plt.title(title, fontsize="22")
    dico_list = sorted(dico.items())
    x = []
    y = []
    
    for k,v in dico.items():
        x.append(k)
        y.append(round(v[1], 3))
        plt.scatter(k, v[1], c="blue")
    
    plt.grid(True, "both")
    plt.minorticks_on()

    for cx, cy in zip(x, y):
        plt.text(cx, cy, '({}, {})'.format(cx, cy))
    plt.plot(x,y, label="Temps d'exécution", c="blue")

    if omp:
       plt.xlabel('Nombre de threads')
    else:
       plt.xlabel('Nombre de machines (travailleurs)')
    plt.ylabel("Temps d'exécution (en secondes)")
    plt.legend(fontsize=18)

    if omp:
       plt.savefig("graphs/omp/exec/{}".format(file.split(".")[0]))
    elif mpi:
       plt.savefig("graphs/mpi/exec/{}".format(file.split(".")[0]))
    else:
       plt.savefig("graphs/para/exec/{}".format(file.split(".")[0]))

def draw_fig_acceleration(dico, file, n):
    title = ""
    if para:
        title = "INSTANCE : {} (36 threads par machine)\n".format(file)
    else:
        title = "INSTANCE : {}\n".format(file)
    fig, ax = plt.subplots(figsize=(10,10))
    plt.title(title, fontsize="22")
    dico_list = sorted(dico.items())
    x = []
    y = []
    lineaire = []
    
    for k,v in dico.items():
        x.append(k)
        y.append(round(v[0], 3))
        lineaire.append(float(k))
        plt.scatter(k, v[0], c="blue")
    
    plt.grid(True, "both")
    plt.minorticks_on()

    for cx, cy in zip(x, y):
        plt.text(cx, cy, '({}, {})'.format(cx, cy))
    if not para:
        plt.plot(x,lineaire, label="Accélération linéaire", c="red", linestyle="--")
    plt.plot(x,y, label="Accélération", c="blue")
    plt.legend(fontsize=18)
    
    if omp:
       plt.xlabel('Nombre de threads')
       plt.savefig("graphs/omp/delta_speed/{}".format(file.split(".")[0]))
    elif mpi:
       plt.xlabel('Nombre de machines (travailleurs)')
       plt.savefig("graphs/mpi/delta_speed/{}".format(file.split(".")[0]))
    else:
       plt.xlabel('Nombre de machines (travailleurs)')
       plt.savefig("graphs/para/delta_speed/{}".format(file.split(".")[0]))

data = open("graphs/graphs.txt", "r")

omp = False
mpi = False
para = False

argLength = len(sys.argv)
if argLength == 2:
    if sys.argv[1] == "omp":
        omp = True
    elif sys.argv[1] == "mpi":
        mpi = True
    elif sys.argv[1] == "para":
        para = True
    else:
        quit()
else:
    quit()

nb_lines = len(data.readlines())
data.seek(0)
for l in range(nb_lines):
    line = data.readline()
    elems = line.split(" ")
    t_seq = float(elems[0])
    file = str(elems[len(elems) - 1])
    dico_str = ""
    for i in range(1, len(elems) - 1):
        dico_str += elems[i]
    dico_json = json.loads(dico_str)

    num_machines = 0
    d = dict(dico_json[0])
    for k,v in d.items():
        num_machines = int(k)

    draw_fig_acceleration(d, file, num_machines)
    draw_fig_temps_exec(d, file, num_machines, t_seq)

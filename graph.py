#! /usr/bin/python3
# -*- coding: utf-8 -*-
import os
import sys
import time
import matplotlib.pyplot as plt
import numpy as np
import math
import operator

#instances
list_instances = ["bell12.ec", "bell13.ec"]
iter_min = 2
iter_max = 4
step = 1

def start_program(iter_min, iter_max, step, instance_file, n):
    next = 1
    for i in range(iter_min, iter_max, step):
        nb_lines = len(open('graphs/data.txt').readlines(  ))
        while(nb_lines < next):
            time.sleep(0.01)
        next+=1
        cmd = "mpirun -np " + str(n) + " ./exact_cover_mpi --in instances/" + instance_file + " --progress-report 0 >> graphs/data.txt"
        os.system(cmd)
    while(nb_lines < next):
        nb_lines = len(open('graphs/data.txt').readlines(  ))
        time.sleep(0.01)
    return nb_lines

def fill_dico(fichier, nb_lines):
    dico=dict()
    for i in range(1,nb_lines+1):
        line = fichier.readline()
        elems = line.split(" ")
        dico[i] = float(elems[5].split('s')[0])
    return dico

def draw_fig(dico, file, n):
    title = "Temps d'execution en fonction du nombre de processeurs\n Instance : {}\n Cluster Grid'5000: Paravance à Rennes\n".format(file)
    fig, ax = plt.subplots()
    plt.title(title)
    dico_list = sorted(dico.items())
    x = []
    y = []
    for k,v in dico.items():
        x.append(int(k))
        y.append(float(v))
        plt.scatter(int(k), float(v), c="blue")
    plt.grid(True)
    plt.minorticks_on()
    plt.plot(x,y)
    
    #x,y = zip(*dico_list) 
    #plt.plot(x, y)
    plt.xlabel('Nombre de processeurs')
    plt.ylabel('Temps d\'execution (s)')
    plt.xticks(range(1, int(n)))
    plt.savefig("graphs/mpi/{}".format(file.split(".")[0]))

def launch_graph(i, n):
    instance_file = list_instances[i]
    fichier = open("graphs/data.txt", "w")
    cmd = "./exact_cover_seq --in instances/" + instance_file + " --progress-report 0 >> graphs/data.txt"
    os.system(cmd)
    fichier = open("graphs/data.txt", "r")

    # boucle de lancement des commandes
    nb_lines=start_program(iter_min, iter_max, step, instance_file, n)

    #remplir le dico
    dico = fill_dico(fichier, nb_lines)

    #affichage
    draw_fig(dico, instance_file, n)

    fichier.close()

argLength = len(sys.argv)
if argLength == 2:
    num_machines = sys.argv[1]
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

for i in range(len(list_instances)):
    print("Start : {}".format(list_instances[i]))
    launch_graph(i, num_machines)
    print("End: {}\n".format(list_instances[i]))
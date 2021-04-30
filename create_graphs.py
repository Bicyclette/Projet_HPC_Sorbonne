#! /usr/bin/python3
# -*- coding: utf-8 -*-

import matplotlib.pyplot as plt
import json

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
        plt.scatter(k, v, c="blue")
    plt.grid(True, "both")
    plt.minorticks_on()

    for cx, cy in zip(x, y):
        plt.text(cx, cy, '({}, {})'.format(cx, cy))
    plt.plot(x,y)
    
    plt.xlabel('Nombre de travailleurs')
    plt.ylabel('Temps d\'execution (s)')
    plt.axhline(t_seq, c="red", label = "Temps séquentiel = {}s".format(round(t_seq, 3)))
    plt.legend(fontsize=20)
    plt.savefig("graphs/mpi/{}".format(file.split(".")[0]))

data = open("graphs/graphs.txt", "r")

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
    for k,v in dico_json.items():
        num_machines = int(k)

    draw_fig(dico_json, file, num_machines, t_seq)

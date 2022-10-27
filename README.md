# Projet HPC Sorbonne : Résolution du problème exact_cover
Le but du projet est de paralléliser un programme séquentiel, lequel résolvant une instance du problème de la couverture exacte, un problème d’optimisation combinatoire.
<br/>
Il s’agit d’un des 21 problèmes NP-complets de Karp. Ce projet étant une mise en pratique des notions vues au sein de l’UE HPC à l'Université de la Sorbonne.
<br/>
Deux outils sont utilisés pour paralléliser ce programme : MPI et OpenMP.

## Compilation : 
#### La version omp
- en mode debug => *make omp=1 debug=1*
- en mode release => *make omp=1*
#### La version mpi
- en mode debug => *make mpi=1  debug=1* 
- en mode release => *make mpi=1* 
#### La version omp + mpi
- en mode debug => *make final=1  debug=1* 
- en mode release => *make final=1* 

## Lancement d'un programme :
#### Séquentiel
- ./exact_cover_seq --in instances/bell12.ec
#### Parallèle
Avec n le nombre de machines :
- Version utilisant MPI => mpirun -np n ./exact_cover_mpi --in instances/bell12.ec
- Version utilisant MPI et OpenMP => mpirun -np n ./exact_cover_para --in instances/bell12.ec
 
## Script python 
#### Création du fichier texte contenant les temps d'exécution et accélération du programme
Avec n le nombre de machines :
- ./graphG5K omp
- ./graphG5K mpi n
- ./graphG5K final n

#### Création des graphs à partir du fichier texte contenant les valeurs à afficher
- ./create_graphs omp
- ./create_graphs mpi
- ./create_graphs final

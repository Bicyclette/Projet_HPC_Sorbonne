# Projet HPC Sorbonne : Résolution du problème exact_cover
Le but du projet est de paralléliser un programme séquentiel, lequel résolvant une instance du problème de la couverture exacte, un problème d’optimisation combinatoire. Il s’agit d’un des 21 problèmes NP-complets de Karp. Ce projet étant une mise en pratique des notions vues au sein de l’UE HPC à l'Université de La Sorbonne. Deux outils sont utilisés pour paralléliser ce programme : MPI et OpenMP.

## Compilation : 
#### La version omp
- make omp=0/1 debug=0/1
#### La version mpi
- make mpi=0/1  debug=0/1 
#### La version omp + mpi
- make final=0/1 debug=0/1

## Lancement d'un programme :
#### Séquentiel
- ./exact_cover --in instances/instance.ec
#### Parallèle
Avec n le nombre de machines :
- mpirun -np n ./exact_cover_version --in instances/instance.ec
 
## Script python 
#### Création du fichier texte contenant les temps d'exécution et accélération du programme
Avec n le nombre de machines :
- python graphG5K omp
- python graphG5K ompi n
- python graphG5K final n

#### Création des graphs à partir du fichier texte contenant les valeurs à afficher
- python create_graph omp
- python create_graph mpi
- python create_graph final

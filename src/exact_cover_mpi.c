#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <getopt.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>

int comm_size;
int proc_rank;

double start = 0.0;

char *in_filename = NULL;              // nom du fichier contenant la matrice
bool print_solutions = false;          // affiche chaque solution
long long report_delta = 1e6;          // affiche un rapport tous les ... noeuds
long long next_report;                 // prochain rapport affiché au noeud...
long long max_solutions = 0x7fffffffffffffff;        // stop après ... solutions


struct instance_t {
        int n_items;
        int n_primary;
        int n_options;
        char **item_name;   // potentiellement NULL, sinon de taille n_items
        int *options;       // l'option i contient les objets options[ptr[i]:ptr[i+1]]
        int *ptr;           // taille n_options + 1
};

struct sparse_array_t {
        int len;           // nombre d'éléments stockés
        int capacity;      // taille maximale
        int *p;            // contenu de l'ensemble = p[0:len] 
        int *q;            // taille capacity (tout comme p)
};

struct context_t {
        struct sparse_array_t *active_items;      // objets actifs
        struct sparse_array_t **active_options;   // options actives contenant l'objet i
        int *chosen_options;                      // options choisies à ce stade
        int *child_num;                           // numéro du fils exploré
        int *num_children;                        // nombre de fils à explorer
        int level;                                // nombre d'options choisies
        long long nodes;                          // nombre de noeuds explorés
        long long solutions;                      // nombre de solutions trouvées 
};

static const char DIGITS[62] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
                                'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                                'u', 'v', 'w', 'x', 'y', 'z',
                                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 
                                'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                                'U', 'V', 'W', 'X', 'Y', 'Z'};

enum MSG_TAG
{
	REQUEST_MACHINES,
	JOB_FINISHED,
	MACHINE_POOL,
	LEVEL,
	CHOOSEN_OPTIONS
};

// organisation maître esclave >>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
bool* work; // qui travaille et qui travaille pas, taille = [0:comm_size-1]
int* task_size; // nombre de branchements à effectuer par machine faisant une demande
bool* task_finished; // machines ayant fini de travailler
bool* available_machines; // les variables à true sont les machines dispos pour explorer l'arbre, taille = [0:comm_size-1]

void master()
{
	// create array of request and status
	MPI_Request* request = malloc((comm_size - 1) * 2 * sizeof(MPI_Request));
	MPI_Status* status = malloc((comm_size - 1) * 2 * sizeof(MPI_Status));

	// output variables for MPI_Waitany
	int index_count;
	int* index = calloc((comm_size - 1) * 2, sizeof(int));

	// state of machines
	int available = (comm_size - 1) - 1; // at the beginning only one machine is working

	while(available != (comm_size - 1)) // as long as some slaves are at work
	{
		// bunch of receive
		int offset = comm_size - 1;
		for(int i = 0; i < comm_size - 1; ++i)
			MPI_Irecv(&task_size[i], 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_MACHINES, MPI_COMM_WORLD, &request[i]);
		for(int i = 0; i < comm_size - 1; ++i)
			MPI_Irecv(&task_finished[i], 1, MPI_C_BOOL, MPI_ANY_SOURCE, JOB_FINISHED, MPI_COMM_WORLD, &request[offset + i]);
		
		// wait those
		MPI_Waitsome((comm_size-1) * 2, request, &index_count, index, status);
		
		// check what we got
		for(int i = 0; i < index_count; ++i)
		{
			// get request index
			int id = index[i];
			
			// get sender rank
			int sender = status[id].MPI_SOURCE;
			
			if(id < (comm_size - 1)) // slave asked for a pool of machines
			{
				int num_options = task_size[id];
				int machines = (available < num_options) ? available : num_options;
				for(int j = 0; j < machines; ++j)
				{
					for(int k = 0; k < comm_size - 1; ++k)
					{
						if(!work[k])
						{
							work[k] = true;
							available_machines[k] = true;
							break;
						}
					}
				}
				
				// response
				MPI_Send(available_machines, comm_size - 1, MPI_C_BOOL, sender, MACHINE_POOL, MPI_COMM_WORLD);
				available -= machines;
			}
			else // slave finished his job
			{
				work[sender] = false;
				available++;
			}
		}

		// reset task size and available machines
		for(int i = 0; i < (comm_size - 1); ++i)
		{
			task_size[i] = 0;
			available_machines = false;
		}
	}

	// cleaning
	for(int i = 0; i < (comm_size - 1); ++i)
		MPI_Request_free(&request[i]);
	free(request);
	free(index);
}

bool can_assign_work()
{
	for(int i = 0; i < comm_size; ++i)
	{
		if(!work[i])
			return true;
	}
	return false;
}

// organisation maître esclave <<<<<<<<<<
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

double wtime()
{
        struct timeval ts;
        gettimeofday(&ts, NULL);
        return (double) ts.tv_sec + ts.tv_usec / 1e6;
}

void usage(char **argv)
{
        printf("%s --in FILENAME [OPTIONS]\n\n", argv[0]);
        printf("Options:\n");
        printf("--progress-report N   display a message every N nodes (0 to disable)\n");
        printf("--print-solutions     display solutions when they are found\n");
        printf("--stop-after N        stop the search once N solutions are found\n");
        exit(0);
}


bool item_is_primary(const struct instance_t *instance, int item)
{
        return item < instance->n_primary;
}


void print_option(const struct instance_t *instance, int option)
{
        if (instance->item_name == NULL)
                errx(1, "tentative d'affichage sans noms d'objet");
        for (int p = instance->ptr[option]; p < instance->ptr[option + 1]; p++) {
                int item = instance->options[p];
                printf("%s ", instance->item_name[item]);
        }
        printf("\n");
}

struct sparse_array_t * sparse_array_init(int n)
{
        struct sparse_array_t *S = malloc(sizeof(*S));
        if (S == NULL)
                err(1, "impossible d'allouer un tableau creux");
        S->len = 0;
        S->capacity = n;
        S->p = malloc(n * sizeof(int));
        S->q = malloc(n * sizeof(int));
        if (S->p == NULL || S->q == NULL)
                err(1, "Impossible d'allouer p/q dans un tableau creux");
        for (int i = 0; i < n; i++)
                S->q[i] = n;           // initialement vide
        return S;
}

bool sparse_array_membership(const struct sparse_array_t *S, int x)
{
        return (S->q[x] < S->len);
}

bool sparse_array_empty(const struct sparse_array_t *S)
{
        return (S->len == 0);
}

void sparse_array_add(struct sparse_array_t *S, int x)
{
        int i = S->len;
        S->p[i] = x;
        S->q[x] = i;
        S->len = i + 1;
}

void sparse_array_remove(struct sparse_array_t *S, int x)
{
        int j = S->q[x];
        int n = S->len - 1;
        // échange p[j] et p[n] 
        int y = S->p[n];
        S->p[n] = x;
        S->p[j] = y;
        // met q à jour
        S->q[x] = n;
        S->q[y] = j;
        S->len = n;
}

void sparse_array_unremove(struct sparse_array_t *S)
{
        S->len++;
}

void sparse_array_unadd(struct sparse_array_t *S)
{
        S->len--;
}

bool item_is_active(const struct context_t *ctx, int item)
{
        return sparse_array_membership(ctx->active_items, item);
}

void solution_found(const struct instance_t *instance, struct context_t *ctx)
{
        ctx->solutions++;
        if (!print_solutions)
                return;
        printf("Trouvé une nouvelle solution au niveau %d après %lld noeuds\n", 
                        ctx->level, ctx->nodes);
        printf("Options : \n");
        for (int i = 0; i < ctx->level; i++) {
                int option = ctx->chosen_options[i];
                printf("+ %d : ", option);
                print_option(instance, option);
        }
        printf("\n");

        printf("----------------------------------------------------\n");
}

void cover(const struct instance_t *instance, struct context_t *ctx, int item);

void choose_option(const struct instance_t *instance, struct context_t *ctx, 
                        int option, int chosen_item)
{
        ctx->chosen_options[ctx->level] = option;
        ctx->level++;
        for (int p = instance->ptr[option]; p < instance->ptr[option + 1]; p++) {
                int item = instance->options[p];
                if (item == chosen_item)
                        continue;
                cover(instance, ctx, item);
        }
}

void uncover(const struct instance_t *instance, struct context_t *ctx, int item);

void unchoose_option(const struct instance_t *instance, struct context_t *ctx, 
                        int option, int chosen_item)
{
        for (int p = instance->ptr[option + 1] - 1; p >= instance->ptr[option]; p--) {
                int item = instance->options[p];
                if (item == chosen_item)
                        continue;
                uncover(instance, ctx, item);
        }
        ctx->level--;
}

int choose_next_item(struct context_t *ctx)
{
        int best_item = -1;
        int best_options = 0x7fffffff;
        struct sparse_array_t *active_items = ctx->active_items;
        for (int i = 0; i < active_items->len; i++) {
                int item = active_items->p[i];
                struct sparse_array_t *active_options = ctx->active_options[item];
                int k = active_options->len;
                if (k < best_options) {
                        best_item = item;
                        best_options = k;
                }
        }
        return best_item;
}

void progress_report(const struct context_t *ctx)
{
        double now = wtime();
        printf("Exploré %lld noeuds, trouvé %lld solutions, temps écoulé %.1fs. ", 
                        ctx->nodes, ctx->solutions, now - start);
        int i = 0;
        for (int k = 0; k < ctx->level; k++) {
                if (i > 44)
                        break;
                int n = ctx->child_num[k];
                int m = ctx->num_children[k];
                if (m == 1)
                        continue;
                printf("%c%c ", (n < 62) ? DIGITS[n] : '*', (m < 62) ? DIGITS[m] : '*');
                i++;
        }
        printf("\n"),
        next_report += report_delta;
}

void deactivate(const struct instance_t *instance, struct context_t *ctx, 
                        int option, int covered_item);

void cover(const struct instance_t *instance, struct context_t *ctx, int item)
{
        if (item_is_primary(instance, item))
                sparse_array_remove(ctx->active_items, item);
        struct sparse_array_t *active_options = ctx->active_options[item];
        for (int i = 0; i < active_options->len; i++) {
                int option = active_options->p[i];
                deactivate(instance, ctx, option, item);
        }
}


void deactivate(const struct instance_t *instance, struct context_t *ctx, 
                        int option, int covered_item)
{
        for (int k = instance->ptr[option]; k < instance->ptr[option+1]; k++) {
                int item = instance->options[k];
                if (item == covered_item)
                        continue;
                sparse_array_remove(ctx->active_options[item], option);
        }
}


void reactivate(const struct instance_t *instance, struct context_t *ctx, 
                        int option, int uncovered_item);

void uncover(const struct instance_t *instance, struct context_t *ctx, int item)
{
        struct sparse_array_t *active_options = ctx->active_options[item];
        for (int i = active_options->len - 1; i >= 0; i--) {
                int option = active_options->p[i];
                reactivate(instance, ctx, option, item);
        }
        if (item_is_primary(instance, item))
                sparse_array_unremove(ctx->active_items);
}


void reactivate(const struct instance_t *instance, struct context_t *ctx, 
                        int option, int uncovered_item)
{
        for (int k = instance->ptr[option + 1] - 1; k >= instance->ptr[option]; k--) {
                int item = instance->options[k];
                if (item == uncovered_item)
                        continue;
                sparse_array_unremove(ctx->active_options[item]);
        }
}


struct instance_t * load_matrix(const char *filename)
{
        struct instance_t *instance = malloc(sizeof(*instance));
        if (instance == NULL)
                err(1, "Impossible d'allouer l'instance");
        FILE *in = fopen(filename, "r");
        if (in == NULL)
                err(1, "Impossible d'ouvrir %s en lecture", filename);
        int n_it, n_op;
        if (fscanf(in, "%d %d\n", &n_it, &n_op) != 2)
                errx(1, "Erreur de lecture de la taille du problème\n");
        if (n_it == 0 || n_op == 0)
                errx(1, "Impossible d'avoir 0 objets ou 0 options");
        instance->n_items = n_it;
        instance->n_primary = 0;
        instance->n_options = n_op;
        instance->item_name = malloc(n_it * sizeof(char *));
        instance->ptr = malloc((n_op + 1) * sizeof(int));
        instance->options = malloc(n_it * n_op *sizeof(int));         // surallocation massive
        if (instance->item_name == NULL || instance->ptr == NULL || instance->options == NULL)
                err(1, "Impossible d'allouer la mémoire pour stocker la matrice");


        enum state_t {START, ID, WHITESPACE, BAR, ENDLINE, ENDFILE};
        enum state_t state = START;

        char buffer[256];
        int i = 0;     // prochain octet disponible du buffer
        int n = 0;     // dernier octet disponible du buffer

        char id[65];
        id[64] = 0;    // sentinelle à la fin, quoi qu'il arrive
        int j = 0;     // longueur de l'identifiant en cours de lecture

        int current_item = 0;
        while (state != ENDLINE) {
                enum state_t prev_state = state;
                if (i >= n) {
                        n = fread(buffer, 1, 256, in);
                        if (n == 0) {
                                if (feof(in)) {
                                        state = ENDFILE;
                                }
                                if (ferror(in))
                                        err(1, "erreur lors de la lecture de %s", in_filename);
                        }
                        i = 0;

                }
                if (state == ENDFILE) {
                        // don't examine buffer[i]
                } else if (buffer[i] == '\n') {
                        state = ENDLINE;
                } else if (buffer[i] == '|') {
                        state = BAR;
                } else if (isspace(buffer[i])) {
                        state = WHITESPACE;
                } else {
                        state = ID;
                }

                // traite le caractère lu
                if (state == ID) {
                        if (j == 64)
                                errx(1, "nom d'objet trop long : %s", id);
                        id[j] = buffer[i];
                        j++;
                }
                if (prev_state == ID && state != ID) {
                        id[j] = '\0';
                        if (current_item == instance->n_items)
                                errx(1, "Objet excedentaire : %s", id);
                        for (int k = 0; k < current_item; k++)
                                if (strcmp(id, instance->item_name[k]) == 0)
                                        errx(1, "Nom d'objets dupliqué : %s", id);
                        instance->item_name[current_item] = malloc(j+1);
                        strcpy(instance->item_name[current_item], id);
                        current_item++;
                        j = 0;


                }
                if (state == BAR)
                        instance->n_primary = current_item;
                if (state == ENDFILE)
                        errx(1, "Fin de fichier prématurée");
                // passe au prochain caractère
                i++;
        }
        if (current_item != instance->n_items)
                errx(1, "Incohérence : %d objets attendus mais seulement %d fournis\n", 
                                instance->n_items, current_item);
        if (instance->n_primary == 0)
                instance->n_primary = instance->n_items;

        int current_option = 0;
        int p = 0;       // pointeur courant dans instance->options
        instance->ptr[0] = p;
        bool has_primary = false;
        while (state != ENDFILE) {
                enum state_t prev_state = state;
                if (i >= n) {
                        n = fread(buffer, 1, 256, in);
                        if (n == 0) {
                                if (feof(in)) {
                                        state = ENDFILE;
                                }
                                if (ferror(in))
                                        err(1, "erreur lors de la lecture de %s", in_filename);
                        }
                        i = 0;

                }
                if (state == ENDFILE) {
                        // don't examine buffer[i]
                } else if (buffer[i] == '\n') {
                        state = ENDLINE;
                } else if (buffer[i] == '|') {
                        state = BAR;
                } else if (isspace(buffer[i])) {
                        state = WHITESPACE;
                } else {
                        state = ID;
                }

                // traite le caractère lu
                if (state == ID) {
                        if (j == 64)
                                errx(1, "nom d'objet trop long : %s", id);
                        id[j] = buffer[i];
                        j++;
                }
                if (prev_state == ID && state != ID) {
                        id[j] = '\0';
                        // identifie le numéro de l'objet en question
                        int item_number = -1;
                        for (int k = 0; k < instance->n_items; k++)
                                if (strcmp(id, instance->item_name[k]) == 0) {
                                        item_number = k;
                                        break;
                                }
                        if (item_number == -1)
                                errx(1, "Objet %s inconnu dans l'option #%d", id, current_option);
                        // détecte les objets répétés
                        for (int k = instance->ptr[current_option]; k < p; k++)
                                if (item_number == instance->options[k])
                                        errx(1, "Objet %s répété dans l'option %d\n", 
                                                        instance->item_name[item_number], current_option);
                        instance->options[p] = item_number;
                        p++;
                        has_primary |= item_is_primary(instance, item_number);
                        j = 0;


                }
                if (state == BAR) {
                        errx(1, "Trouvé | dans une option.");
                }
                if ((state == ENDLINE || state == ENDFILE)) {
                        // esquive les lignes vides
                        if (p > instance->ptr[current_option]) {
                                if (current_option == instance->n_options)
                                        errx(1, "Option excédentaire");
                                if (!has_primary)
                                        errx(1, "Option %d sans objet primaire\n", current_option);
                                current_option++;
                                instance->ptr[current_option] = p;
                                has_primary = false;


                        }
                }
                // passe au prochain caractère
                i++;
        }
        if (current_option != instance->n_options)
                errx(1, "Incohérence : %d options attendues mais seulement %d fournies\n", 
                                instance->n_options, current_option);


        fclose(in);
        if (proc_rank ==0)
                fprintf(stderr, "Lu %d objets (%d principaux) et %d options\n", 
                        instance->n_items, instance->n_primary, instance->n_options);
        return instance;
}


struct context_t * backtracking_setup(const struct instance_t *instance)
{
        struct context_t *ctx = malloc(sizeof(*ctx));
        if (ctx == NULL)
                err(1, "impossible d'allouer un contexte");
        ctx->level = 0;
        ctx->nodes = 0;
        ctx->solutions = 0;
        int n = instance->n_items;
        int m = instance->n_options;
        ctx->active_options = malloc(n * sizeof(*ctx->active_options));
        ctx->chosen_options = malloc(n * sizeof(*ctx->chosen_options));
        ctx->child_num = malloc(n * sizeof(*ctx->child_num));
        ctx->num_children = malloc(n * sizeof(*ctx->num_children));
        if (ctx->active_options == NULL || ctx->chosen_options == NULL
                || ctx->child_num == NULL || ctx->num_children == NULL)
                err(1, "impossible d'allouer le contexte");
        ctx->active_items = sparse_array_init(n);

        for (int item = 0; item < instance->n_primary; item++)
                sparse_array_add(ctx->active_items, item);

        for (int item = 0; item < n; item++)
                ctx->active_options[item] = sparse_array_init(m);
        for (int option = 0; option < m; option++)
                for (int k = instance->ptr[option]; k < instance->ptr[option + 1]; k++) {
                        int item = instance->options[k];
                        sparse_array_add(ctx->active_options[item], option);
                }
        return ctx;
}

void solve(const struct instance_t *instance, struct context_t *ctx)
{
	ctx->nodes++;
	if(ctx->nodes == next_report)
		progress_report(ctx);
	if(sparse_array_empty(ctx->active_items))
	{
		solution_found(instance, ctx);
		return;                         /* succès : plus d'objet actif */
	}
	int chosen_item = choose_next_item(ctx);

	struct sparse_array_t *active_options = ctx->active_options[chosen_item];
	if (sparse_array_empty(active_options))
	{
		return;           /* échec : impossible de couvrir chosen_item */
	}
	cover(instance, ctx, chosen_item);
	ctx->num_children[ctx->level] = active_options->len;

	// block de communication
	int begin = 0;
	int end = active_options->len;
	int step = 1;            

	while(!work[proc_rank])
	{
		MPI_Recv(&ctx->level, 1, MPI_INT, MPI_ANY_SOURCE, LEVEL, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(ctx->chosen_options, ctx->level, MPI_INT, MPI_ANY_SOURCE, CHOOSEN_OPTIONS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		for(int i = 0 ; i < ctx->level; i++)
		{
			choose_option(instance, ctx, ctx->chosen_options[i], -1);
		}
		begin = proc_rank;
		step = comm_size;
		work[proc_rank] = true;
	}

	if(proc_rank == 0 && active_options->len >= comm_size && can_assign_work())
	{
		for(int i = 1; i < comm_size; ++i)
		{
			work[i] = true;
			MPI_Send(&ctx->level, 1, MPI_INT, i, LEVEL, MPI_COMM_WORLD);
			MPI_Send(ctx->chosen_options, ctx->level, MPI_INT, i, CHOOSEN_OPTIONS, MPI_COMM_WORLD);
		}
		begin = proc_rank;
		step = comm_size;
	}

	// work
	for(int k = begin; k < end; k+=step)
	{
		int option = active_options->p[k];
		ctx->child_num[ctx->level] = k;
		choose_option(instance, ctx, option, chosen_item);
		solve(instance, ctx);
		if (ctx->solutions >= max_solutions)
			return;
		unchoose_option(instance, ctx, option, chosen_item);
	}

	// uncover
	uncover(instance, ctx, chosen_item);                      // backtrack 
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

	struct option longopts[5] = {
                {"in", required_argument, NULL, 'i'},
                {"progress-report", required_argument, NULL, 'v'},
                {"print-solutions", no_argument, NULL, 'p'},
                {"stop-after", required_argument, NULL, 's'},
                {NULL, 0, NULL, 0}
        };
	char ch;
	while((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
				in_filename = optarg;
				break;
			case 'p':
				print_solutions = true;
				break;
			case 's':
				max_solutions = atoll(optarg);
				break;
			case 'v':
				report_delta = atoll(optarg);
				break;
			default:
				errx(1, "Unknown option\n");
		}
	}
	if (in_filename == NULL)
		usage(argv);
	next_report = report_delta;


	struct instance_t * instance = load_matrix(in_filename);
	struct context_t * ctx = backtracking_setup(instance);
	start = wtime();

	available_machines = calloc(comm_size - 1, sizeof(bool));
	task_size = calloc(comm_size - 1, sizeof(int));
	task_finished = calloc(comm_size - 1, sizeof(bool));
	work = calloc(comm_size - 1, sizeof(bool));
	work[0] = true;
/*
	if(proc_rank == 0)
		master();
	else*/
		solve(instance, ctx);

	// wait all threads
	long long solution;
	MPI_Reduce(&ctx->solutions, &solution, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

	if (proc_rank == 0)
		printf("FINI. Trouvé %lld solutions en %.1fs\n", solution, wtime() - start);
        
	MPI_Finalize();
	free(available_machines);
	free(task_size);
	free(task_finished);
	free(work);
	exit(EXIT_SUCCESS);
}

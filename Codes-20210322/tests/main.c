#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

int rank;
int size;
MPI_Datatype vertex_type;

struct array
{
	int len;
	int * options;
};

struct vertex
{
	int index;
	bool smooth;
	float* pos;
	float* normal;
	//struct array ** active_options;
};

void create_type(struct vertex* v)
{
	MPI_Datatype types[2] = {MPI_INT, MPI_C_BOOL};
	int elementsPerType[2] = {1, 1};
	MPI_Aint rootAddr;
	MPI_Get_address(v, &rootAddr);
	MPI_Aint addr1;
	MPI_Get_address(&v->index, &addr1);
	MPI_Aint addr2;
	MPI_Get_address(&v->smooth, &addr2);
	MPI_Aint addresses[2] = {addr1 - rootAddr, addr2 - rootAddr};

	MPI_Type_create_struct(2, elementsPerType, addresses, types, &vertex_type);
	MPI_Type_commit(&vertex_type);
}

void create_type_ptr(struct vertex* v)
{
	MPI_Datatype types[4] = {MPI_INT, MPI_C_BOOL, MPI_REAL, MPI_REAL};
	int elementsPerType[4] = {1, 1, 3, 3};
	MPI_Aint rootAddr;
	MPI_Get_address(v, &rootAddr);
	MPI_Aint addr1;
	MPI_Get_address(&v->index, &addr1);
	MPI_Aint addr2;
	MPI_Get_address(&v->smooth, &addr2);
	MPI_Aint addr3;
	MPI_Get_address(v->pos, &addr3);
	MPI_Aint addr4;
	MPI_Get_address(v->normal, &addr4);
	MPI_Aint addresses[4] = {addr1 - rootAddr, addr2 - rootAddr, addr3 - rootAddr, addr4 - rootAddr};

	MPI_Type_create_struct(4, elementsPerType, addresses, types, &vertex_type);
	MPI_Type_commit(&vertex_type);
}

void free_type()
{
	MPI_Type_free(&vertex_type);
}

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	struct vertex v;
	v.pos = calloc(3, sizeof(float));
	v.normal = calloc(3, sizeof(float));
	if(rank == 0)
	{
		v.index = 10;
		v.smooth = true;
		v.pos[0] = 1.0f, v.pos[1] = 2.0f; v.pos[2] = 3.0f;
		v.normal[0] = 0.0f, v.normal[1] = 1.0f; v.normal[2] = 0.0f;
	}
	create_type_ptr(&v);

	if(rank == 0)
	{
		MPI_Send(&v, 1, vertex_type, 1, 0, MPI_COMM_WORLD);
	}
	else if(rank == 1)
	{
		MPI_Recv(&v, 1, vertex_type, 0, 0, MPI_COMM_WORLD, NULL);
		char* vsmooth = (v.smooth) ? "true" : "false";
		printf("Processus %d reçoit le vertex : \nindex = %d\nsmooth = %s\n", rank, v.index, vsmooth);
		printf("pos = (%f,%f,%f)\nnormal = (%f,%f,%f)\n", v.pos[0], v.pos[1], v.pos[2], v.normal[0], v.normal[1], v.normal[2]);
	}

	free(v.pos);
	free(v.normal);
	free_type(&v);
	MPI_Finalize();
	return EXIT_SUCCESS;
}

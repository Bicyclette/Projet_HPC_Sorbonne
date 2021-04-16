#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

int rank;
int size;
MPI_Datatype vertex_type;
MPI_Datatype pixel_type;
MPI_Datatype array_type;
MPI_Datatype ptr_array_type;

typedef struct Pixel Pixel;
typedef struct vertex vertex;

struct Pixel
{
	int x;
	int y;
};

struct array
{
	int len;
	int * options;
};

struct ptr_array
{
	struct array* data;
};

struct vertex
{
	int index;
	bool smooth;
	float* pos;
	float* normal;
	Pixel * pix;
	struct ptr_array * active_options;
};

void create_pixel_type(struct Pixel* p)
{
	MPI_Datatype types[2] = {MPI_INT, MPI_INT};
	int elementsPerType[2] = {1, 1};
	MPI_Aint rootAddr;
	MPI_Get_address(p, &rootAddr);
	MPI_Aint addr1;
	MPI_Get_address(&p->x, &addr1);
	MPI_Aint addr2;
	MPI_Get_address(&p->y, &addr2);
	MPI_Aint addresses[2] = {addr1 - rootAddr, addr2 - rootAddr};

	MPI_Type_create_struct(2, elementsPerType, addresses, types, &pixel_type);
	MPI_Type_commit(&pixel_type);
}

void create_ptr_array_type(struct ptr_array* ptr_tab)
{
	MPI_Datatype types[1] = {array_type};
	int elementsPerType[1] = {1};
	MPI_Aint rootAddr;
	MPI_Get_address(ptr_tab, &rootAddr);
	MPI_Aint addr1;
	MPI_Get_address(ptr_tab->data, &addr1);
	MPI_Aint addresses[1] = {addr1 - rootAddr};

	MPI_Type_create_struct(1, elementsPerType, addresses, types, &ptr_array_type);
	MPI_Type_commit(&ptr_array_type);
}

void create_array_type(struct array* tab)
{
	MPI_Datatype types[2] = {MPI_INT, MPI_INT};
	int elementsPerType[2] = {1, 5};
	MPI_Aint rootAddr;
	MPI_Get_address(tab, &rootAddr);
	MPI_Aint addr1;
	MPI_Get_address(&tab->len, &addr1);
	MPI_Aint addr2;
	MPI_Get_address(tab->options, &addr2);
	MPI_Aint addresses[2] = {addr1 - rootAddr, addr2 - rootAddr};

	MPI_Type_create_struct(2, elementsPerType, addresses, types, &array_type);
	MPI_Type_commit(&array_type);
}

void create_vertex_type(struct vertex* v)
{
	MPI_Datatype types[6] = {MPI_INT, MPI_C_BOOL, MPI_REAL, MPI_REAL, pixel_type, ptr_array_type};
	int elementsPerType[6] = {1, 1, 3, 3, 1, 3};
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
	MPI_Aint addr5;
	MPI_Get_address(v->pix, &addr5);
	MPI_Aint addr6;
	MPI_Get_address(v->active_options, &addr6);
	MPI_Aint addresses[6] = {addr1 - rootAddr, addr2 - rootAddr, addr3 - rootAddr, addr4 - rootAddr, addr5 - rootAddr, addr6 - rootAddr};

	MPI_Type_create_struct(6, elementsPerType, addresses, types, &vertex_type);
	MPI_Type_commit(&vertex_type);
}

void free_type()
{
	MPI_Type_free(&ptr_array_type);
	MPI_Type_free(&array_type);
	MPI_Type_free(&pixel_type);
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
	v.pix = malloc(sizeof(Pixel));

	v.active_options = malloc(3 * sizeof(struct ptr_array));
	for(int i = 0; i < 3; ++i)
	{
		v.active_options[i].data = malloc(sizeof(struct array));
		v.active_options[i].data->len = 5;
		v.active_options[i].data->options = malloc(5 * sizeof(int));
	}
	
	if(rank == 0)
	{
		v.index = 10;
		v.smooth = true;
		v.pos[0] = 1.0f, v.pos[1] = 2.0f; v.pos[2] = 3.0f;
		v.normal[0] = 0.0f, v.normal[1] = 1.0f; v.normal[2] = 0.0f;
		v.pix->x = 10;
		v.pix->y = 10;

		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < 5; ++j)
				v.active_options[i].data->options[j] = j * 2;
		}
	}

	struct ptr_array tmp;
	create_pixel_type(v.pix);
	create_array_type(v.active_options[0].data);
	create_ptr_array_type(&tmp);
	create_vertex_type(&v);

	if(rank == 0)
	{
		MPI_Send(&v, 1, vertex_type, 1, 0, MPI_COMM_WORLD);
	}
	else if(rank == 1)
	{
		printf("avant recv\n");
		MPI_Recv(&v, 1, vertex_type, 0, 0, MPI_COMM_WORLD, NULL);
		printf("après recv\n");
		char* vsmooth = (v.smooth) ? "true" : "false";
		printf("Processus %d reçoit le vertex : \nindex = %d\nsmooth = %s\n", rank, v.index, vsmooth);
		printf("pos = (%f,%f,%f)\nnormal = (%f,%f,%f)\n", v.pos[0], v.pos[1], v.pos[2], v.normal[0], v.normal[1], v.normal[2]);
		printf("pix = (%d,%d)\n", v.pix->x, v.pix->y);
		
		for(int i = 0; i < 3; ++i)
		{
			printf("option %d : [", i+1);
			for(int j = 0; j < 5; ++j)
			{
				if(j != 4)
					printf("%d, ", v.active_options[i].data->options[j]);
				else
					printf("%d]\n", v.active_options[i].data->options[j]);
			}
		}
	}

	free(v.pos);
	free(v.normal);
	free(v.pix);
	free_type();

	MPI_Finalize();
	return EXIT_SUCCESS;
}

# DEFINE COMPILER ==========
# ==========================
CC = gcc
ifdef mpi
	CC = mpicc
endif

# DEFINE COMPILE FLAGS ==========
# ===============================
CFLAGS = -Wall -Wextra -Wall
ifdef debug
	CFLAGS += -g
else
	CFLAGS += -O3
endif

ifdef omp
	CFLAGS += -fopenmp
endif

# FILES ==========
# ================
SRC = src/exact_cover.c
ifdef mpi
	SRC = src/exact_cover_mpi.c
else
	ifdef omp
		SRC = src/exact_cover_omp.c
	else
		ifdef final
			SRC = src/exact_cover_para.c
		endif
	endif
endif

# TARGETS ==========
# ==================

PROGRAM = exact_cover

$(PROGRAM): $(SRC)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm $(PROGRAM)

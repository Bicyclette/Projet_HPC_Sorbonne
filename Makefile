PROGRAM = exact_cover_seq

# DEFINE COMPILER ==========
# ==========================
CC = gcc
ifdef mpi
	CC = mpicc
else
	ifdef final
		CC = mpicc
	endif
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
else
	ifdef final
		CFLAGS += -fopenmp
	endif
endif

# FILES ==========
# ================
SRC = src/exact_cover.c
ifdef final
	SRC = src/exact_cover_para.c
else
	ifdef omp
		SRC = src/exact_cover_omp.c
	else
		ifdef mpi
			SRC = src/exact_cover_mpi.c
		endif
	endif
endif

# PROGRAM NAME ==========
# =======================
ifdef final
	PROGRAM = exact_cover_para
else
	ifdef omp
		PROGRAM = exact_cover_omp
	else
		ifdef mpi
			PROGRAM = exact_cover_mpi
		endif
	endif
endif

# TARGETS ==========
# ==================

$(PROGRAM): $(SRC)
	$(CC) -o $@ $^ $(CFLAGS)


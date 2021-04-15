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

# FILES ==========
# ================
SRC = src/exact_cover.c
ifdef mpi
	SRC = src/exact_cover_mpi.c
endif

OBJ = $(subst %.c,%.o,$(SRC))

# TARGETS ==========
# ==================

PROGRAM = src/exact_cover

$(PROGRAM): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm $(PROGRAM)

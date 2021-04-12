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
SRC = exact_cover.c
ifdef mpi
	SRC = exact_cover_mpi.c
endif

OBJ = $(subst %.c,%.o,$(SRC))

# TARGETS ==========
# ==================

PROGRAM = exact_cover

$(PROGRAM): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm $(PROGRAM)

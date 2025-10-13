# ==============================================================
#  Makefile for Project (C)
# ==============================================================

# Compiler
CC = gcc

# Compiler flags with warnings !!
# CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude

# Compiler flags without warnings
CFLAGS = -std=c11 -g -Iinclude

# Executable name
TARGET = search

# Directories
SRCDIR = src
OBJDIR = objectFiles
INCDIR = include

# Source files (add more 
SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/parseInput.c #\
#        $(SRCDIR)/dataset.c \
#        $(SRCDIR)/search_base.c \
#        $(SRCDIR)/lsh.c \
#        $(SRCDIR)/hypercube.c \
#        $(SRCDIR)/ivfflat.c \
#        $(SRCDIR)/ivfpq.c \
#        $(SRCDIR)/metrics.c \
#        $(SRCDIR)/utils.c

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# =====================================================================
# User Input Parameters

# CHOOSE WHAT ALGORITHM YOU WANT TO USE
# AND THEN CHANGE THE SECTIONS:
# -> Common Parameters
# -> The Corresponding Parameters section of the algorithm you chose

# =====================================================================

ALGO = ivfpq

# Common parameters
# -------------------

INPUT_FILE  = data/input.dat
QUERY_FILE  = data/query.dat

OUTPUT_FILE = results/output.txt
N           = 1
R           = 2000

TYPE        = mnist
RANGE       = false

# LSH defaults
# -------------------
K = 4
L = 5
W = 4.0

# Hypercube defaults
# -------------------
KPROJ  = 14
W      = 2
M      = 10
PROBES = 2

# IVFFLAT defaults
# -------------------
KCLUSTERS = 50
NPROBE    = 5
SEED      = 10

# IVFPQ defaults
# -------------------
KCLUSTERS = 50
NPROBE    = 5
M         = 10
NBITS     = 8
SEED      = 10


# ==============================================================
# Algorithm-specific parameters (auto-selected by ALGO)
# ==============================================================

ifeq ($(ALGO), lsh)
    ALGO_FLAG = -lsh
    ALGO_PARAMS = -k $(K) -L $(L) -w $(W) -o $(OUTPUT_FILE) -N $(N) -R $(R) -type $(TYPE) $(ALGO_FLAG) -range $(RANGE)
endif

ifeq ($(ALGO), hypercube)
    ALGO_FLAG = -hypercube
    ALGO_PARAMS = -kproj $(KPROJ) -w $(W) -M $(M) -probes $(PROBES) -o $(OUTPUT_FILE) -N $(N) -R $(R) -type $(TYPE) -range $(RANGE) $(ALGO_FLAG)
endif

ifeq ($(ALGO), ivfflat)
    ALGO_FLAG = -ivfflat
    ALGO_PARAMS = -kclusters $(KCLUSTERS) -nprobe $(NPROBE) -o $(OUTPUT_FILE) -N $(N) -R $(R) -type $(TYPE) -range $(RANGE) $(ALGO_FLAG) -seed $(SEED)
endif

ifeq ($(ALGO), ivfpq)
    ALGO_FLAG = -ivfpq
    ALGO_PARAMS = -kclusters $(KCLUSTERS) -nprobe $(NPROBE) -M $(M) -o $(OUTPUT_FILE) -N $(N) -R $(R) -type $(TYPE) -nbits $(NBITS) -range $(RANGE) $(ALGO_FLAG) -seed $(SEED)
endif


# ==============================================================
# Default rule: build and run
# ==============================================================

all: $(OBJDIR) $(TARGET)
	@echo "Running algorithm: $(ALGO)"
	./$(TARGET) \
		-d $(INPUT_FILE) \
		-q $(QUERY_FILE) \
		$(ALGO_PARAMS)
	@$(MAKE) clean


# ==============================================================
# Build executable from object files
# ==============================================================

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# ==============================================================
# Compile each .c into .o
# ==============================================================

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==============================================================
# Create object directory if missing
# ==============================================================

$(OBJDIR):
	@mkdir -p $(OBJDIR)

# ==============================================================
# Cleaning
# ==============================================================

clean:
	@echo "Cleaning up..."
	rm -f $(OBJDIR)/*.o $(TARGET)

# ==============================================================
# Phony targets
# ==============================================================

.PHONY: all clean

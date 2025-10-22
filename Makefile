# ==============================================================
#  Makefile for Project (C)
# ==============================================================

# Compiler
CC = gcc

# Compiler flags with warnings !!
# CFLAGS = -Wall -Wextra -std=c11 -g -Iinclude

# Compiler flags without warnings
CFLAGS = -std=c11 -g -Iinclude 
LDFLAGS = -lm

# Optional LSH optimization flags:
# Enable exact ID matching for faster queries (13% speedup, no recall loss with W=2000)
# CFLAGS += -DLSH_EXACT_ID_MATCH

# Executable name
TARGET = search

# Directories
SRCDIR = src
OBJDIR = objectFiles
INCDIR = include

# Source files
SRCS = $(wildcard $(SRCDIR)/*.c)

# Object files
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Pattern rule to compile any .c file in src/ to .o file in objectFiles/
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =====================================================================
# User Input Parameters

# CHOOSE WHAT ALGORITHM YOU WANT TO USE
# AND THEN CHANGE THE SECTIONS:
# -> Common Parameters
# -> The Corresponding Parameters section of the algorithm you chose

# =====================================================================

ALGO = hypercube

# DEBUGGING PURPOSES
#---------------------
# INPUT_FILE  = random_3d_points.txt
# QUERY_FILE  = query.dat
# TYPE = sift
# K = 6
# L = 10 
# W = 4

# Common parameters
# -------------------

INPUT_FILE  = Mnist_data/train-images.idx3-ubyte
QUERY_FILE  = Mnist_data/t10k-images-100-sample.idx3-ubyte

OUTPUT_FILE = output.txt
N           = 1
R           = 500

TYPE        = mnist
RANGE       = false

# LSH defaults
# -------------------
# For 2D data: Use K=3-4, L=15-20, W=5-6 for high recall
# For MNIST (784D, pixel values 0-255): Tuned high-recall baseline comes from large W due to E2LSH projections on raw ints
K = 4
L = 20
W = 2400

# Hypercube defaults
# -------------------
KPROJ       = 7
HYPERCUBE_W = 2000
M           = 200
PROBES      = 35

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
	ALGO_PARAMS = -kproj $(KPROJ) -w $(HYPERCUBE_W) -M $(M) -probes $(PROBES) -o $(OUTPUT_FILE) -N $(N) -R $(R) -type $(TYPE) -range $(RANGE) $(ALGO_FLAG)
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
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

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

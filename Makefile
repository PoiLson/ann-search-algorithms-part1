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

# Common parameters
# -------------------

INPUT_FILE  = random_2d_points.txt
QUERY_FILE  = query.dat

OUTPUT_FILE = output.txt
N           = 3
R           = 3

TYPE        = mnist
RANGE       = false

# LSH defaults
# -------------------
# For 2D data: Use K=3-4, L=15-20, W=5-6 for high recall
# For high-dim data: Use K=8-12, L=10-15, W=2-4
K = 3
L = 20
W = 6.0

# Hypercube defaults
# -------------------
KPROJ  = 3
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

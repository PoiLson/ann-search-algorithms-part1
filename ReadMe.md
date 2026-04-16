# Approximate Nearest Neighbor Search Algorithms Implementation

## **Authors:**  

- _Lytra Maria
- _Mylonaki Danai

## Main idea

In this project we implemented the following algorithms:

- **Locality sensitive Hashing using hash-tables**
    The way this algorithm works is by hashing each point $x$ with dimension $d$ to $L$ hash tables using amplified functions:

    $g_j(x)= \left(\sum_{i=1}^k r_{j,i}h_i(x) \mod M \right) \mod TableSize , \forall j \in [L]$

    where,
  
    $$h_i(x) = \lfloor (x \cdot v_i + t_i)/w \rfloor$$
  
    is the linear projection of the point,
    with:
    
    - **$v_i$** is a random Gaussian projection vector, drawn from $\mathcal{N}(0, 1)$ and normalized to unit length to              stabilize the projection scale.
    - **$t_i$** is a random offset sampled uniformly from [0, w), ensuring translation invariance of the hash bins.
    - **$r_j,i$** is a random integer coefficient chosen uniformly in [1, $R_range$), $R_range$ = $2^{29}$, used to form a linear             combination of the k hash values foreach table j.
    - **$M$** = $2^{32} - 5$ is a large prime modulus preventing overflow and promoting uniform bucket distribution.
       $2^{29}$ to avoid overflows.
      
    Each of the 𝐿 tables uses an independent set of 𝑘 projection vectors and linear coefficients.
  
    The table size (TableSize) was empirically set to 𝑁/4, where 𝑁 is the dataset size, to maintain a balanced load factor across         buckets.
  
    The parameters 𝐿,𝑘,𝑤,𝑁 (and optionally the search radius 𝑅) are user-defined and can be adjusted depending on the dataset and         desired recall–speed trade-off.
  
- **Locality sensitive Hashing Hypercube**
    This implementation is structurally fairly similar to the aforementioned algorithm keeping the $h_i$ functions to project the points while also utilizing a new function $f_i(h_i)$ to map their result to a single bit.
    This bit vector $[f_i(h_i(x))]$ is then converted into an integer that represents the hash value and corresponds to a single bucket in a hash-table.

    In our code, these $f_i$ use a precomputed threshold to determine the bit value.
    This threshold is computed as the average value of $h_i$ for all points, making the mapping balanced.

- **InVerted File indexing**
    this algorithm deviates largely from the previous approaches focusing on clustering data points to group them.
    We use Kmeans++ to initialize our centroids from our dataset and the Lloyd algorithm to compute the final ones.
    To mitigate the risk of non-convergence, the algorithm stops after a predetermined maximum number of iterations, which has been determined to be 50 following numerous trials.
    As convergence we define the state where no centroid moved more than $10^{-4}$

- **InVerted File indexing with Product Quantization**
    similarly to IVFflat we group the dataset points using clustering and then we encode each point with a vector of lower dimension and size as follows:
      - We compute the remainder vectors $r(x) = x - cluster$, then break this in $M$ subspaces, such that $d$ is divisible by $M$
      - We sample each subspace to train $2^{nbist}$ new centroids and assign all r_i(x) to them. The corresponding centroid becomes the encoding of the point resulting to a vector $[centroid_i(x)], \forall i \in [M]$
      - During quering we compute the LUT values when need, since most values won't be need and precomputing them adds unnecessery overhead

## Project Structure

This project is formatted in the following way:  

- _Include_: contains all necessary header files
- _src_: contains all .c files
- _objectFiles_: contains all .o files created during compiling  
- _Data_: contains folders with data as follows:
  - .cache: files created for logging brutforce results to avoid recomputing on each run
  - MNIST: mnist train and query sets can be saved here
  - SIFT: sift base and query sets can be saved here
- _Makefile_

## Dataset Structure

For more details about the dataset structure, see [datasets.md](datasets.md).

### **Providing a more in-depth analysis of every .c and header packet we encounter:**

**Files included in the src/ folder:**

The src/ directory contains the implementation (.c) files corresponding to the declared functions in the header files.
Each source file implements the functionality defined in its matching header, with full documentation and structured modular design.
A brief description of each file is provided below:

- **_bruteforce_cache.c:_** runs brute force K-NN algorithm for the train and query sets given and saves rerults and average time to a file.
- **_datasets.c:_** different functions to proccess each dataset (MNIST/SIFT)
- **_hashtable.c:_** hash-table structure with fixed capicity handling collision by chaining dynamic arrays to each bucket.
- **_hypercube.c:_** Contains functions specific to this algorithm:
  - _hash_func_impl_hyper_: computes the binary hash function by projecting points and mapping each projection to a bit using precomputed thresholds
  - _hyper_init_(build process): initializes a Hypercube struct, computes average thresholds for each projection to preserve locality, and stores data in a single hash-table with buckets corresponding to binary vectors
  - _hyper_index_lookup_: performs A-NN for a query by probing multiple buckets in Hamming distance order up to the specified probes limit
  - _range_search_hyper_: returns points inside a given range from the query
  - _hyper_destroy_: frees all memory occupied by the Hypercube struct
- **_ivfflat.c:_** Contains functions specific to this algorithm:
  - _assign_points_to_clusters_: assigns dataset points to their nearest cluster centroids using parallel processing
  - _recompute_centroids_: updates cluster centroids by averaging assigned points and checks for convergence
  - _ivfflat_init_(build process): initializes an IVFFlatIndex struct using KMeans++ initialization and Lloyd's algorithm with parallel assignment to cluster data points
  - _ivfflat_index_lookup_: performs A-NN for a query by searching only the nprobe nearest clusters to the query vector
  - _range_search_ivfflat_: returns points inside a given range from the query within the searched clusters
  - _ivfflat_destroy_: frees all memory occupied by the IVFFlatIndex struct
- **_ivfpq.c:_** Contains functions specific to this algorithm:
  - _run_lloyd_on_subspace_: trains product quantization codebooks for each subspace using KMeans++ initialization and Lloyd's algorithm with parallel assignment
  - _compute_residual_: computes remainder vectors $r(x) = x - c(x)$ where $c(x)$ is the cluster centroid
  - _ivfpq_init_(build process): initializes an IVFPQIndex struct, clusters data using IVFFlat approach, splits residuals into M subspaces, and trains $2^{nbits}$ centroids per subspace to encode points with compact codes
  - _ivfpq_index_lookup_: performs A-NN for a query by computing asymmetric distances using lookup tables (LUT) for distance estimation between query and compressed codes in the nprobe nearest clusters
  - _range_search_ivfpq_: returns points inside a given range from the query within the searched clusters
  - _ivfpq_destroy_: frees all memory occupied by the IVFPQIndex struct including PQ codebooks
- **_lsh.c:_** Contains functions specific to this algorithm:
  - _hash_func_impl_lsh_: computed the amplified hash function $g(x)$
  - _lsh_init_(build proccess): initializes an lsh struct to group all necessary values, sets values to all parameters and stores the data in the hash-tables
  - _lsh_index_lookup_: performs A-NN for a query using the lsh hash-tables
  - _range_search_lsh_: returns points inside a given range from the query
  - _lsh_destroy_: frees all memory occupied by the lsh struct
- **_main.c:_** Main function of the programm. After all arguments are parsed and checked it saves the dataset in a struct and passes it in the corresponding algorithm. Before exiting it frees the dataset.
- **_minheap.c:_** Contains all functions to implement a basic min-heap ADT.
        Helper function to keep top-N candidates
- **_parseinput.c:_** Defines structures related to the input parameters, parsed the command line arguments and performs besic checks
- **_query.c:_** The perform_query function runs bruteforce_cache if there is no Cache file, and for each query vector runs and times a lookup_function specified by the algorithm.
        If range is true it also runs range_search.
        Lastly, it computes all required metrics and saves the results in the output file.
- **_runAlgorithms.c:_** Contains functions to initialize each algorithm and start the quering proccess
- **_silhouette.c:_** Contains a function to compute the silhouette for each cluster. Its main purpose is to evaluate the number of clusters
- **_utils.c:_** Contains all functions shared by all algorithms such as functions to compute dot product, euclidean distance, L2 norm, hamming distance etc.

**Files included in the include/ folder:**

The include/ directory contains all header files necessary for the implementation.
Below is a brief description of each:

- **_bruteforce_cache.h:_** Declarations for the brute-force K-NN baseline and caching utilities.
- **_datasets.h:_** Dataset loading, parsing, and preprocessing utilities for MNIST and SIFT.
- **_hashtable.h:_** Hash table structure and functions used by LSH and Hypercube.
- **_hypercube.h:_** Declarations for Hypercube-specific structures and lookup functions.
- **_ivfflat.h:_** Functions and structures for IVFFlat index creation and querying.
- **_ivfpq.h:_** Functions and structures for IVFPQ encoding, codebook training, and search.
- **_lsh.h:_** Core definitions and data structures for the LSH algorithm.
- **_main.h:_** Contains the declarations for all the needed C and User-Defined libraries.
- **_minheap.h:_** Declarations for the min-heap data structure used during nearest-neighbor search.
- **_parseinput.h:_**  Command-line argument parsing and validation routines.
- **_query.h:_** Query execution, timing, and metric computation utilities.
- **_runAlgorithms.h:_** Functions for initializing and running all implemented algorithms.
- **_silhouette.h:_** Functions for cluster evaluation and silhouette computation.
- **_utils.h:_** Common mathematical operations shared among all algorithms.

## Compilation Instructions

The project can be compiled directly using the provided Makefile.  
To build the executable manually, run:

```code
make
```

This will create an executable named search inside the root directory.  
The Makefile uses the GCC compiler with the following key flags:

- **_std=c11:_** enforces C11 standard compliance
- **_fopenmp:_** enables OpenMP parallelization
- **_Iinclude:_** includes all header files from the include/ directory
- **_lm:_** links the math library

The compiled object files are stored in the objectFiles/ directory.  
To clean all generated files (object files and executables), run:  

```code
make clean
```

## Usage Instructions

Once compiled, the program can be executed as follows:

```code
./search -d {dataset file} -q {query file} -o {output file} -{algorithm_flag} -type {dataset_name} [additional_parameters]
```

### **Files**

- **_Dataset file →_** Includes the data of the wanted dataset (MNIST or SIFT)
- **_Query file →_** Includes the query data of the wanted dataset (MNIST or SIFT)
- **_Output file →_** File to extract the output with the execution's metrics

### **Available Algorithm Flags**

- **_lsh →_** Locality Sensitive Hashing (LSH)
- **_hypercube →_** Binary Hypercube
- **_ivfflat →_** Inverted File Index (IVFFlat)
- **_ivfpq →_** Inverted File Index with Product Quantization (IVFPQ)

### **Available Datasets**

- **_mnist →_** for the MNIST dataset
- **_sift →_** for the SIFT dataset

### **Additional Parameters**

Guides for the additional parameters are given inside the makefile, you can change the 
value of the parameters in the corresponding line based on the algorithm you choose
and the given dataset you want to use.

### **Default Execution**

The Makefile already contains pre-tuned (optimized) parameter sets for each algorithm–dataset combination.
However, these presets are meant for our best configurations and not for a “default” run.
If the user wants to run the program with default settings, they should use one of the following commands directly:

**Locality Sensitive Hashing (LSH)**

```code
./search -lsh -type mnist -d {dataset file} -q {query file} -o {output file}
```

**Binary Hypercube**

```code
./search -hypercube -type mnist -d {dataset file} -q {query file} -o {output file}
```

**Inverted File Index (IVFFlat)**

```code
OMP_NUM_THREADS=8 OMP_NESTED=TRUE OMP_MAX_ACTIVE_LEVELS=2 ./search -ivfflat -type mnist -d {dataset file} -q {query file} -o {output file}
```

**Inverted File Index with Product Quantization (IVFPQ)**

```code
OMP_NUM_THREADS=8 OMP_NESTED=TRUE OMP_MAX_ACTIVE_LEVELS=2 ./search -ivfpq -type mnist -d {dataset file} -q {query file} -o {output file}
```

### **Optimized Configurations Execution**
If the user wishes to run the optimized configurations (as used in our experiments),
they can do so through the Makefile preset.

For the MNIST dataset:

```code
make mnist ALGO=lsh
make mnist ALGO=hypercube
make mnist ALGO=ivfflat
make mnist ALGO=ivfpq
```

The same applies for the SIFT dataset:

```code
make sift ALGO=lsh
make sift ALGO=hypercube
make sift ALGO=ivfflat
make sift ALGO=ivfpq
```

### **Parameters Across Algorithms:**

The following parameters can be modified directly in the Makefile. Each parameter is preceded by a “-” in the Makefile, and you can change its value there.

- **General:_** -d `<dataset file>` , -q `<query file>`, -o `<output file>`, -N `<nearest neighbors>`, -R `<radius>`, -type `<mnist|sift>`, -range `<true|false>`, -seed `<int>`.
- **_LSH:_** -k `<int/>`, -L `<int>`, -w `<double>`, -lsh.
- **_Hypercube:_** -kproj `<int>`, -w `<double>`, -M `<int>`, -probes `<int>`, -hypercube.
- **_IVFFlat:_** -kclusters `<int>`, -nprobe `<int>`, -ivfflat.
- **_IVFPQ:_** -kclusters `<int>`, -nprobe `<int>`, -M `<int>`, -nbits `<int>`, -ivfpq.

## OpenMP Usage

Throughout this project OpenMp was used in various places to sorten build times in the preprocessing stage without affecting time dependent results i.e. anything related to query search.

## Valgrind

We put our programm through extensive Valgrind checks and it is leak free with the exeption of a few bytes allocated by OpenMP that show up by Valgrind as still reachable and OpenMP handles them itself.

## Version Control and Collaboration

The development of this project was managed using the Git version control system.

All source files, headers, and experimental scripts were tracked through a dedicated Git repository to ensure collaborative development, change tracking, and reproducibility of results. The repository was hosted on a private GitHub project for version tracking and collaboration.


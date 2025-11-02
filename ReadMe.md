# Approximate Nearest Neighbor Search Algorithms Implementation

## **Authors:**  

- _Lytra Maria - sdi2200089_
- _Mylonaki Danai - sdi2200114_

## Main idea

In this project we implemented the following algorithms:

- **Locality sensitive Hashing using hash-tables**
    The way this algorithm works is by hashing each point $x$ with dimension $d$ to $L$ hash tables using amplified functions:

    $g_j(x)= \left(\sum_{i=1}^k h_i(x) \mod M \right) \mod TableSize , \forall j \in [L]$

    where $h_i(x) = \lfloor (x \cdot v_i + t_i)/w \rfloor$ is the linear projection of the point,
    with $v_i$ a normalized gaussian vector of dimension $d$ so that we can use a smaller window size while keeping the data intact
    and $t_i$ a random offset bounded between $0$ and $2^{29}$ to avoid overflows.

    The parameters $L, k, w$ are chosen by the user.
    $M = 2^{32} - 5$ prime
    Tablesize was chosen, after careful consideration, to be $N/4$, where $N$ is the dataset size.

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

## Project Sturcture

This project is formatted in the following way:  

- _Include_: contains all necessary header files
- _src_: contains all .c files
- _objectFiles_: contains all .o files created during compiling  
- _Data_: contains folders with data as follows:
  - .cache: files created for logging brutforce results to avoid recomputing on each run
  - MNIST: mnist train and query sets can be saved here
  - SIFT: sift base and query sets can be saved here
- _Makefile_

**Providing a more in-depth analysis of every .c and header packet we encounter:**

Throughtout this project OpenMp was used in various places to sorten build times without affecting time dependent results i.e. anything related to query search

- **_bruteforce_cache:_** runs brute force K-NN algorithm for the train and query sets given and saves rerults and average time to a file.
- **_datasets:_** different functions to proccess each dataset (MNIST/SIFT)
- **_hashtable:_** hash-table structure with fixed capicity handling collision by chaining dynamic arrays to each bucket.
- **_hypercube:_** Contains functions specific to this algorithm:
  - _hash_func_impl_hyper_: computes the binary hash function by projecting points and mapping each projection to a bit using precomputed thresholds
  - _hyper_init_(build process): initializes a Hypercube struct, computes average thresholds for each projection to preserve locality, and stores data in a single hash-table with buckets corresponding to binary vectors
  - _hyper_index_lookup_: performs A-NN for a query by probing multiple buckets in Hamming distance order up to the specified probes limit
  - _range_search_hyper_: returns points inside a given range from the query
  - _hyper_destroy_: frees all memory occupied by the Hypercube struct
- **_ivfflat:_** Contains functions specific to this algorithm:
  - _assign_points_to_clusters_: assigns dataset points to their nearest cluster centroids using parallel processing
  - _recompute_centroids_: updates cluster centroids by averaging assigned points and checks for convergence
  - _ivfflat_init_(build process): initializes an IVFFlatIndex struct using KMeans++ initialization and Lloyd's algorithm with parallel assignment to cluster data points
  - _ivfflat_index_lookup_: performs A-NN for a query by searching only the nprobe nearest clusters to the query vector
  - _range_search_ivfflat_: returns points inside a given range from the query within the searched clusters
  - _ivfflat_destroy_: frees all memory occupied by the IVFFlatIndex struct
- **_ivfpq:_** Contains functions specific to this algorithm:
  - _run_lloyd_on_subspace_: trains product quantization codebooks for each subspace using KMeans++ initialization and Lloyd's algorithm with parallel assignment
  - _compute_residual_: computes remainder vectors $r(x) = x - c(x)$ where $c(x)$ is the cluster centroid
  - _ivfpq_init_(build process): initializes an IVFPQIndex struct, clusters data using IVFFlat approach, splits residuals into M subspaces, and trains $2^{nbits}$ centroids per subspace to encode points with compact codes
  - _ivfpq_index_lookup_: performs A-NN for a query by computing asymmetric distances using lookup tables (LUT) for distance estimation between query and compressed codes in the nprobe nearest clusters
  - _range_search_ivfpq_: returns points inside a given range from the query within the searched clusters
  - _ivfpq_destroy_: frees all memory occupied by the IVFPQIndex struct including PQ codebooks
- **_lsh:_** Contains functions specific to this algorithm:
  - _hash_func_impl_lsh_: computed the amplified hash function $g(x)$
  - _lsh_init_(build proccess): initializes an lsh struct to group all necessary values, sets values to all parameters and stores the data in the hash-tables
  - _lsh_index_lookup_: performs A-NN for a query using the lsh hash-tables
  - _range_search_lsh_: returns points inside a given range from the query
  - _lsh_destroy_: frees all memory occupied by the lsh struct
- **_main:_** Main function of the programm. After all arguments are parsed and checked it saves the dataset in a struct and passes it in the corresponding algorithm. Before exiting it frees the dataset.
- **_minheap:_** Contains all functions to implement a basic min-heap ADT.
        Helper function to keep top-N candidates
- **_parseinput:_** Defines structures related to the input parameters, parsed the command line arguments and performs besic checks
- **_query:_** The perform_query function runs bruteforce_cache if there is no Cache file, and for each query vector runs and times a lookup_function specified by the algorithm.
        If range is true it also runs range_search.
        Lastly, it computes all required metrics and saves the results in the output file.
- **_runAlgorithms:_** Contains functions to initialize each algorithm and start the quering proccess
- **_silhouette:_** Contains a function to compute the silhouette for each cluster. Its main purpose is to evaluate the number of clusters
- **_utils:_** Contains all functions shared by all algorithms such as functions to compute dot product, euclidean distance, L2 norm, hamming distance etc.

## Valgrind

We put our programm through extensive valgrind checks and it is leak free with the exeption of a few bytes allocated by OpenMP that show up by valgrind as still reachable and openMP handles them itself.

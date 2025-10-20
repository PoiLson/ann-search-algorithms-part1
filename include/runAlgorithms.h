#ifndef RUNALGORITHMS_H
#define RUNALGORITHMS_H

struct SearchParams;

// function declarations for running different algorithms
void run_lsh(SearchParams* params, Dataset* dataset);
void run_hypercube(SearchParams* params, Dataset* dataset);
void run_ivfflat(SearchParams* params, Dataset* dataset);
void run_ivfpq(SearchParams* params, Dataset* dataset);

#endif
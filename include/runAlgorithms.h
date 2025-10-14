#ifndef RUNALGORITHMS_H
#define RUNALGORITHMS_H

struct SearchParams;

void run_lsh(SearchParams* params, Dataset* dataset);
void run_hypercube(SearchParams* params);
void run_ivfflat(SearchParams* params);
void run_ivfpq(SearchParams* params);

#endif
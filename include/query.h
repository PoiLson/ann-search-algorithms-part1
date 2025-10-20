#ifndef QUERY_H
#define QUERY_H

// Function pointer type for index lookup functions
typedef void (*index_lookup)(const void*, const struct SearchParams*, int*, double*, int*, int**, int*, void*);

// Performs queries on the query set using the provided index lookup function
void perform_query(const struct SearchParams* params, const struct Dataset* dataset, const struct Dataset* query_set, index_lookup lookup_func, void* index_data);

#endif
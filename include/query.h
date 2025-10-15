#ifndef QUERY_H
#define QUERY_H

typedef void (*index_lookup)(const void*, const struct SearchParams*, int*, double*, int*, int**, int*, void*);

void perform_query(const struct SearchParams* params, const struct Dataset* dataset, const struct Dataset* query_set, index_lookup lookup_func, void* index_data);

#endif
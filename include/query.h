#ifndef QUERY_H
#define QUERY_H

void perform_query(const struct SearchParams* params, const struct Dataset* dataset, const struct Dataset* query_set, void (*index_lookup)(const void*, const struct SearchParams*, int*, double*, int*, int**, int*, void*), void* index_data);

#endif
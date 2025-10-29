#ifndef SILHOUETTE_H
#define SILHOUETTE_H

int compute_silhouette_parallel(const IVFFlatIndex *index, const Dataset *dataset, double *per_cluster);

#endif
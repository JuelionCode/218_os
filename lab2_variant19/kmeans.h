#ifndef KMEANS_H
#define KMEANS_H

#include <stddef.h>

typedef struct {
    float x;
    float y;
} Point;

typedef struct {
    float x;
    float y;
} Centroid;

void kmeans_serial(const Point *points, size_t n_points, size_t k,
                   size_t max_iters, Centroid *centroids, int *assignments);

void kmeans_parallel(const Point *points, size_t n_points, size_t k,
                     size_t max_iters, size_t n_threads,
                     Centroid *centroids, int *assignments);

#endif

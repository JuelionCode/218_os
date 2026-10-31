#include "kmeans.h"
#include <stdlib.h>
#include <float.h>

void kmeans_serial(const Point *points, size_t n_points, size_t k,
                   size_t max_iters, Centroid *centroids, int *assignments) {
    double *sum_x = (double *)calloc(k, sizeof(double));
    double *sum_y = (double *)calloc(k, sizeof(double));
    size_t *counts = (size_t *)calloc(k, sizeof(size_t));

    for (size_t iter = 0; iter < max_iters; ++iter) {
        for (size_t c = 0; c < k; ++c) {
            sum_x[c] = 0.0;
            sum_y[c] = 0.0;
            counts[c] = 0;
        }

        for (size_t i = 0; i < n_points; ++i) {
            float min_dist_sq = FLT_MAX;
            int best_cluster = 0;

            for (size_t c = 0; c < k; ++c) {
                float dx = points[i].x - centroids[c].x;
                float dy = points[i].y - centroids[c].y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    best_cluster = (int)c;
                }
            }

            assignments[i] = best_cluster;
            sum_x[best_cluster] += points[i].x;
            sum_y[best_cluster] += points[i].y;
            counts[best_cluster]++;
        }

        for (size_t c = 0; c < k; ++c) {
            if (counts[c] > 0) {
                centroids[c].x = (float)(sum_x[c] / counts[c]);
                centroids[c].y = (float)(sum_y[c] / counts[c]);
            }
        }
    }

    free(sum_x);
    free(sum_y);
    free(counts);
}

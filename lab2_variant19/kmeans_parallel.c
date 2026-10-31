#include "kmeans.h"
#include <pthread.h>
#include <stdlib.h>
#include <float.h>

typedef struct {
    size_t thread_id;
    size_t n_threads;
    const Point *points;
    size_t n_points;
    Centroid *centroids;
    size_t k;
    size_t max_iters;
    int *assignments;
    double *all_sums_x;
    double *all_sums_y;
    size_t *all_counts;
    pthread_barrier_t *barrier;
} ThreadData;

static void *kmeans_thread_func(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    size_t id = data->thread_id;
    size_t n_threads = data->n_threads;
    size_t n_points = data->n_points;
    size_t k = data->k;
    size_t max_iters = data->max_iters;

    double *my_sums_x = &data->all_sums_x[id * k];
    double *my_sums_y = &data->all_sums_y[id * k];
    size_t *my_counts = &data->all_counts[id * k];

    size_t chunk = (n_points + n_threads - 1) / n_threads;
    size_t start = id * chunk;
    size_t end = start + chunk;
    if (start > n_points) start = n_points;
    if (end > n_points) end = n_points;

    for (size_t iter = 0; iter < max_iters; ++iter) {
        for (size_t c = 0; c < k; ++c) {
            my_sums_x[c] = 0.0;
            my_sums_y[c] = 0.0;
            my_counts[c] = 0;
        }

        for (size_t i = start; i < end; ++i) {
            float min_dist_sq = FLT_MAX;
            int best_cluster = 0;

            for (size_t c = 0; c < k; ++c) {
                float dx = data->points[i].x - data->centroids[c].x;
                float dy = data->points[i].y - data->centroids[c].y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq < min_dist_sq) {
                    min_dist_sq = dist_sq;
                    best_cluster = (int)c;
                }
            }

            data->assignments[i] = best_cluster;
            my_sums_x[best_cluster] += data->points[i].x;
            my_sums_y[best_cluster] += data->points[i].y;
            my_counts[best_cluster]++;
        }

        pthread_barrier_wait(data->barrier);

        if (id == 0) {
            for (size_t c = 0; c < k; ++c) {
                double total_sum_x = 0.0;
                double total_sum_y = 0.0;
                size_t total_count = 0;

                for (size_t t = 0; t < n_threads; ++t) {
                    total_sum_x += data->all_sums_x[t * k + c];
                    total_sum_y += data->all_sums_y[t * k + c];
                    total_count += data->all_counts[t * k + c];
                }

                if (total_count > 0) {
                    data->centroids[c].x = (float)(total_sum_x / total_count);
                    data->centroids[c].y = (float)(total_sum_y / total_count);
                }
            }
        }

        pthread_barrier_wait(data->barrier);
    }

    return NULL;
}

void kmeans_parallel(const Point *points, size_t n_points, size_t k,
                     size_t max_iters, size_t n_threads,
                     Centroid *centroids, int *assignments) {
    if (n_threads < 1) n_threads = 1;

    pthread_t *threads = (pthread_t *)malloc(n_threads * sizeof(pthread_t));
    ThreadData *thread_data = (ThreadData *)malloc(n_threads * sizeof(ThreadData));
    double *all_sums_x = (double *)calloc(n_threads * k, sizeof(double));
    double *all_sums_y = (double *)calloc(n_threads * k, sizeof(double));
    size_t *all_counts = (size_t *)calloc(n_threads * k, sizeof(size_t));

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, (unsigned int)n_threads);

    for (size_t t = 0; t < n_threads; ++t) {
        thread_data[t].thread_id = t;
        thread_data[t].n_threads = n_threads;
        thread_data[t].points = points;
        thread_data[t].n_points = n_points;
        thread_data[t].centroids = centroids;
        thread_data[t].k = k;
        thread_data[t].max_iters = max_iters;
        thread_data[t].assignments = assignments;
        thread_data[t].all_sums_x = all_sums_x;
        thread_data[t].all_sums_y = all_sums_y;
        thread_data[t].all_counts = all_counts;
        thread_data[t].barrier = &barrier;

        pthread_create(&threads[t], NULL, kmeans_thread_func, &thread_data[t]);
    }

    for (size_t t = 0; t < n_threads; ++t) {
        pthread_join(threads[t], NULL);
    }

    pthread_barrier_destroy(&barrier);
    free(all_sums_x);
    free(all_sums_y);
    free(all_counts);
    free(thread_data);
    free(threads);
}

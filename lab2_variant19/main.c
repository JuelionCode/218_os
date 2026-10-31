#include "kmeans.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

static double get_time_ms(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) * 1000.0 +
           (double)(end.tv_nsec - start.tv_nsec) * 1e-6;
}

static void generate_synthetic_data(Point *points, size_t n_points, size_t k) {
    srand(42);
    for (size_t i = 0; i < n_points; ++i) {
        size_t cluster = i % k;
        float center_x = (float)(cluster * 50.0);
        float center_y = (float)((cluster % 3) * 50.0);
        float offset_x = ((float)(rand() % 10000) / 100.0f) - 50.0f;
        float offset_y = ((float)(rand() % 10000) / 100.0f) - 50.0f;
        points[i].x = center_x + offset_x;
        points[i].y = center_y + offset_y;
    }
}

static void *demo_worker(void *arg) {
    (void)arg;
    struct timespec ts = {0, 10000000L};
    while (1) {
        nanosleep(&ts, NULL);
    }
}

static void run_demo_mode(size_t n_threads) {
    printf("PID текущего процесса: %d\n", getpid());
    printf("Создание %zu рабочих потоков...\n", n_threads);

    pthread_t *threads = (pthread_t *)malloc(n_threads * sizeof(pthread_t));
    for (size_t i = 0; i < n_threads; ++i) {
        pthread_create(&threads[i], NULL, demo_worker, NULL);
    }

    printf("Потоки успешно созданы и активны!\n");
    printf("Для просмотра стандартными средствами ОС выполните команду:\n");
    printf("    ps -T -p %d\n", getpid());
    printf("или в реальном времени:\n");
    printf("    top -H -p %d\n", getpid());

    if (isatty(STDIN_FILENO)) {
        printf("\nНажмите Enter для завершения демонстрации...");
        getchar();
    } else {
        sleep(2);
    }

    for (size_t i = 0; i < n_threads; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    free(threads);
    printf("Демонстрация завершена, потоки корректно освобождены.\n");
}

static void print_usage(const char *prog) {
    printf("Использование: %s [опции]\n", prog);
    printf("Опции:\n");
    printf("  -t <потоки>    Количество потоков (по умолчанию: 4)\n");
    printf("  -k <кластеры>  Количество кластеров (по умолчанию: 32)\n");
    printf("  -n <точки>     Количество точек данных (по умолчанию: 2000000)\n");
    printf("  -i <итерации>  Количество итераций k-средних (по умолчанию: 15)\n");
    printf("  -b, --bench    Запустить автоматический бенчмарк для отчёта\n");
    printf("  -d, --demo     Режим демонстрации потоков\n");
    printf("  -h, --help     Показать эту справку\n");
}

int main(int argc, char **argv) {
    size_t n_threads = 4;
    size_t k = 32;
    size_t n_points = 2000000;
    size_t max_iters = 15;
    int run_bench = 0;
    int run_demo = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            n_threads = (size_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            k = (size_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            n_points = (size_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            max_iters = (size_t)atoi(argv[++i]);
        } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bench") == 0) {
            run_bench = 1;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--demo") == 0) {
            run_demo = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (k < 1) k = 1;
    if (n_threads < 1) n_threads = 1;

    if (run_demo) {
        run_demo_mode(n_threads);
        return 0;
    }

    printf(" Точек: %zu, Кластеров: %zu, Итераций: %zu, Потоков по ключу: %zu\n",
           n_points, k, max_iters, n_threads);

    Point *points = (Point *)malloc(n_points * sizeof(Point));
    generate_synthetic_data(points, n_points, k);

    Centroid *initial_centroids = (Centroid *)malloc(k * sizeof(Centroid));
    for (size_t c = 0; c < k; ++c) {
        initial_centroids[c] = (Centroid){ points[c].x, points[c].y };
    }

    Centroid *centroids_s = (Centroid *)malloc(k * sizeof(Centroid));
    int *assign_s = (int *)malloc(n_points * sizeof(int));
    memcpy(centroids_s, initial_centroids, k * sizeof(Centroid));

    struct timespec start, end;

    if (!run_bench) {
        printf("Запуск последовательной версии (1 поток)...\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
        kmeans_serial(points, n_points, k, max_iters, centroids_s, assign_s);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double t_s = get_time_ms(start, end);
        printf("Последовательное время Ts: %.2f мс\n\n", t_s);

        Centroid *centroids_p = (Centroid *)malloc(k * sizeof(Centroid));
        int *assign_p = (int *)malloc(n_points * sizeof(int));
        memcpy(centroids_p, initial_centroids, k * sizeof(Centroid));

        printf("Запуск параллельной версии (потоков: %zu)...\n", n_threads);
        clock_gettime(CLOCK_MONOTONIC, &start);
        kmeans_parallel(points, n_points, k, max_iters, n_threads, centroids_p, assign_p);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double t_p = get_time_ms(start, end);

        double speedup = t_s / t_p;
        double efficiency = (speedup / (double)n_threads) * 100.0;

        printf(" Результаты (потоков: %zu):\n", n_threads);
        printf(" Время параллельной реализации Tp: %.2f мс\n", t_p);
        printf(" Ускорение S = Ts / Tp:            %.2f\n", speedup);
        printf(" Эффективность E = (S / p) * 100%%: %.2f%%\n", efficiency);

        float max_diff = 0.0f;
        for (size_t c = 0; c < k; ++c) {
            float dx = fabsf(centroids_s[c].x - centroids_p[c].x);
            float dy = fabsf(centroids_s[c].y - centroids_p[c].y);
            if (dx > max_diff) max_diff = dx;
            if (dy > max_diff) max_diff = dy;
        }
        if (max_diff < 1e-2f) {
            printf("[OK] Результаты последовательной и параллельной версий полностью совпали!\n");
        } else {
            printf("[INFO] Максимальное расхождение центроидов: %f\n", max_diff);
        }

        free(centroids_p);
        free(assign_p);
    } else {
        printf("Запуск последовательной версии (1 поток)...\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
        kmeans_serial(points, n_points, k, max_iters, centroids_s, assign_s);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double t_s = get_time_ms(start, end);

        long logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (logical_cores < 1) logical_cores = 12;

        size_t experiments[] = {1, 2, 4, 8, (size_t)logical_cores, 16, 64, 128, 1024};
        size_t num_exp = sizeof(experiments) / sizeof(experiments[0]);

        printf("Логических ядер процессора в системе: %ld\n", logical_cores);
        printf("| Число потоков | Время исполнения (мс) | Ускорение | Эффективность |\n");
        printf("| 1 (посл.)     | %17.2f     |      1.00 |       100.00%% |\n", t_s);

        Centroid *centroids_p = (Centroid *)malloc(k * sizeof(Centroid));
        int *assign_p = (int *)malloc(n_points * sizeof(int));

        for (size_t i = 0; i < num_exp; ++i) {
            size_t p = experiments[i];
            memcpy(centroids_p, initial_centroids, k * sizeof(Centroid));

            clock_gettime(CLOCK_MONOTONIC, &start);
            kmeans_parallel(points, n_points, k, max_iters, p, centroids_p, assign_p);
            clock_gettime(CLOCK_MONOTONIC, &end);
            double t_p = get_time_ms(start, end);

            double s = t_s / t_p;
            double e = (s / (double)p) * 100.0;

            printf("| %13zu | %21.2f | %9.2f | %12.2f%% |\n", p, t_p, s, e);
        }

        free(centroids_p);
        free(assign_p);
    }

    free(assign_s);
    free(centroids_s);
    free(initial_centroids);
    free(points);

    return 0;
}

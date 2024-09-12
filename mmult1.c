#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <papi.h>
#define CLOCK_REALTIME 0
#define CLOCK_PROCESS_CPUTIME_ID 2
#define CLOCK_THREAD_CPUTIME_ID 3
#define NUM_ITERATIONS 3
#define NUM_PAPI_EVENTS 3
#define CACHE_SIZE 1024 * 1024 * 16

void mmult_ijk(double *A, double *B, double *C, int N);
void mmult_ikj(double *A, double *B, double *C, int N);
void mmult_jik(double *A, double *B, double *C, int N);
void mmult_jki(double *A, double *B, double *C, int N);
void mmult_kji(double *A, double *B, double *C, int N);
void mmult_kij(double *A, double *B, double *C, int N);

void flush_cache()
{
    char *cache = malloc(CACHE_SIZE);
    if (cache == NULL)
    {
        fprintf(stderr, "memory allocation failed");
        exit(1);
    }

    for (size_t i = 0; i < CACHE_SIZE; i++)
    {
        cache[i] = i % 10;
    }

    // free
    free((void *)cache);
}

// IJK
void mmult_ijk(double *A, double *B, double *C, int N)
{
    int i, j, k;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            C[i * N + j] = 0.0;
            for (k = 0; k < N; k++)
            {
                C[i * N + j] += A[i * N + k] * B[k * N + j];
            }
        }
    }
}

// IKJ
void mmult_ikj(double *A, double *B, double *C, int N)
{
    int i, j, k;
    for (i = 0; i < N; i++)
    {
        for (k = 0; k < N; k++)
        {
            for (j = 0; j < N; j++)
            {
                C[i * N + j] += A[i * N + k] * B[k * N + j];
            }
        }
    }
}

// JIK
void mmult_jik(double *A, double *B, double *C, int N)
{
    int i, j, k;
    for (j = 0; j < N; j++)
    {
        for (i = 0; i < N; i++)
        {
            for (k = 0; k < N; k++)
            {
                C[i * N + j] += A[i * N + k] * B[k * N + j];
            }
        }
    }
}

// JKI
void mmult_jki(double *A, double *B, double *C, int N)
{
    int i, j, k;
    for (j = 0; j < N; j++)
    {
        for (k = 0; k < N; k++)
        {
            for (i = 0; i < N; i++)
            {
                C[i * N + j] += A[i * N + k] * B[k * N + j];
            }
        }
    }
}

// KIJ
void mmult_kij(double *A, double *B, double *C, int N)
{
    int i, j, k;
    for (k = 0; k < N; k++)
    {
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                C[i * N + j] += A[i * N + k] * B[k * N + j];
            }
        }
    }
}

// KJI
void mmult_kji(double *A, double *B, double *C, int N)
{
    int i, j, k;
    for (k = 0; k < N; k++)
    {
        for (j = 0; j < N; j++)
        {
            for (i = 0; i < N; i++)
            {
                C[i * N + j] += A[i * N + k] * B[k * N + j];
            }
        }
    }
}

int main()
{
    int arr_sizes[] = {50, 100, 200, 400, 800, 1200, 1600, 2000};
    for (int ijkvariation = 0; ijkvariation < 6; ijkvariation++)
    {

        for (int arri = 0; arri < 8; arri++) // TBD for testing, use arri < 3
        {
            int N = arr_sizes[arri];

            long long int counters_all[NUM_PAPI_EVENTS];

            for (int i = 0; i < NUM_PAPI_EVENTS; i++)
            {
                counters_all[i] = 0;
            }

            unsigned long long int realTime = 0;
            unsigned long long int threadTime = 0;
            unsigned long long int processTime = 0;
            for (int iteration = 0; iteration < NUM_ITERATIONS; iteration++)
            {

                double *A, *B, *C;
                int i, j;
                long long counters[NUM_PAPI_EVENTS];

                // counters[0] = # of L1 cache misses
                // counters[1] = # of L1 cache total accesses

                // counters[0] = # of L2 cache misses
                // counters[1] = # of L2 cache total accesses
                // counters[2] = # of FP instructions
                // counters[3] = # loads

                // counters[2] = # stores

                int PAPI_events[] = {
                    PAPI_L1_DCM,
                    PAPI_L1_DCA,
                    // PAPI_L2_DCM,
                    // PAPI_L2_DCA,
                    // PAPI_FP_INS,
                    // PAPI_LD_INS,
                    PAPI_SR_INS
                };

                flush_cache();

                // create and initialize matrices
                A = (double *)malloc(N * N * sizeof(double));
                B = (double *)malloc(N * N * sizeof(double));
                C = (double *)malloc(N * N * sizeof(double));

                for (i = 0; i < N * N; i++)
                {
                    A[i] = ((double)rand() / RAND_MAX) * 9;
                    B[i] = ((double)rand() / RAND_MAX) * 9;
                }
                printf("A[0][0]: %f\n", A[0]);
                printf("B[0][0]: %f\n", B[0]);

                // start PAPI
                if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
                {
                    fprintf(stderr, "PAPI version isn't the most updated version\n");
                    exit(1);
                }
                int retval = PAPI_start_counters(PAPI_events, NUM_PAPI_EVENTS);
                if (retval != PAPI_OK)
                {
                    fprintf(stderr, "PAPI error: %s\n", PAPI_strerror(retval));
                    exit(1);
                }

                // start CLOCK_PROCESS_CPUTIME_ID (0)
                struct timespec tick0, tock0;
                uint64_t execTime0;
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tick0);

                // start CLOCK_REALTIME (1)
                struct timespec tick1, tock1;
                uint64_t execTime1;
                clock_gettime(CLOCK_REALTIME, &tick1);

                // start CLOCK_THREAD_CPUTIME_ID (2)
                struct timespec tick2, tock2;
                uint64_t execTime2;
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tick2);

                // 6 mmult operations
                if (ijkvariation == 0)
                {
                    mmult_ijk(A, B, C, N);
                }
                if (ijkvariation == 1)
                {
                    mmult_ikj(A, B, C, N);
                }
                if (ijkvariation == 2)
                {
                    mmult_jik(A, B, C, N);
                }
                if (ijkvariation == 3)
                {
                    mmult_jki(A, B, C, N);
                }
                if (ijkvariation == 4)
                {
                    mmult_kji(A, B, C, N);
                }
                if (ijkvariation == 5)
                {
                    mmult_kij(A, B, C, N);
                }

                // print values to prevent optimization
                printf("A[0][0]: %f\n", A[0]);
                printf("B[0][0]: %f\n", B[0]);

                // stop PAPI
                if (PAPI_stop_counters(counters, NUM_PAPI_EVENTS) != PAPI_OK)
                {
                    fprintf(stderr, "Error stopping PAPI counters!\n");
                    exit(1);
                }

                // stop CLOCK_PROCESS_CPUTIME_ID
                clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tock0);
                execTime0 = 1000000000 * (tock0.tv_sec - tick0.tv_sec) + tock0.tv_nsec - tick0.tv_nsec;

                // stop CLOCK_REALTIME
                clock_gettime(CLOCK_REALTIME, &tock1);
                execTime1 = 1000000000 * (tock1.tv_sec - tick1.tv_sec) + tock1.tv_nsec - tick1.tv_nsec;

                // stop CLOCK_THREAD_CPUTIME_ID
                clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tock2);
                execTime2 = 1000000000 * (tock2.tv_sec - tick2.tv_sec) + tock2.tv_nsec - tick2.tv_nsec;

                // update results for this iteration
                for (int i = 0; i < NUM_PAPI_EVENTS; i++)
                {
                    counters_all[i] = counters_all[i] + counters[i];
                }
                realTime += execTime1;
                processTime += execTime0;
                threadTime += execTime2;

                // free memory for 3 matrices
                free(A);
                free(B);
                free(C);
            }
            // printf("Results for method ")
            // printf("Elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);
            // printf("Total # of cycles: %lld\n", counters_all[0]);
            if (ijkvariation == 0)
            {
                printf("\n\n----------ijk size = %d x %d--------------\n", N, N);
            }
            if (ijkvariation == 1)
            {
                printf("\n\n----------ikj size = %d x %d--------------\n", N, N);
            }
            if (ijkvariation == 2)
            {
                printf("\n\n----------jik size = %d x %d--------------\n", N, N);
            }
            if (ijkvariation == 3)
            {
                printf("\n\n----------jki size = %d x %d--------------\n", N, N);
            }
            if (ijkvariation == 4)
            {
                printf("\n\n----------kji size = %d x %d--------------\n", N, N);
            }
            if (ijkvariation == 5)
            {
                printf("\n\n----------kij size = %d x %d--------------\n", N, N);
            }
            // printf("Elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int)execTime);

            // counters[0] = TOTAL CYCLES
            // counters[1] = # of L1 cache misses
            // counters[2] = # of L1 cache total accesses
            // counters[3] = # of L2 cache misses
            // counters[4] = # of L2 cache total accesses
            // counters[5] = # of FP instructions
            // counters[6] = # loads
            // counters[7] = # stores

            // int PAPI_events[] = {
            //     PAPI_TOT_CYC,
            //     PAPI_L1_DCM,
            //     PAPI_L1_DCA,
            //     PAPI_L2_DCM,
            //     PAPI_L2_DCA,
            //     PAPI_FP_INS,
            //     PAPI_LD_INS,
            //     PAPI_SR_INS
            // };

            printf("REALTIME: %llu\n", realTime / NUM_ITERATIONS);
            printf("PROCESS_CPUTIME: %llu\n", processTime / NUM_ITERATIONS);
            printf("THREAD_CPUTIME: %llu\n", threadTime / NUM_ITERATIONS);
            // printf("# of FP instrs: %'lld\n", (long long)counters_all[4] / NUM_ITERATIONS);
            // printf("# of loads: %'lld\n", (long long)counters_all[5] / NUM_ITERATIONS);
            printf("# of stores: %'lld\n", (long long)counters_all[2] / NUM_ITERATIONS);
            printf("L1 data cache miss rate: %.3lf%%\n", (double)counters_all[0] / (double)counters_all[1] * 100);
            // printf("L2 data cache miss rate: %.3lf%%\n", (double)counters_all[2] / (double)counters_all[3] * 100);
        }
    }

    return 0;
}

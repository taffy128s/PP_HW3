#include <mpi.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

const int MAX_V = 2000;
const int MAX_W = 500000;
const int NUM_THREADS = 12;

int rank, size;
int v_num, e_num, num_process, num_row, last_num_row;
int graph[MAX_V][MAX_V];
pthread_barrier_t barrier1, barrier2;

void *multi_thr_alg(void *ptr) {
    int id = *(int*)ptr;
    delete (int*)ptr;
    int end_row = (rank == size - 1) ? v_num : num_row * (rank + 1);
    for (int k = 0; k < v_num; k++) {
        for (int i = num_row * rank + id; i < end_row; i += NUM_THREADS) {
            for (int j = 0; j < v_num; j++) {
                int sum = graph[i][k] + graph[k][j];
                if (sum < graph[i][j])
                    graph[i][j] = sum;
            }
        }
        pthread_barrier_wait(&barrier1);
        pthread_barrier_wait(&barrier2);
    }
    pthread_exit(NULL);
}

void init_graph() {
    for (int i = 0; i < v_num; i++) {
        for (int j = 0; j < v_num; j++) {
            graph[i][j] = MAX_W;
        }
    }
    for (int i = 0; i < v_num; i++) {
        graph[i][i] = 0;
    }
}

void show_graph() {
    for (int i = 0; i < v_num; i++) {
        for (int j = 0; j < v_num; j++) {
            printf("%7d ", graph[i][j]);
        }
        puts("");
    }
    puts("");
}

void write_graph(FILE *wfh) {
    for (int i = 0; i < v_num; i++) {
        for (int j = 0; j < v_num; j++) {
            fprintf(wfh, "%d ", graph[i][j]);
        }
        fprintf(wfh, "\n");
    }
}

int getNodeCount()
{
    int rank, is_rank0, nodes;
    MPI_Comm shmcomm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &shmcomm);
    MPI_Comm_rank(shmcomm, &rank);
    is_rank0 = (rank == 0) ? 1 : 0;
    MPI_Allreduce(&is_rank0, &nodes, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    MPI_Comm_free(&shmcomm);
    return nodes;
}

int main(int argc, char** argv) {
    assert(argc == 4);
    MPI_Init(&argc, &argv);
    FILE *rfh, *wfh;
    rfh = fopen(argv[1], "r");
    wfh = fopen(argv[2], "w");
    num_process = atoi(argv[3]);
    
    fscanf(rfh, "%d %d", &v_num, &e_num);
    init_graph();
    for (int i = 0; i < e_num; i++) {
        int l, r, w;
        fscanf(rfh, "%d %d %d", &l, &r, &w);
        graph[l][r] = w;
        graph[r][l] = w;
    }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int node_num = getNodeCount();
    
    if (size > node_num)
        size = node_num;
    
    if (rank < size) {
        num_row = v_num / size;
        last_num_row = num_row + v_num % size;
        
        pthread_barrier_init(&barrier1, NULL, NUM_THREADS + 1);
        pthread_barrier_init(&barrier2, NULL, NUM_THREADS + 1);
        pthread_t threads[NUM_THREADS];
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_create(&threads[i], NULL, multi_thr_alg, new int(i));
            pthread_detach(threads[i]);
        }
        for (int k = 0; k < v_num; k++) {
            pthread_barrier_wait(&barrier1);
            for (int i = 0; i < size; i++) {
                if (i == rank)
                    continue;
                MPI_Request req;
                int end_row = (rank == size - 1) ? v_num : num_row * (rank + 1);
                for (int j = num_row * rank; j < end_row; j++) {
                    MPI_Isend(graph[j], v_num, MPI_INT, i, 0, MPI_COMM_WORLD, &req);
                }
            }
            for (int i = 0; i < size; i++) {
                if (i == rank)
                    continue;
                MPI_Status stat;
                int end_row = (i == size - 1) ? v_num : num_row * (i + 1);
                for (int j = num_row * i; j < end_row; j++) {
                    MPI_Recv(graph[j], v_num, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
                }
            }
            pthread_barrier_wait(&barrier2);
        }
        
        if (rank == 0) {
            write_graph(wfh);
            fclose(wfh);
            fclose(rfh);
        }
    }
    
    MPI_Finalize();
}

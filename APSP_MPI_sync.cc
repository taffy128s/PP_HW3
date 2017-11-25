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

int v_num, e_num, num_process;
int graph[MAX_V][MAX_V];
std::vector<int> to_comm;

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
            printf("%10d ", graph[i][j]);
        }
        puts("");
    }
    puts("");
}

void get_to_comm(int rank) {
    for (int i = 0; i < v_num; i++) {
        if (graph[rank][i] != 0 && graph[rank][i] != MAX_W)
            to_comm.push_back(i);
    }
}

void write_graph(FILE *wfh) {
    for (int i = 0; i < v_num; i++) {
        for (int j = 0; j < v_num; j++) {
            fprintf(wfh, "%d ", graph[i][j]);
        }
        fprintf(wfh, "\n");
    }
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
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    get_to_comm(rank);
    
    int *to_send[to_comm.size()], *to_recv[to_comm.size()], temp[v_num];
    for (int i = 0; i < to_comm.size(); i++) {
        to_send[i] = new int[v_num];
        to_recv[i] = new int[v_num];
    }
    for (int i = 0; i < v_num; i++) {
        temp[i] = graph[rank][i];
    }
    
    MPI_Request req;
    MPI_Status stat;
    int cont, cont_all = 1;
    while (cont_all) {
        cont = 0;
        for (int i = 0; i < to_comm.size(); i++) {
            for (int j = 0; j < v_num; j++) {
                to_send[i][j] = temp[j] + graph[rank][to_comm[i]];
            }
            MPI_Isend(to_send[i], v_num, MPI_INT, to_comm[i], 0, MPI_COMM_WORLD, &req);
        }
        for (int i = 0; i < to_comm.size(); i++) {
            MPI_Recv(to_recv[i], v_num, MPI_INT, to_comm[i], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
            for (int j = 0; j < v_num; j++) {
                if (to_recv[i][j] < temp[j]) {
                    temp[j] = to_recv[i][j];
                    cont = 1;
                }
            }
        }
        MPI_Allreduce(&cont, &cont_all, 1, MPI_INT, MPI_BOR, MPI_COMM_WORLD);
    }
    
    if (rank == 0) {
        for (int i = 0; i < v_num; i++) {
            graph[0][i] = temp[i];
        }
        for (int i = 1; i < size; i++) {
            MPI_Recv(graph[i], v_num, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        }
        write_graph(wfh);
        fclose(wfh);
        fclose(rfh);
    } else {
        MPI_Isend(temp, v_num, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
    }
    
    MPI_Finalize();
}

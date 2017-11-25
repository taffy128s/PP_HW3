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
const int term_tag = 2;
const int data_tag = 1;
const int ring_tag = 0;
const int white = 0;
const int black = 1;

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
    
    int *to_send[to_comm.size()], to_recv[v_num], temp[v_num];
    for (int i = 0; i < to_comm.size(); i++) {
        to_send[i] = new int[v_num];
    }
    for (int i = 0; i < v_num; i++) {
        temp[i] = graph[rank][i];
    }
    
    MPI_Request send_req, data_req, ring_req, term_req;
    MPI_Status stat;
    int need_to_send = 1, data_flag = 1, ring_flag = 1, ring;
    int color = white, term_flag = 1, term;
    while (1) {
        if (need_to_send) {
            for (int i = 0; i < to_comm.size(); i++) {
                for (int j = 0; j < v_num; j++) {
                    to_send[i][j] = temp[j] + graph[rank][to_comm[i]];
                }
                MPI_Isend(to_send[i], v_num, MPI_INT, to_comm[i], data_tag, MPI_COMM_WORLD, &send_req);
                if (to_comm[i] < rank)
                    color = black;
            }
        }
        need_to_send = 0;
        if (data_flag != 0) {
            MPI_Irecv(to_recv, v_num, MPI_INT, MPI_ANY_SOURCE, data_tag, MPI_COMM_WORLD, &data_req);
            data_flag = 0;
        }
        MPI_Test(&data_req, &data_flag, &stat);
        while (data_flag != 0) {
            for (int i = 0; i < v_num; i++) {
                if (to_recv[i] < temp[i]) {
                    temp[i] = to_recv[i];
                    need_to_send = 1;
                }
            }
            MPI_Irecv(to_recv, v_num, MPI_INT, MPI_ANY_SOURCE, data_tag, MPI_COMM_WORLD, &data_req);
            MPI_Test(&data_req, &data_flag, &stat);
        }
        if (rank == 0) {
            if (ring_flag != 0) {
                ring = white;
                MPI_Isend(&ring, 1, MPI_INT, 1, ring_tag, MPI_COMM_WORLD, &send_req);
                MPI_Irecv(&ring, 1, MPI_INT, size - 1, ring_tag, MPI_COMM_WORLD, &ring_req);
                ring_flag = 0;
            }
            MPI_Test(&ring_req, &ring_flag, &stat);
            if (ring_flag != 0) {
                if (ring == white) {
                    for (int i = 1; i < size; i++) {
                        MPI_Isend(&term, 1, MPI_INT, i, term_tag, MPI_COMM_WORLD, &send_req);
                    }
                    break;
                }
            }
        } else {
            if (ring_flag != 0) {
                MPI_Irecv(&ring, 1, MPI_INT, rank - 1, ring_tag, MPI_COMM_WORLD, &ring_req);
                ring_flag = 0;
            }
            MPI_Test(&ring_req, &ring_flag, &stat);
            if (ring_flag != 0) {
                if (color == black) {
                    ring = black;
                    MPI_Isend(&ring, 1, MPI_INT, (rank + 1) % size, ring_tag, MPI_COMM_WORLD, &send_req);
                    color = white;
                } else {
                    MPI_Isend(&ring, 1, MPI_INT, (rank + 1) % size, ring_tag, MPI_COMM_WORLD, &send_req);
                }
            }
            if (term_flag != 0) {
                MPI_Irecv(&term, 1, MPI_INT, 0, term_tag, MPI_COMM_WORLD, &term_req);
                term_flag = 0;
            }
            MPI_Test(&term_req, &term_flag, &stat);
            if (term_flag != 0) {
                break;
            }
        }
        /*
        if (need_to_send == 1)
            printf("rank: %d, need_to_send: %d\n", rank, need_to_send);
        */
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
        MPI_Isend(temp, v_num, MPI_INT, 0, 0, MPI_COMM_WORLD, &send_req);
    }
    
    MPI_Finalize();
}

#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const int MAX_V = 2000;
const int MAX_W = 500000;

int v_num, e_num, num_thr;
int graph[MAX_V][MAX_V];
pthread_barrier_t barrier;

void *multi_thr_alg(void *ptr) {
    int id = *(int*)ptr;
    delete (int*)ptr;
    for (int k = 0; k < v_num; k++) {
        for (int i = id; i < v_num; i += num_thr) {
            for (int j = 0; j < v_num; j++) {
                int sum = graph[i][k] + graph[k][j];
                    if (sum < graph[i][j])
                        graph[i][j] = sum;
            }
        }
        pthread_barrier_wait(&barrier);
    }
    pthread_exit(NULL);
}

void single_thr_alg() {
    for (int k = 0; k < v_num; k++) {
        for (int i = 0; i < v_num; i++) {
            for (int j = 0; j < v_num; j++) {
                int sum = graph[i][k] + graph[k][j];
                if (sum < graph[i][j])
                    graph[i][j] = sum;
            }
        }
    }
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
            printf("%10d ", graph[i][j]);
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

int main(int argc, char** argv) {
    assert(argc == 4);
    FILE *rfh, *wfh;
    rfh = fopen(argv[1], "r");
    wfh = fopen(argv[2], "w");
    num_thr = atoi(argv[3]);
    
    fscanf(rfh, "%d %d", &v_num, &e_num);
    init_graph();
    for (int i = 0; i < e_num; i++) {
        int l, r, w;
        fscanf(rfh, "%d %d %d", &l, &r, &w);
        graph[l][r] = w;
        graph[r][l] = w;
    }
    
    /*
    show_graph();
    single_thr_alg();
    show_graph();
    write_graph(wfh);
    */
    
    pthread_barrier_init(&barrier, NULL, num_thr);
    pthread_t threads[num_thr];
    for (int i = 0; i < num_thr; i++) {
        pthread_create(&threads[i], NULL, multi_thr_alg, new int(i));
    }
    for (int i = 0; i < num_thr; i++) {
        pthread_join(threads[i], NULL);
    }
    write_graph(wfh);
    fclose(rfh);
    fclose(wfh);
    pthread_exit(NULL);
}

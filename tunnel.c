#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRAJETS 10

sem_t mutex;            
sem_t sem_XY;           
sem_t sem_YX;           

int sens = 0;           
int count_XY = 0;       
int count_YX = 0;       
int attente_XY = 0;
int attente_YX = 0;

void wait_random() {
    usleep((rand() % 500 + 1000) * 1000); 
}

void entrer_tunnel(int direction) {
    sem_wait(&mutex);

    if (direction == 1) {
        if (sens == 2 || (sens == 1 && attente_YX > 0)) {
            attente_XY++;
            sem_post(&mutex);
            sem_wait(&sem_XY); 
            sem_wait(&mutex);
            attente_XY--;
        }
        sens = 1;
        count_XY++;
    } else { 
        if (sens == 1 || (sens == 2 && attente_XY > 0)) {
            attente_YX++;
            sem_post(&mutex);
            sem_wait(&sem_YX); 
            sem_wait(&mutex);
            attente_YX--;
        }
        sens = 2;
        count_YX++;
    }

    sem_post(&mutex);
}

void sortir_tunnel(int direction) {
    sem_wait(&mutex);

    if (direction == 1) {
        count_XY--;
        if (count_XY == 0) {
            if (attente_YX > 0) {
                sens = 2;
                for (int i = 0; i < attente_YX; i++)
                    sem_post(&sem_YX);
            } else {
                sens = 0;
            }
        }
    } else {
        count_YX--;
        if (count_YX == 0) {
            if (attente_XY > 0) {
                sens = 1;
                for (int i = 0; i < attente_XY; i++)
                    sem_post(&sem_XY);
            } else {
                sens = 0;
            }
        }
    }

    sem_post(&mutex);
}

typedef struct {
    int id;
    int ville; // 0 = X, 1 = Y
} BusArgs;

void* bus_thread(void* arg) {
    BusArgs* args = (BusArgs*)arg;
    int id = args->id;
    int ville = args->ville;
    free(arg);

    for (int i = 1; i <= NB_TRAJETS; i++) {
        int direction = (ville == 0) ? 1 : 2;
        printf("Bus %d de %s : %s -> %s (Trajet %d)\n", id, ville == 0 ? "X" : "Y",
               ville == 0 ? "X" : "Y", ville == 0 ? "Y" : "X", i);
        entrer_tunnel(direction);
        wait_random();
        sortir_tunnel(direction);

        direction = (direction == 1) ? 2 : 1;
        printf("Bus %d de %s : %s -> %s (Trajet %d)\n", id, ville == 0 ? "X" : "Y",
               ville == 0 ? "Y" : "X", ville == 0 ? "X" : "Y", i);
        entrer_tunnel(direction);
        wait_random();
        sortir_tunnel(direction);
    }

    return NULL;
}

int main() {
    srand(time(NULL));
    sem_init(&mutex, 0, 1);
    sem_init(&sem_XY, 0, 0);
    sem_init(&sem_YX, 0, 0);

    pthread_t threads[NB_BUS_X + NB_BUS_Y];

    for (int i = 0; i < NB_BUS_X; i++) {
        BusArgs* args = malloc(sizeof(BusArgs));
        args->id = i;
        args->ville = 0;
        pthread_create(&threads[i], NULL, bus_thread, args);
    }

    for (int i = 0; i < NB_BUS_Y; i++) {
        BusArgs* args = malloc(sizeof(BusArgs));
        args->id = i;
        args->ville = 1;
        pthread_create(&threads[NB_BUS_X + i], NULL, bus_thread, args);
    }

    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&sem_XY);
    sem_destroy(&sem_YX);

    return 0;
}

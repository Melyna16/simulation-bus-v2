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
int sens = 0; // 0 : libre, 1 : X->Y, 2 : Y->X
int count_XY = 0;
int count_YX = 0;
int attente_XY = 0;
int attente_YX = 0;

void wait_random() {
    usleep((rand() % 500 + 1000) * 1000); // entre 1 et 1.5 secondes
}

void entrer_tunnel(int direction) {
    sem_wait(&mutex);

    if (direction == 1) { // X -> Y
        attente_XY++;
        while (sens == 2) { // Si l'autre sens est en cours
            sem_post(&mutex);
            usleep(100000); // attend 0.1s
            sem_wait(&mutex);
        }
        attente_XY--;
        count_XY++;
        sens = 1;
    } else { // Y -> X
        attente_YX++;
        while (sens == 1) {
            sem_post(&mutex);
            usleep(100000);
            sem_wait(&mutex);
        }
        attente_YX--;
        count_YX++;
        sens = 2;
    }

    sem_post(&mutex);
}

void sortir_tunnel(int direction) {
    sem_wait(&mutex);
    if (direction == 1) {
        count_XY--;
        if (count_XY == 0) {
            // Le tunnel est vide, priorité au côté qui attend
            if (attente_YX > 0) {
                sens = 2;
            } else {
                sens = 0;
            }
        }
    } else {
        count_YX--;
        if (count_YX == 0) {
            if (attente_XY > 0) {
                sens = 1;
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
        // Aller
        int direction = (ville == 0) ? 1 : 2;
        printf("Bus %d de %s : %s -> %s (Trajet %d)\n", id, ville == 0 ? "X" : "Y",
               ville == 0 ? "X" : "Y", ville == 0 ? "Y" : "X", i);
        entrer_tunnel(direction);
        wait_random();
        sortir_tunnel(direction);

        // Retour
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

    pthread_t threads[NB_BUS_X + NB_BUS_Y];

    // Créer les bus de X
    for (int i = 0; i < NB_BUS_X; i++) {
        BusArgs* args = malloc(sizeof(BusArgs));
        args->id = i;
        args->ville = 0; // ville X
        pthread_create(&threads[i], NULL, bus_thread, args);
    }

    // Créer les bus de Y
    for (int i = 0; i < NB_BUS_Y; i++) {
        BusArgs* args = malloc(sizeof(BusArgs));
        args->id = i;
        args->ville = 1; // ville Y
        pthread_create(&threads[NB_BUS_X + i], NULL, bus_thread, args);
    }

    // Attendre tous les threads
    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex);
    return 0;
}


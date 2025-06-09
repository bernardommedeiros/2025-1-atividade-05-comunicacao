#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>

#define SHM_SIZE 1024  // Tamanho da memória compartilhada

typedef struct {
    char *shm;
    int *shmid;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int *data_ready;
} thread_data;

// Thread escritora
void *writer_thread(void *arg) {
    thread_data *data = (thread_data *)arg;
    
    pthread_mutex_lock(data->mutex);
    
    // Escreve na memória compartilhada
    strcpy(data->shm, "Olá, Memória Compartilhada com Threads!");
    *data->data_ready = 1;
    
    printf("Escritor: Mensagem escrita na memória compartilhada.\n");
    printf("Escritor: Aguardando leitura...\n");
    
    // Sinaliza que os dados estão prontos
    pthread_cond_signal(data->cond);
    
    // Espera confirmação do leitor
    while (*data->data_ready == 1) {
        pthread_cond_wait(data->cond, data->mutex);
    }
    
    pthread_mutex_unlock(data->mutex);
    
    printf("Escritor: Leitura confirmada. Finalizando.\n");
    return NULL;
}

// Thread leitora
void *reader_thread(void *arg) {
    thread_data *data = (thread_data *)arg;
    
    pthread_mutex_lock(data->mutex);
    
    // Espera dados estarem prontos
    while (*data->data_ready == 0) {
        pthread_cond_wait(data->cond, data->mutex);
    }
    
    printf("Leitor: Mensagem lida: %s\n", data->shm);
    
    // Sinaliza que terminou a leitura
    *data->data_ready = 0;
    pthread_cond_signal(data->cond);
    
    pthread_mutex_unlock(data->mutex);
    
    printf("Leitor: Finalizado.\n");
    return NULL;
}

int main() {
    int shmid;
    key_t key = 1234;
    char *shm;
    pthread_t writer, reader;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int data_ready = 0;

    // Cria segmento de memória compartilhada
    shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    // Anexa o segmento
    shm = shmat(shmid, NULL, 0);
    if (shm == (char *)-1) {
        perror("shmat");
        exit(1);
    }

    // Prepara dados para as threads
    thread_data data = {
        .shm = shm,
        .shmid = &shmid,
        .mutex = &mutex,
        .cond = &cond,
        .data_ready = &data_ready
    };

    // Cria threads
    pthread_create(&writer, NULL, writer_thread, &data);
    pthread_create(&reader, NULL, reader_thread, &data);

    // Espera threads terminarem
    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    // Libera recursos
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>

#define SHM_SIZE 1024
#define KEY 1234

typedef struct {
    char* shm_ptr;
    int shm_id;
    pthread_mutex_t lock;
    pthread_cond_t data_ready;
    pthread_cond_t data_processed;
    int is_ready;
    int is_processed;
} shared_data_t;

void* writer_thread(void* arg) {
    shared_data_t* data = (shared_data_t*)arg;
    
    pthread_mutex_lock(&data->lock);
    
    // Write to shared memory
    strcpy(data->shm_ptr, "Olá, Memória Compartilhada com Threads!");
    data->is_ready = 1;
    
    printf("[WRITER] Mensagem escrita na memória compartilhada.\n");
    printf("[WRITER] Aguardando leitura...\n");
    
    // Signal reader that data is ready
    pthread_cond_signal(&data->data_ready);
    
    // Wait for reader to process
    while (!data->is_processed) {
        pthread_cond_wait(&data->data_processed, &data->lock);
    }
    
    pthread_mutex_unlock(&data->lock);
    
    printf("[WRITER] Confirmação recebida. Finalizando.\n");
    return NULL;
}

void* reader_thread(void* arg) {
    shared_data_t* data = (shared_data_t*)arg;
    
    pthread_mutex_lock(&data->lock);
    
    // Wait for data to be ready
    while (!data->is_ready) {
        pthread_cond_wait(&data->data_ready, &data->lock);
    }
    
    printf("[READER] Mensagem lida: %s\n", data->shm_ptr);
    
    // Signal writer that we're done
    data->is_processed = 1;
    pthread_cond_signal(&data->data_processed);
    
    pthread_mutex_unlock(&data->lock);
    
    printf("[READER] Finalizado.\n");
    return NULL;
}

int main() {
    int shmid;
    char* shm;
    pthread_t writer, reader;
    shared_data_t shared_data;
    
    // Initialize synchronization primitives
    pthread_mutex_init(&shared_data.lock, NULL);
    pthread_cond_init(&shared_data.data_ready, NULL);
    pthread_cond_init(&shared_data.data_processed, NULL);
    shared_data.is_ready = 0;
    shared_data.is_processed = 0;
    
    // Create shared memory segment
    if ((shmid = shmget(KEY, SHM_SIZE, IPC_CREAT | 0666)) == -1) {
        perror("shmget");
        exit(1);
    }
    
    // Attach shared memory
    if ((shm = shmat(shmid, NULL, 0)) == (char*)-1) {
        perror("shmat");
        exit(1);
    }
    
    shared_data.shm_ptr = shm;
    shared_data.shm_id = shmid;
    
    // Create threads
    pthread_create(&writer, NULL, writer_thread, &shared_data);
    pthread_create(&reader, NULL, reader_thread, &shared_data);
    
    // Wait for threads to finish
    pthread_join(writer, NULL);
    pthread_join(reader, NULL);
    
    // Clean up
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    pthread_mutex_destroy(&shared_data.lock);
    pthread_cond_destroy(&shared_data.data_ready);
    pthread_cond_destroy(&shared_data.data_processed);
    
    return 0;
}
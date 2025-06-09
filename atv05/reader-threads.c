#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define FILENAME "comunicacao.txt"
#define TEMPNAME "comunicacao.lida"

// Estrutura para comunicação entre threads
typedef struct {
    char buffer[1024];
    int file_ready;
    int file_processed;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} thread_comm;

// Função da thread leitora
void* reader_thread(void* arg) {
    thread_comm* comm = (thread_comm*)arg;
    FILE* file;

    pthread_mutex_lock(&comm->mutex);

    // Espera até que o arquivo esteja pronto
    while (!comm->file_ready) {
        pthread_cond_wait(&comm->cond, &comm->mutex);
    }

    // Abre o arquivo para leitura
    file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&comm->mutex);
        return NULL;
    }

    // Lê o conteúdo do arquivo
    size_t bytes_read = fread(comm->buffer, 1, sizeof(comm->buffer), file);
    fclose(file);
    comm->buffer[bytes_read] = '\0';

    printf("Leitor: Mensagem lida:\n%s", comm->buffer);

    // Renomeia o arquivo (sinaliza que terminou)
    if (rename(FILENAME, TEMPNAME) != 0) {
        perror("rename");
        pthread_mutex_unlock(&comm->mutex);
        return NULL;
    }

    printf("Leitor: Arquivo renomeado para %s\n", TEMPNAME);

    // Sinaliza que terminou o processamento
    comm->file_processed = 1;
    pthread_cond_signal(&comm->cond);
    pthread_mutex_unlock(&comm->mutex);

    return NULL;
}

// Função da thread principal (simulando o escritor)
void* writer_thread(void* arg) {
    thread_comm* comm = (thread_comm*)arg;
    FILE* file;
    const char* message = "Mensagem de teste para comunicação entre threads!\n";

    pthread_mutex_lock(&comm->mutex);

    // Cria/sobrescreve o arquivo
    file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(&comm->mutex);
        return NULL;
    }

    fwrite(message, 1, strlen(message), file);
    fclose(file);

    printf("Escritor: Mensagem escrita no arquivo\n");

    // Sinaliza que o arquivo está pronto
    comm->file_ready = 1;
    pthread_cond_signal(&comm->cond);

    // Espera até que o leitor termine
    while (!comm->file_processed) {
        pthread_cond_wait(&comm->cond, &comm->mutex);
    }

    pthread_mutex_unlock(&comm->mutex);
    printf("Escritor: Leitura concluída. Finalizando.\n");

    return NULL;
}

int main() {
    pthread_t reader, writer;
    thread_comm comm = {
        .file_ready = 0,
        .file_processed = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
        .cond = PTHREAD_COND_INITIALIZER
    };

    // Cria as threads
    if (pthread_create(&writer, NULL, writer_thread, &comm)) {
        perror("pthread_create writer");
        exit(1);
    }

    if (pthread_create(&reader, NULL, reader_thread, &comm)) {
        perror("pthread_create reader");
        exit(1);
    }

    // Espera as threads terminarem
    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    // Limpa os recursos
    pthread_mutex_destroy(&comm.mutex);
    pthread_cond_destroy(&comm.cond);

    return 0;
}
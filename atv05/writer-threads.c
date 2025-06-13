#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define FILENAME "comunicacao.txt"

// Estrutura para passar dados para as threads
typedef struct {
    const char *message;
    int *file_processed;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} thread_data;

// Função da thread escritora
void *writer_thread(void *arg) {
    thread_data *data = (thread_data *)arg;
    FILE *file;

    // Bloqueia o mutex antes de acessar os recursos compartilhados
    pthread_mutex_lock(data->mutex);

    // Abre o arquivo para escrita
    file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(data->mutex);
        return NULL;
    }

    // Escreve a mensagem no arquivo
    fwrite(data->message, 1, strlen(data->message), file);
    fclose(file);

    printf("Escritor: Mensagem escrita no arquivo.\n");
    printf("Escritor: Aguardando leitura...\n");

    // Sinaliza que o arquivo está pronto para leitura
    *(data->file_processed) = 1;
    pthread_cond_signal(data->cond);

    // Espera até que o leitor termine
    while (*(data->file_processed) == 1) {
        pthread_cond_wait(data->cond, data->mutex);
    }

    pthread_mutex_unlock(data->mutex);

    printf("Escritor: Arquivo lido e removido. Finalizado.\n");
    return NULL;
}

// Função da thread leitora
void *reader_thread(void *arg) {
    thread_data *data = (thread_data *)arg;
    FILE *file;
    char buffer[256];
    size_t bytes_read;

    // Bloqueia o mutex antes de verificar o arquivo
    pthread_mutex_lock(data->mutex);

    // Espera até que o escritor tenha criado o arquivo
    while (*(data->file_processed) == 0) {
        pthread_cond_wait(data->cond, data->mutex);
    }

    // Lê o arquivo
    file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("fopen");
        pthread_mutex_unlock(data->mutex);
        return NULL;
    }

    bytes_read = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);

    buffer[bytes_read] = '\0';
    printf("Leitor: Mensagem lida: %s", buffer);

    // Remove o arquivo
    remove(FILENAME);

    // Sinaliza que terminou de processar
    *(data->file_processed) = 0;
    pthread_cond_signal(data->cond);

    pthread_mutex_unlock(data->mutex);

    printf("Leitor: Finalizado.\n");
    return NULL;
}

int main() {
    pthread_t writer, reader;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    int file_processed = 0;

    thread_data data = {
        .message = "Olá, comunicação via arquivo com threads!\n",
        .file_processed = &file_processed,
        .mutex = &mutex,
        .cond = &cond
    };

    // Cria as threads
    if (pthread_create(&writer, NULL, writer_thread, &data)) {
        perror("pthread_create writer");
        exit(1);
    }

    if (pthread_create(&reader, NULL, reader_thread, &data)) {
        perror("pthread_create reader");
        exit(1);
    }

    // Espera as threads terminarem
    pthread_join(writer, NULL);
    pthread_join(reader, NULL);

    // Limpa os recursos
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <time.h>

#define MSG_SIZE 20 
#define PIPE_READ_END  0
#define PIPE_WRITE_END 1
#define SENTINEL_VALUE 0

int is_prime(long long candidate) {
    if (candidate < 2) return 0;
    if (candidate == 2) return 1;
    if (candidate % 2 == 0) return 0;
    for (long long divisor = 3; divisor * divisor <= candidate; divisor += 2)
        if (candidate % divisor == 0) return 0;
    return 1;
}

void producer(int pipe_write_fd, int total_numbers_to_generate) {
    char message_buffer[MSG_SIZE];
    long long current_number = 1; 
    srand((unsigned)time(NULL) ^ getpid());

    printf("[Producer PID=%d] Starting. Will generate %d numbers.\n",
           getpid(), total_numbers_to_generate);

    for (int sequence_index = 0; sequence_index < total_numbers_to_generate; sequence_index++) {
        int random_delta = (rand() % 100) + 1;     
        
        if (sequence_index > 0) current_number += random_delta;

        snprintf(message_buffer, MSG_SIZE, "%-19lld", current_number);
        message_buffer[MSG_SIZE - 1] = '\0';

        if (write(pipe_write_fd, message_buffer, MSG_SIZE) != MSG_SIZE) {
            perror("write");
            break;
        }
        printf("[Producer] Sent: %lld\n", current_number);
    }

    snprintf(message_buffer, MSG_SIZE, "%-19d", SENTINEL_VALUE);
    message_buffer[MSG_SIZE - 1] = '\0';
    if (write(pipe_write_fd, message_buffer, MSG_SIZE) != MSG_SIZE)
        perror("write sentinel");
    printf("[Producer PID=%d] Sent sentinel 0. Exiting.\n", getpid());
    close(pipe_write_fd);
}

void consumer(int pipe_read_fd) {
    char message_buffer[MSG_SIZE + 1];
    long long received_number;
    int processed_count = 0;

    printf("[Consumer PID=%d] Starting. Waiting for numbers...\n", getpid());

    while (1) {
        ssize_t bytes_read = read(pipe_read_fd, message_buffer, MSG_SIZE);
        if (bytes_read <= 0) {
            if (bytes_read < 0) perror("read");
            break;
        }
        message_buffer[MSG_SIZE] = '\0';
        received_number = atoll(message_buffer);

        if (received_number == SENTINEL_VALUE) {
            printf("[Consumer] Received sentinel 0. Terminating.\n");
            break;
        }

        processed_count++;
        int number_is_prime = is_prime(received_number);
        printf("[Consumer] #%d  num=%-15lld  %s\n",
               processed_count, received_number, number_is_prime ? "PRIME" : "not prime");
    }

    printf("[Consumer PID=%d] Processed %d numbers. Exiting.\n",
           getpid(), processed_count);
    close(pipe_read_fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_numbers>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int total_numbers = atoi(argv[1]);
    if (total_numbers <= 0) {
        fprintf(stderr, "Error: num_numbers must be positive.\n");
        return EXIT_FAILURE;
    }

    int pipe_file_descriptors[2];
    if (pipe(pipe_file_descriptors) == -1) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child_pid == 0) {
        /* Processo filho = Consumidor */
        close(pipe_file_descriptors[PIPE_WRITE_END]); 
        consumer(pipe_file_descriptors[PIPE_READ_END]);
        exit(EXIT_SUCCESS);
    } else {
        /* Processo pai = Produtor */
        close(pipe_file_descriptors[PIPE_READ_END]);
        producer(pipe_file_descriptors[PIPE_WRITE_END], total_numbers);
        wait(NULL);
        printf("[Main] Both processes finished.\n");
    }

    return EXIT_SUCCESS;
}
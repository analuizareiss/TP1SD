#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

static int   buffer_size;        
static int   num_producer_threads; 
static int   num_consumer_threads; 
static long  total_to_consume;     

static int  *shared_buffer;
static int   write_position = 0;   
static int   read_position  = 0;

static sem_t mutex_buffer;       
static sem_t semaphore_free_slots;
static sem_t semaphore_used_slots;

static volatile long consumed_total = 0;
static pthread_mutex_t termination_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int    all_consumed = 0; 

#define MAX_LOG_ENTRIES (1 << 22)
static int  *occupancy_log;  
static long  occupancy_log_count = 0;
static pthread_mutex_t occupancy_log_lock = PTHREAD_MUTEX_INITIALIZER;

// usa int ao inves de long long porque sao numeros menos que na parte 1
static int is_prime(int candidate) {
    if (candidate < 2) return 0;
    if (candidate == 2) return 1;
    if (candidate % 2 == 0) return 0;
    int square_root_limit = (int)sqrt((double)candidate);
    for (int divisor = 3; divisor <= square_root_limit; divisor += 2)
        if (candidate % divisor == 0) return 0;
    return 1;
}

static int get_current_occupancy(void) {
    int occupied_slots;
    sem_getvalue(&semaphore_used_slots, &occupied_slots);
    return occupied_slots;
}

static void log_current_occupancy(void) {
    pthread_mutex_lock(&occupancy_log_lock);
    if (occupancy_log_count < MAX_LOG_ENTRIES) {
        occupancy_log[occupancy_log_count++] = get_current_occupancy();
    }
    pthread_mutex_unlock(&occupancy_log_lock);
}

static void *producer_thread_func(void *thread_id) {
    unsigned int random_seed = (unsigned int)(time(NULL) ^ (uintptr_t)thread_id);

    while (!all_consumed) {
        int generated_number = (rand_r(&random_seed) % 10000000) + 1;

        sem_wait(&semaphore_free_slots);
        if (all_consumed) { sem_post(&semaphore_free_slots); break; }

        sem_wait(&mutex_buffer);
        shared_buffer[write_position] = generated_number;
        write_position = (write_position + 1) % buffer_size;
        sem_post(&mutex_buffer);

        sem_post(&semaphore_used_slots);

        log_current_occupancy();
    }
    return NULL;
}

static void *consumer_thread_func(void *thread_id) {
    (void)thread_id;

    while (1) {
        sem_wait(&semaphore_used_slots);
        if (all_consumed) { sem_post(&semaphore_used_slots); break; }

        sem_wait(&mutex_buffer);
        int consumed_number = shared_buffer[read_position];
        read_position = (read_position + 1) % buffer_size;
        sem_post(&mutex_buffer);

        sem_post(&semaphore_free_slots);

        log_current_occupancy();

        int number_is_prime = is_prime(consumed_number);
        (void)number_is_prime;   

        pthread_mutex_lock(&termination_lock);
        consumed_total++;
        long current_consumed_total = consumed_total;
        pthread_mutex_unlock(&termination_lock);

        if (current_consumed_total >= total_to_consume) {
            all_consumed = 1;
            for (int i = 0; i < num_producer_threads; i++) sem_post(&semaphore_free_slots);
            for (int i = 0; i < num_consumer_threads; i++) sem_post(&semaphore_used_slots);
            break;
        }
    }
    return NULL;
}

static void save_occupancy_to_csv(const char *filename) {
    FILE *csv_file = fopen(filename, "w");
    if (!csv_file) { perror("fopen"); return; }
    fprintf(csv_file, "operation,occupancy\n");
    for (long entry_index = 0; entry_index < occupancy_log_count; entry_index++) {
        fprintf(csv_file, "%ld,%d\n", entry_index, occupancy_log[entry_index]);
    }
    fclose(csv_file);
    printf("[Main] Occupancy log saved to %s (%ld entries)\n",
           filename, occupancy_log_count);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr,
            "Usage: %s <N> <Np> <Nc> [M] [output_dir]\n"
            "  N          = buffer size\n"
            "  Np         = producer threads\n"
            "  Nc         = consumer threads\n"
            "  M          = numbers to consume (default 100000)\n"
            "  output_dir = directory for occupancy CSV (default: .)\n",
            argv[0]);
        return EXIT_FAILURE;
    }

    buffer_size          = atoi(argv[1]);
    num_producer_threads = atoi(argv[2]);
    num_consumer_threads = atoi(argv[3]);
    total_to_consume     = (argc >= 5) ? atol(argv[4]) : 100000L;
    const char *output_dir = (argc >= 6) ? argv[5] : ".";

    if (buffer_size <= 0 || num_producer_threads <= 0 ||
        num_consumer_threads <= 0 || total_to_consume <= 0) {
        fprintf(stderr, "Error: all parameters must be positive.\n");
        return EXIT_FAILURE;
    }

    printf("=== Produtor-Consumidor com Semáforos ===\n");
    printf("N=%d  Np=%d  Nc=%d  M=%ld\n\n",
           buffer_size, num_producer_threads, num_consumer_threads, total_to_consume);

    shared_buffer  = malloc(buffer_size * sizeof(int));
    occupancy_log  = malloc(MAX_LOG_ENTRIES * sizeof(int));
    if (!shared_buffer || !occupancy_log) {
        fprintf(stderr, "Error: malloc failed.\n");
        return EXIT_FAILURE;
    }
    memset(shared_buffer, 0, buffer_size * sizeof(int));

    sem_init(&mutex_buffer,         0, 1);          
    sem_init(&semaphore_free_slots, 0, buffer_size); 
    sem_init(&semaphore_used_slots, 0, 0);          
    
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    pthread_t *producer_threads = malloc(num_producer_threads * sizeof(pthread_t));
    pthread_t *consumer_threads = malloc(num_consumer_threads * sizeof(pthread_t));

    for (int i = 0; i < num_producer_threads; i++)
        pthread_create(&producer_threads[i], NULL, producer_thread_func, (void*)(uintptr_t)i);
    for (int i = 0; i < num_consumer_threads; i++)
        pthread_create(&consumer_threads[i], NULL, consumer_thread_func, (void*)(uintptr_t)i);

    for (int i = 0; i < num_producer_threads; i++) pthread_join(producer_threads[i], NULL);
    for (int i = 0; i < num_consumer_threads; i++) pthread_join(consumer_threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_seconds = (end_time.tv_sec  - start_time.tv_sec) +
                             (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("\n[Main] Consumed: %ld numbers\n", consumed_total);
    printf("[Main] Elapsed time: %.4f seconds\n", elapsed_seconds);

    printf("RESULT N=%d Np=%d Nc=%d time=%.6f\n",
           buffer_size, num_producer_threads, num_consumer_threads, elapsed_seconds);

    char csv_filename[256];
    snprintf(csv_filename, sizeof(csv_filename),
             "%s/occupancy_N%d_Np%d_Nc%d.csv",
             output_dir, buffer_size, num_producer_threads, num_consumer_threads);
    save_occupancy_to_csv(csv_filename);

    sem_destroy(&mutex_buffer);
    sem_destroy(&semaphore_free_slots);
    sem_destroy(&semaphore_used_slots);
    free(shared_buffer);
    free(occupancy_log);
    free(producer_threads);
    free(consumer_threads);

    return EXIT_SUCCESS;
}
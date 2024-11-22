#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define NUM_CONTEXTS 16
#define BUFFER_SIZE 1024

typedef struct {
    double value;
    pthread_mutex_t mutex;
} Context;

Context contexts[NUM_CONTEXTS];
FILE *output_file;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void run_operation(const char *operation, int context_id, int operand, int result) {
    pthread_mutex_lock(&log_mutex);
    fprintf(output_file, "ctx %02d: %s %d (result: %d)\n", context_id, operation, operand, result);
    fflush(output_file);
    pthread_mutex_unlock(&log_mutex);
}

void set_operation(int context_id, int value) {
    pthread_mutex_lock(&log_mutex);
    fprintf(output_file, "ctx %02d: set to value %d\n", context_id, value);
    fflush(output_file);
    pthread_mutex_unlock(&log_mutex);
}

void primes_up_to(int context_id, int n) {
    char result[BUFFER_SIZE] = "";
    int is_prime;

    for (int i = 2; i <= n; i++) {
        is_prime = 1;

        for (int j = 2; j <= i / 2; j++) {
            if (i % j == 0) {
                is_prime = 0;
                break;
            }
        }

        if(i != 2 && is_prime){
            strcat(result, ", ");
        }

        if (is_prime) {
            char buffer[16];
            sprintf(buffer, "%d", i);
            strcat(result, buffer);
        }
    }

    pthread_mutex_lock(&log_mutex);
    fprintf(output_file, "ctx %02d: primes (result: %s)\n", context_id, result);
    fflush(output_file);
    pthread_mutex_unlock(&log_mutex);
}

// Function to approximate π using the Leibniz series
void pi_approximation(int iterations, int context_id) {
    double pi = 0.0;
    for (int i = 0; i < iterations; i++) {
        pi += (i % 2 == 0 ? 1 : -1) / (2.0 * i + 1);
    }
    pi *= 4.0;

    pthread_mutex_lock(&log_mutex);
    fprintf(output_file, "ctx %02d: pia (result %.15f)\n", context_id, pi);
    fflush(output_file);
    pthread_mutex_unlock(&log_mutex);
}

// Recursive Fibonacci function
int recursive_fibonacci(int n) {
    if (n <= 1) return n;
    return recursive_fibonacci(n - 1) + recursive_fibonacci(n - 2);
}

void fib_out(int context_id) {
    int n, fib_result;

    pthread_mutex_lock(&contexts[context_id].mutex);
    n = (int)contexts[context_id].value;
    pthread_mutex_unlock(&contexts[context_id].mutex);

    fib_result = recursive_fibonacci(n);

    pthread_mutex_lock(&log_mutex);
    fprintf(output_file, "ctx %02d: fib (result: %d)\n", context_id, fib_result);
    fflush(output_file);
    pthread_mutex_unlock(&log_mutex);
}

void perform_operation(const char *operation, int context_id, double operand) {
    int initial_value, result;
    pthread_mutex_lock(&contexts[context_id].mutex);
    initial_value = contexts[context_id].value;

    if (strcmp(operation, "add") == 0) {
        result = initial_value + operand;
        contexts[context_id].value = result;
        run_operation(operation, context_id, operand, result);
    }
    else if (strcmp(operation, "sub") == 0) {
        result = initial_value - operand;
        contexts[context_id].value = result;
        run_operation(operation, context_id, operand, result);
    }
    else if (strcmp(operation, "mul") == 0) {
        result = initial_value * operand;
        contexts[context_id].value = result;
        run_operation(operation, context_id, operand, result);
    }
    else if (strcmp(operation, "div") == 0) {
        result = initial_value / operand;
        contexts[context_id].value = result;
        run_operation(operation, context_id, operand, result);
    }
    pthread_mutex_unlock(&contexts[context_id].mutex);
}

void queue_operation(const char *operation, int context_id, int operand) {
    if (strcmp(operation, "set") == 0) {
        pthread_mutex_lock(&contexts[context_id].mutex);
        contexts[context_id].value = operand;
        pthread_mutex_unlock(&contexts[context_id].mutex);

        set_operation(context_id, operand);
    } 
    else if (strcmp(operation, "fib") == 0) {
        fib_out(context_id);
    } 
    else if (strcmp(operation, "pri") == 0) {
        primes_up_to(context_id, operand); // Use operand for prime number computation
    } 
    else if (strcmp(operation, "pia") == 0) {
        pi_approximation((int)contexts[context_id].value, context_id);
    } 
    else {
        perform_operation(operation, context_id, operand);
    }
}

void *process_operations(void *arg) {
    free(arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    FILE *input_file = fopen(argv[1], "r");
    output_file = fopen("temp.txt", "w");  // Ensure output goes to temp.txt
    if (!input_file || !output_file) {
        perror("Error opening files");
        return 1;
    }

    for (int i = 0; i < NUM_CONTEXTS; i++) {
        contexts[i].value = 0;
        pthread_mutex_init(&contexts[i].mutex, NULL);
    }

    pthread_t threads[NUM_CONTEXTS];
    for (int i = 0; i < NUM_CONTEXTS; i++) {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        if (pthread_create(&threads[i], NULL, process_operations, arg) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), input_file)) {
        int context_id;
        char operation[10];
        int operand;

        sscanf(buffer, " %s %d %d", operation, &context_id, &operand);
        queue_operation(operation, context_id, operand);
    }

    fclose(input_file);

    for (int i = 0; i < NUM_CONTEXTS; i++) {
        pthread_join(threads[i], NULL);
    }

    fclose(output_file);

    return 0;
}

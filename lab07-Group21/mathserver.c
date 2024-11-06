#include <stdio.h>
#include <pthread.h>


int main(int argc, char* argv[]) {
    const char usage[] = "Usage: mathserver.out <input trace> <output trace>\n";
    char* input_trace;
    char buffer[1024];

    // Parse command line arguments
    if (argc != 3) {
        printf("%s", usage);
        return 1;
    }
    
    input_trace = argv[1];

    // Open input and output files
    FILE* input_file = fopen(input_trace, "r");


    while ( !feof(input_file) ) {
        // Read input file line by line
        char *rez = fgets(buffer, sizeof(buffer), input_file);
        if ( !rez )
            break;
        else {
            // Remove endline character
            buffer[strlen(buffer) - 1] = '\0';
        }

        // CONTINUE...
    }
    fclose(input_file);

    return 0;
}
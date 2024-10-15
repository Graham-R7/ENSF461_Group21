#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define min(a,b) (((a)<(b))?(a):(b))

// total jobs
int numofjobs = 0;
static int current_id = 0;

struct job {
    // job id is ordered by the arrival; jobs arrived first have smaller job id, always increment by 1
    int id;
    int arrival; // arrival time; safely assume the time unit has the minimal increment of 1
    int length;
    int remaining_time;
    int tickets; // number of tickets for lottery scheduling

    int start_time;
    int completion_time;
    int last_executed_time; 
    int run_start_time;

    struct job *next;
};

// the workload list
struct job *head = NULL;


void append_to(struct job **head_pointer, int arrival, int length, int tickets){

    struct job* new_job = (struct job*) malloc (sizeof(struct job));
    if(!new_job){
        printf("Memory allocation failed");
        return;
    }

    new_job->id = current_id++;
    new_job->arrival = arrival;
    new_job->length = length;
    new_job->remaining_time = length;
    new_job->tickets = tickets;

    new_job->start_time = -1;
    new_job->completion_time = 0;
    new_job->last_executed_time = -1;
    new_job->run_start_time = -1;


    new_job->next = NULL;

    // if list is empty, insert new job at the beginning
    if(*head_pointer == NULL){
        *head_pointer = new_job;
    }else{
        // if list is not empty we'll find the end of the list to append the new job
        struct job* temp = *head_pointer;
        while(temp->next != NULL){
            temp = temp->next;
        }

        temp->next = new_job;
    }

    numofjobs++;
    return;
}


void read_job_config(const char* filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int tickets  = 0;

    char* delim = ",";
    char *arrival = NULL;
    char *length = NULL;

    // TODO, error checking
    fp = fopen(filename, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);


    while ((read = getline(&line, &len, fp)) != -1)
    {
        if( line[read-1] == '\n' )
            line[read-1] =0;
        arrival = strtok(line, delim);
        length = strtok(NULL, delim);
        tickets += 100;
        append_to(&head, atoi(arrival), atoi(length), tickets);
    }

    fclose(fp);
    if (line) free(line);
}


void policy_SJF()
{
    printf("Execution trace with SJF:\n");

    int current_time = 0;
    int jobs_remaining = numofjobs;

    while (jobs_remaining > 0)
    {
        // Build the list of available jobs
        struct job *available_job = NULL;
        struct job *temp = head;
        int available_jobs = 0;

        // Find the job(s) that have arrived and not yet completed
        while (temp != NULL)
        {
            if (temp->arrival <= current_time && temp->completion_time == 0)
            {
                if (available_job == NULL)
                {
                    available_job = temp;
                }
                else
                {
                    // Compare with available_job to find the job with the shortest length
                    if (temp->length < available_job->length)
                    {
                        available_job = temp;
                    }
                    else if (temp->length == available_job->length)
                    {
                        // Break tie by arrival time
                        if (temp->arrival < available_job->arrival)
                        {
                            available_job = temp;
                        }
                    }
                }
                available_jobs++;
            }
            temp = temp->next;
        }

        if (available_jobs == 0)
        {
            // No available jobs, advance current_time to the next job's arrival time
            int next_arrival = INT_MAX;
            temp = head;
            while (temp != NULL)
            {
                if (temp->arrival > current_time && temp->completion_time == 0)
                {
                    if (temp->arrival < next_arrival)
                    {
                        next_arrival = temp->arrival;
                    }
                }
                temp = temp->next;
            }

            if (next_arrival != INT_MAX)
            {
                // CPU is idle
                current_time = next_arrival;
            }
            else
            {
                // No more jobs arriving
                break;
            }

            continue; // Go back to the beginning of the while loop
        }
        else
        {
            // We have at least one available job
            struct job *job = available_job;

            job->start_time = current_time;
            job->completion_time = current_time + job->length;

            printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
                   current_time, job->id, job->arrival, job->length);

            current_time = job->completion_time;

            jobs_remaining--;
        }
    }


    printf("End of execution with SJF.\n");
}


void policy_STCF()
{
    printf("Execution trace with STCF:\n");

    int current_time = 0;
    int jobs_remaining = numofjobs;

    struct job *running_job = NULL;

    while (jobs_remaining > 0)
    {
        // Update the list of arrived jobs
        struct job *temp = head;
        struct job *shortest_job = NULL;

        // Find the job with the shortest remaining time that has arrived
        while (temp != NULL)
        {
            if (temp->arrival <= current_time && temp->remaining_time > 0)
            {
                if (shortest_job == NULL || temp->remaining_time < shortest_job->remaining_time)
                {
                    shortest_job = temp;
                }
                else if (temp->remaining_time == shortest_job->remaining_time)
                {
                    // Break ties by arrival time
                    if (temp->arrival < shortest_job->arrival)
                    {
                        shortest_job = temp;
                    }
                }
            }
            temp = temp->next;
        }

        if (shortest_job == NULL)
        {
            // No job is available, advance time
            current_time++;
            continue;
        }

        // Check for preemption
        if (running_job != shortest_job)
        {
            // If there is a running job, it is being preempted
            if (running_job != NULL)
            {
                // Calculate the time it ran during this period
                int time_ran = current_time - running_job->run_start_time;
                // Print the time ran during this period
                printf("%d]\n", time_ran);
            }

            // Now switch to the new shortest job
            running_job = shortest_job;

            if (running_job->start_time == -1)
            {
                running_job->start_time = current_time;
            }

            running_job->run_start_time = current_time;

            printf("t=%d: [Job %d] arrived at [%d], ran for: [", current_time, running_job->id, running_job->arrival);
        }

        // Run the job for 1 time unit
        running_job->remaining_time--;
        current_time++;

        // Check if the job has completed
        if (running_job->remaining_time == 0)
        {
            // Calculate the time it ran during this period
            int time_ran = current_time - running_job->run_start_time;
            running_job->completion_time = current_time;

            printf("%d]\n", time_ran);

            jobs_remaining--;
            running_job = NULL;
        }
    }

    // If there's a running job at the end (unlikely in this loop, but just in case)
    if (running_job != NULL)
    {
        int time_ran = current_time - running_job->run_start_time;
        printf("%d]\n", time_ran);
    }


    printf("End of execution with STCF.\n");
}

void policy_RR(int slice) {

    printf("Execution trace with RR:\n");

    int current_time = 0;
    int jobs_remaining = numofjobs;

    while (jobs_remaining > 0) {
        struct job *temp = head;
        int job_ran = 0;

        while (temp != NULL) {
            if (temp->arrival <= current_time && temp->remaining_time > 0) {
                int run_time = min(slice, temp->remaining_time);

                if (temp->start_time == -1) {
                    temp->start_time = current_time;
                }

                printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", current_time, temp->id, temp->arrival, run_time);
                current_time += run_time;
                temp->remaining_time -= run_time;
                job_ran = 1;

                if (temp->remaining_time == 0) {
                    temp->completion_time = current_time;
                    jobs_remaining--;
                }
            }
            temp = temp->next;
        }

        if (job_ran == 0) {
            current_time++;
        }
    }

    printf("End of execution with RR.\n");
}

void policy_LT(int slice) {
    srand(42);
    printf("Execution trace with LT:\n");

    int current_time = 0;
    int jobs_remaining = numofjobs;
    int total_tickets = 0;

    struct job *temp = head;
    while (temp != NULL) {
        total_tickets += (temp->id + 1) * 100;
        temp->start_time = -1;
        temp = temp->next;
    }

    while (jobs_remaining > 0) {
        int available_tickets = 0;
        int earliest_arrival = -1;
        temp = head;

        while (temp != NULL) {
            if (temp->arrival <= current_time && temp->remaining_time > 0) {
                available_tickets += (temp->id + 1) * 100;
            } else if (temp->arrival > current_time && (earliest_arrival == -1 || temp->arrival < earliest_arrival)) {
                earliest_arrival = temp->arrival;
            }
            temp = temp->next;
        }

        if (available_tickets == 0 && earliest_arrival > current_time) {
            current_time = earliest_arrival;
            continue;
        }

        int winning_ticket = rand() % available_tickets;
        int ticket_counter = 0;

        struct job *selected_job = NULL;
        temp = head;

        while (temp != NULL) {
            if (temp->arrival <= current_time && temp->remaining_time > 0) {
                ticket_counter += (temp->id + 1) * 100;
                if (ticket_counter > winning_ticket) {
                    selected_job = temp;
                    break;
                }
            }
            temp = temp->next;
        }

        if (selected_job->start_time == -1) {
            selected_job->start_time = current_time;
        }

        int run_time = min(slice, selected_job->remaining_time);
        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n", current_time, selected_job->id, selected_job->arrival, run_time);

        current_time += run_time;
        selected_job->remaining_time -= run_time;

        if (selected_job->remaining_time == 0) {
            selected_job->completion_time = current_time;
            jobs_remaining--;
            total_tickets -= (selected_job->id + 1) * 100;
        }
    }

    printf("End of execution with LT.\n");
}

void policy_FIFO(){
    printf("Execution trace with FIFO:\n");

    int current_time = 0;
    struct job* current = head;

    while(current != NULL){
        if(current_time < current->arrival){
            current_time = current->arrival;
        }
        current->start_time = current_time;
        current->completion_time = current_time + current->length;

        printf("t=%d: [Job %d] arrived at [%d], ran for: [%d]\n",
            current_time, current->id, current->arrival, current->length);

        current_time += current->length;
        current = current->next;
    }


    printf("End of execution with FIFO.\n");
}


int main(int argc, char **argv){

    static char usage[] = "usage: %s analysis policy slice trace\n";

    int analysis;
    char *pname;
    char *tname;
    int slice;

    if (argc < 5)
    {
        fprintf(stderr, "missing variables\n");
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    // if 0, we don't analysis the performance
    analysis = atoi(argv[1]);

    // policy name
    pname = argv[2];

    // time slice, only valid for RR
    slice = atoi(argv[3]);

    // workload trace
    tname = argv[4];

    read_job_config(tname);

    if (strcmp(pname, "FIFO") == 0){
        policy_FIFO();
        if (analysis == 1){
            // Perform analysis
            printf("Begin analyzing FIFO:\n");
            struct job* current = head;

            int RW_time = 0; // FIFO is non-preemptive so response_time = wait_time
            int turnaround_time = 0;

            float sum_RW = 0;
            float sum_turnaround = 0;

            while(current != NULL){
                RW_time = current->start_time - current->arrival;
                turnaround_time = current->completion_time - current->arrival;
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
                    current->id, RW_time, turnaround_time, RW_time);

                sum_RW += RW_time;
                sum_turnaround += turnaround_time;
                current = current->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
                (sum_RW/numofjobs), (sum_turnaround/numofjobs), (sum_RW/numofjobs));

            printf("End analyzing FIFO.\n");
        }
    }
    else if (strcmp(pname, "SJF") == 0)
    {
        policy_SJF();
        if (analysis == 1){
            // Perform analysis
            printf("Begin analyzing SJF:\n");
            struct job* current = head;

            int response_time = 0;
            int turnaround_time = 0;
            int wait_time = 0;

            float sum_response = 0;
            float sum_turnaround = 0;
            float sum_wait = 0;

            while(current != NULL){
                response_time = current->start_time - current->arrival;
                turnaround_time = current->completion_time - current->arrival;
                // For non-preemptive scheduling, wait time is same as response time
                wait_time = response_time;
                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
                    current->id, response_time, turnaround_time, wait_time);

                sum_response += response_time;
                sum_turnaround += turnaround_time;
                sum_wait += wait_time;
                current = current->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
                (sum_response/numofjobs), (sum_turnaround/numofjobs), (sum_wait/numofjobs));

            printf("End analyzing SJF.\n");
        }
    }
    else if (strcmp(pname, "STCF") == 0)
    {
        policy_STCF();
        if (analysis == 1){
            // Perform analysis
            printf("Begin analyzing STCF:\n");
            struct job* current = head;

            int response_time = 0;
            int turnaround_time = 0;
            int wait_time = 0;

            float sum_response = 0;
            float sum_turnaround = 0;
            float sum_wait = 0;

            while(current != NULL){
                response_time = current->start_time - current->arrival;
                turnaround_time = current->completion_time - current->arrival;
                wait_time = turnaround_time - current->length;

                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
                    current->id, response_time, turnaround_time, wait_time);

                sum_response += response_time;
                sum_turnaround += turnaround_time;
                sum_wait += wait_time;
                current = current->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
                (sum_response/numofjobs), (sum_turnaround/numofjobs), (sum_wait/numofjobs));

            printf("End analyzing STCF.\n");
        }
    }

    else if (strcmp(pname, "RR") == 0)
    {
        policy_RR(slice);

        if (analysis == 1){
            // Perform analysis
            printf("Begin analyzing RR:\n");
            struct job* current = head;

            int response_time = 0;
            int turnaround_time = 0;
            int wait_time = 0;

            float sum_response = 0;
            float sum_turnaround = 0;
            float sum_wait = 0;

            while(current != NULL){
                response_time = current->start_time - current->arrival;
                turnaround_time = current->completion_time - current->arrival;
                wait_time = turnaround_time - current->length;

                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
                    current->id, response_time, turnaround_time, wait_time);

                sum_response += response_time;
                sum_turnaround += turnaround_time;
                sum_wait += wait_time;
                current = current->next;
            }
            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
                (sum_response/numofjobs), (sum_turnaround/numofjobs), (sum_wait/numofjobs));

            printf("End analyzing RR.\n");
        }
    }

    else if (strcmp(pname, "LT") == 0)
    {
        policy_LT(slice);

        if (analysis == 1){
            // Perform analysis
            printf("Begin analyzing LT:\n");
            struct job* current = head;

            int response_time = 0;
            int turnaround_time = 0;
            int wait_time = 0;

            float sum_response = 0;
            float sum_turnaround = 0;
            float sum_wait = 0;

            while(current != NULL){
                response_time = current->start_time - current->arrival;
                turnaround_time = current->completion_time - current->arrival;
                wait_time = turnaround_time - current->length;

                printf("Job %d -- Response time: %d  Turnaround: %d  Wait: %d\n",
                    current->id, response_time, turnaround_time, wait_time);

                sum_response += response_time;
                sum_turnaround += turnaround_time;
                sum_wait += wait_time;

                current = current->next;
            }

            printf("Average -- Response: %.2f  Turnaround %.2f  Wait %.2f\n",
                (sum_response / numofjobs), (sum_turnaround / numofjobs), (sum_wait / numofjobs));

            printf("End analyzing LT.\n");
        }
    }

    exit(0);
}

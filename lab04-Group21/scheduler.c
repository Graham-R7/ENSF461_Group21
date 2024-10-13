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
    int tickets; // number of tickets for lottery scheduling
    int start_time;
    int completion_time;

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
    new_job->tickets = tickets;

    new_job->start_time = 0;
    new_job->completion_time = 0;

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
        struct job *available_job = NULL;
        struct job *temp = head;
        int available_jobs = 0;

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
                    if (temp->length < available_job->length)
                    {
                        available_job = temp;
                    }
                    else if (temp->length == available_job->length)
                    {
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
                current_time = next_arrival;
            }
            else
            {
                break;
            }

            continue; 
        }
        else
        {
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

    // TODO: implement STCF policy

    printf("End of execution with STCF.\n");
}


void policy_RR(int slice)
{
    printf("Execution trace with RR:\n");

    // TODO: implement RR policy

    printf("End of execution with RR.\n");
}


void policy_LT(int slice)
{
    printf("Execution trace with LT:\n");

    // Leave this here, it will ensure the scheduling behavior remains deterministic
    srand(42);

    // In the following, you'll need to:
    // Figure out which active job to run first
    // Pick the job with the shortest remaining time
    // Considers jobs in order of arrival, so implicitly breaks ties by choosing the job with the lowest ID

    // To achieve consistency with the tests, you are encouraged to choose the winning ticket as follows:
    // int winning_ticket = rand() % total_tickets;
    // And pick the winning job using the linked list approach discussed in class, or equivalent

    printf("End of execution with LT.\n");

}


void policy_FIFO(){
    printf("Execution trace with FIFO:\n");

    // TODO: implement FIFO policy
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
            // TODO: perform analysis
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
        // TODO
    }
    else if (strcmp(pname, "RR") == 0)
    {
        // TODO
    }
    else if (strcmp(pname, "LT") == 0)
    {
        // TODO
    }

    exit(0);
}

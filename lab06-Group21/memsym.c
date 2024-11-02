#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define NUM_REGISTERS 8
#define MEMORY_SIZE 1024
#define NUM_PROCESSES 4
#define TLB_SIZE 8

typedef struct {
    bool valid;
    int VPN;
    int PFN;
    int PID;
} TLBEntry;

TLBEntry tlb[TLB_SIZE];
int* page_tables[NUM_PROCESSES];
uint32_t* physical_memory = NULL;
bool define_called = false;
int offset_bits, pfn_bits, vpn_bits;
char* strategy;

typedef struct {
    int registers[NUM_REGISTERS];
} ProcessState;

ProcessState process_states[NUM_PROCESSES];
int current_pid = 0;

void init_process_states() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        memset(process_states[i].registers, 0, sizeof(process_states[i].registers));
    }
}

int get_register_index(const char* reg_str) {
    if (strcmp(reg_str, "r1") == 0) return 1;
    if (strcmp(reg_str, "r2") == 0) return 2;
    return -1;
}

void handle_define(int off, int pfn, int vpn, FILE* output_file) {
    if (define_called) {
        fprintf(output_file, "Current PID: %d. Error: multiple calls to define in the same trace\n", current_pid);
        exit(1);
    }

    offset_bits = off;
    pfn_bits = pfn;
    vpn_bits = vpn;

    size_t mem_size = (size_t)(1 << (off + pfn));
    physical_memory = (uint32_t*)calloc(mem_size, sizeof(uint32_t));
    if (!physical_memory) {
        fprintf(stderr, "Error: failed to allocate physical memory\n");
        exit(1);
    }

    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = false;
    }
    for (int i = 0; i < NUM_PROCESSES; i++) {
        page_tables[i] = (int*)calloc((size_t)(1 << vpn), sizeof(int));
        if (!page_tables[i]) {
            fprintf(stderr, "Error: failed to allocate page table\n");
            exit(1);
        }
        memset(page_tables[i], -1, (size_t)(1 << vpn) * sizeof(int));
    }

    init_process_states();
    define_called = true;
    fprintf(output_file, "Current PID: %d. Memory instantiation complete. OFF bits: %d. PFN bits: %d. VPN bits: %d\n", current_pid, off, pfn, vpn);
}

void handle_add(FILE* output_file) {
    int r1_value = process_states[current_pid].registers[1];
    int r2_value = process_states[current_pid].registers[2];
    int result = r1_value + r2_value;

    // Store the result in r1
    process_states[current_pid].registers[1] = result;

    // Output the operation result
    fprintf(output_file, "Current PID: %d. Added contents of registers r1 (%d) and r2 (%d). Result: %d\n",
            current_pid, r1_value, r2_value, result);
}

void handle_map(int vpn, int pfn, FILE* output_file) {
    // Update the page table for the current process
    page_tables[current_pid][vpn] = pfn;

    // Check if VPN already exists in TLB, and update or add a new entry
    bool replaced = false;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].VPN == vpn && tlb[i].PID == current_pid) {
            tlb[i].PFN = pfn;
            replaced = true;
            break;
        }
    }

    // If not replaced, find an invalid entry or use a replacement policy (e.g., FIFO)
    if (!replaced) {
        for (int i = 0; i < TLB_SIZE; i++) {
            if (!tlb[i].valid) {
                tlb[i].valid = true;
                tlb[i].VPN = vpn;
                tlb[i].PFN = pfn;
                tlb[i].PID = current_pid;
                break;
            }
        }
    }

    fprintf(output_file, "Current PID: %d. Mapped virtual page number %d to physical frame number %d\n", current_pid, vpn, pfn);
}

void handle_unmap(int vpn, FILE* output_file) {
    // Invalidate the VPN in the page table
    page_tables[current_pid][vpn] = -1;

    // Invalidate the VPN in the TLB for the current process
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].VPN == vpn && tlb[i].PID == current_pid) {
            tlb[i].valid = false;
            break;
        }
    }

    fprintf(output_file, "Current PID: %d. Unmapped virtual page number %d\n", current_pid, vpn);
}

void handle_ctxswitch(int pid, FILE* output_file) {
    if (pid < 0 || pid >= NUM_PROCESSES) {
        fprintf(output_file, "Current PID: %d. Invalid context switch to process %d\n", current_pid, pid);
        exit(1);
    }

    current_pid = pid;
    fprintf(output_file, "Current PID: %d. Switched execution context to process: %d\n", current_pid, pid);
}

void handle_pinspect(int vpn, FILE* output_file) {
    if (vpn < 0 || vpn >= (1 << vpn_bits)) {
        fprintf(output_file, "Current PID: %d. Error: invalid VPN %d\n", current_pid, vpn);
        exit(1);
    }

    int pfn = page_tables[current_pid][vpn];
    int valid = (pfn != -1) ? 1 : 0;
    if(pfn == -1) {
        pfn = 0;
    }
    
    fprintf(output_file, "Current PID: %d. Inspected page table entry %d. Physical frame number: %d. Valid: %d\n",
            current_pid, vpn, pfn, valid);
}

void handle_tinspect(int tlbn, FILE* output_file) {
    if (tlbn < 0 || tlbn >= TLB_SIZE) {
        fprintf(output_file, "Current PID: %d. Error: invalid TLB entry %d\n", current_pid, tlbn);
        exit(1);
    }

    TLBEntry entry = tlb[tlbn];
    int valid = entry.valid ? 1 : 0;
    
    fprintf(output_file, "Current PID: %d. Inspected TLB entry %d. VPN: %d. PFN: %d. Valid: %d. PID: %d. Timestamp: <timestamp_placeholder>\n",
            current_pid, tlbn, entry.VPN, entry.PFN, valid, entry.PID);
}

void handle_linspect(int pl, FILE* output_file) {
    if (pl < 0 || pl >= (1 << (offset_bits + pfn_bits))) {
        fprintf(output_file, "Current PID: %d. Error: invalid physical location %d\n", current_pid, pl);
        exit(1);
    }

    uint32_t value = physical_memory[pl];
    fprintf(output_file, "Current PID: %d. Inspected physical location %d. Value: %u\n", current_pid, pl, value);
}

void handle_rinspect(const char* reg, FILE* output_file) {
    int reg_num = get_register_index(reg);
    if (reg_num == -1) {
        fprintf(output_file, "Current PID: %d. Error: invalid register %s\n", current_pid, reg);
        exit(1);
    }

    uint32_t value = process_states[current_pid].registers[reg_num];
    fprintf(output_file, "Current PID: %d. Inspected register %s. Content: %u\n", current_pid, reg, value);
}

int translate_vpn_to_pfn(int vpn, FILE* output_file) {
    // Check TLB for the current process
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].VPN == vpn && tlb[i].PID == current_pid) {
            fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %d hit in TLB entry %d. PFN is %d\n", 
                    current_pid, vpn, i, tlb[i].PFN);
            return tlb[i].PFN;  // TLB hit
        }
    }
    
    // TLB miss - fetch from page table
    fprintf(output_file, "Current PID: %d. Translating. Lookup for VPN %d missed in TLB\n", current_pid, vpn);
    int pfn = page_tables[current_pid][vpn];
    if (pfn == -1) {
        fprintf(output_file, "Current PID: %d. Error: invalid page table entry for VPN %d\n", current_pid, vpn);
        exit(1);
    }

    // Add/update TLB entry for the current VPN
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            tlb[i].valid = true;
            tlb[i].VPN = vpn;
            tlb[i].PFN = pfn;
            tlb[i].PID = current_pid;
            break;
        }
    }
    return pfn;
}

void handle_load(const char* dst, const char* src, FILE* output_file) {
    int reg_num = get_register_index(dst);
    if (reg_num == -1) {
        fprintf(output_file, "Current PID: %d. Error: invalid register operand %s\n", current_pid, dst);
        exit(1);
    }

    if (src[0] == '#') {
        int immediate_value = atoi(src + 1);
        process_states[current_pid].registers[reg_num] = immediate_value;
        fprintf(output_file, "Current PID: %d. Loaded immediate %d into register %s\n", current_pid, immediate_value, dst);
    } else {
        int mem_addr = atoi(src);
        int vpn = mem_addr >> offset_bits;
        int offset = mem_addr & ((1 << offset_bits) - 1);
        int pfn = translate_vpn_to_pfn(vpn, output_file);

        int physical_addr = (pfn << offset_bits) | offset;
        process_states[current_pid].registers[reg_num] = physical_memory[physical_addr];
        fprintf(output_file, "Current PID: %d. Loaded value of location %d (%d) into register %s\n",
                current_pid, physical_addr, process_states[current_pid].registers[reg_num], dst);
    }
}

void handle_store(const char* dst, const char* src, FILE* output_file) {
    // Parse destination as a memory address
    int mem_addr = atoi(dst);
    int vpn = mem_addr >> offset_bits;
    int offset = mem_addr & ((1 << offset_bits) - 1);
    int pfn = translate_vpn_to_pfn(vpn, output_file);
    int physical_addr = (pfn << offset_bits) | offset;

    // Determine the value to store
    int value;
    if (src[0] == 'r') {  // src is a register
        int reg_num = get_register_index(src);
        if (reg_num == -1) {  // Invalid register name
            fprintf(output_file, "Current PID: %d. Error: invalid register operand %s\n", current_pid, src);
            exit(1);
        }
        // Store the value from the register
        value = process_states[current_pid].registers[reg_num];
        physical_memory[physical_addr] = value;
        fprintf(output_file, "Current PID: %d. Stored value of register %s (%d) into location %d\n", current_pid, src, value, physical_addr);
        
    } else if (src[0] == '#') {  // src is an immediate
        value = atoi(src + 1);
        physical_memory[physical_addr] = value;
        fprintf(output_file, "Current PID: %d. Stored immediate %d into location %d\n", current_pid, value, physical_addr);
        
    } else {  // Invalid src format
        fprintf(output_file, "Current PID: %d. Error: invalid source operand %s\n", current_pid, src);
        exit(1);
    }
}


void process_instruction(char** tokens, FILE* output_file) {
    if (tokens[0] == NULL) return;

    // Check if define has been called
    if (!define_called && strcmp(tokens[0], "define") != 0) {
        fprintf(output_file, "Current PID: %d. Error: attempt to execute instruction before define\n", current_pid);
        exit(1);
    }

    if (strcmp(tokens[0], "define") == 0) {
        int off = atoi(tokens[1]);
        int pfn = atoi(tokens[2]);
        int vpn = atoi(tokens[3]);
        handle_define(off, pfn, vpn, output_file);
    } else if (strcmp(tokens[0], "ctxswitch") == 0) {
        int pid = atoi(tokens[1]);
        handle_ctxswitch(pid, output_file);
    } else if (strcmp(tokens[0], "load") == 0) {
        handle_load(tokens[1], tokens[2], output_file);
    } else if (strcmp(tokens[0], "add") == 0) {
        handle_add(output_file);
    } else if (strcmp(tokens[0], "map") == 0) {
        int vpn = atoi(tokens[1]);
        int pfn = atoi(tokens[2]);
        handle_map(vpn, pfn, output_file);
    } else if (strcmp(tokens[0], "unmap") == 0) {
        int vpn = atoi(tokens[1]);
        handle_unmap(vpn, output_file);
    } else if (strcmp(tokens[0], "store") == 0) {
        handle_store(tokens[1], tokens[2], output_file);
    } else if (strcmp(tokens[0], "pinspect") == 0) {
        int vpn = atoi(tokens[1]);
        handle_pinspect(vpn, output_file);
    } else if (strcmp(tokens[0], "tinspect") == 0) {
        int tlbn = atoi(tokens[1]);
        handle_tinspect(tlbn, output_file);
    } else if (strcmp(tokens[0], "linspect") == 0) {
        int pl = atoi(tokens[1]);
        handle_linspect(pl, output_file);
    } else if (strcmp(tokens[0], "rinspect") == 0) {
        handle_rinspect(tokens[1], output_file);
    } else {
        fprintf(output_file, "Current PID: %d. Error: Unknown instruction %s\n", current_pid, tokens[0]);
    }
}

char** tokenize_input(char* input) {
    char** tokens = NULL;
    char* token = strtok(input, " ");
    int num_tokens = 0;

    while (token != NULL) {
        tokens = realloc(tokens, ++num_tokens * sizeof(char*));
        tokens[num_tokens - 1] = strdup(token);
        token = strtok(NULL, " ");
    }

    tokens = realloc(tokens, ++num_tokens * sizeof(char*));
    tokens[num_tokens - 1] = NULL;
    return tokens;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: memsym.out <strategy> <input trace> <output trace>\n");
        return 1;
    }

    strategy = argv[1];
    FILE* input_file = fopen(argv[2], "r");
    FILE* output_file = fopen(argv[3], "w");
    if (!input_file || !output_file) {
        fprintf(stderr, "Error: Could not open input/output file.\n");
        return 1;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), input_file)) {
        if (buffer[0] == '%' || strlen(buffer) == 1) continue;
        buffer[strcspn(buffer, "\n")] = '\0';

        char** tokens = tokenize_input(buffer);
        process_instruction(tokens, output_file);

        for (int i = 0; tokens[i] != NULL; i++) free(tokens[i]);
        free(tokens);
    }

    fclose(input_file);
    fclose(output_file);
    return 0;
}

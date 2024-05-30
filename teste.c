#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 128
#define PAGE_SIZE 256
#define BACKING_STORE "BACKING_STORE.bin"

typedef struct {
    int page_number;
    int frame_number;
    int last_used;
} PageTableEntry;

typedef struct {
    int page_number;
    int frame_number;
} TLBEntry;

TLBEntry tlb[TLB_SIZE];
PageTableEntry page_table[PAGE_TABLE_SIZE];

int tlb_index = 0;
int page_table_index = 0;
int page_faults = 0;
int tlb_hits = 0;
int current_time = 0;

FILE *backing_store;

void initialize() {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].page_number = -1;
    }
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        page_table[i].page_number = -1;
    }
    backing_store = fopen(BACKING_STORE, "rb");
    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin\n");
        exit(1);
    }
}

int search_tlb(int page_number, int *tlb_position) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page_number == page_number) {
            *tlb_position = i;
            return tlb[i].frame_number;
        }
    }
    return -1;
}

void add_to_tlb(int page_number, int frame_number, int *tlb_position) {
    tlb[tlb_index].page_number = page_number;
    tlb[tlb_index].frame_number = frame_number;
    *tlb_position = tlb_index;
    tlb_index = (tlb_index + 1) % TLB_SIZE;
}

int search_page_table(int page_number) {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (page_table[i].page_number == page_number) {
            page_table[i].last_used = current_time++;
            return page_table[i].frame_number;
        }
    }
    return -1;
}

void add_to_page_table_lru(int page_number, int frame_number) {
    int lru_index = 0;
    for (int i = 1; i < PAGE_TABLE_SIZE; i++) {
        if (page_table[i].last_used < page_table[lru_index].last_used) {
            lru_index = i;
        }
    }
    page_table[lru_index].page_number = page_number;
    page_table[lru_index].frame_number = frame_number;
    page_table[lru_index].last_used = current_time++;
}

int get_free_frame() {
    static int next_frame = 0;
    if (next_frame >= PAGE_TABLE_SIZE) {
        next_frame = 0;
    }
    return next_frame++;
}

void read_from_backing_store(int page_number, unsigned char *buffer) {
    fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);
    fread(buffer, sizeof(unsigned char), PAGE_SIZE, backing_store);
}

void translate_addresses(const char *address_file, const char *algorithm) {
    FILE *input_file = fopen(address_file, "r");
    FILE *output_file = fopen("correct.txt", "w");
    if (input_file == NULL) {
        fprintf(stderr, "Error opening address file\n");
        exit(1);
    }
    int address_count = 0;
    char address[10];
    unsigned char buffer[PAGE_SIZE];
    while (fgets(address, sizeof(address), input_file) != NULL) {
        int logical_address = atoi(address);
        int page_number = (logical_address >> 8) & 0xFF;
        int offset = logical_address & 0xFF;
        int tlb_position;
        int frame_number = search_tlb(page_number, &tlb_position);
        if (frame_number == -1) {
            frame_number = search_page_table(page_number);
            if (frame_number == -1) {
                page_faults++;
                frame_number = get_free_frame();
                read_from_backing_store(page_number, buffer);
                if (strcmp(algorithm, "fifo") == 0) {
                    page_table[page_table_index].page_number = page_number;
                    page_table[page_table_index].frame_number = frame_number;
                    page_table[page_table_index].last_used = current_time++;
                    page_table_index = (page_table_index + 1) % PAGE_TABLE_SIZE;
                } else if (strcmp(algorithm, "lru") == 0) {
                    add_to_page_table_lru(page_number, frame_number);
                }
            } else {
                read_from_backing_store(page_number, buffer);
            }
            add_to_tlb(page_number, frame_number, &tlb_position);
        } else {
            read_from_backing_store(page_number, buffer);
            tlb_hits++;
        }
        int physical_address = (frame_number * PAGE_SIZE) + offset;
        int value = buffer[offset];
        fprintf(output_file, "Virtual address: %d TLB: %d Physical address: %d Value: %d\n", 
                logical_address, tlb_position, physical_address, (int8_t)value);
        address_count++;
    }
    fclose(input_file);

    fprintf(output_file, "Number of Translated Addresses = %d\n", address_count);
    fprintf(output_file, "Page Faults = %d\n", page_faults);
    fprintf(output_file, "Page Fault Rate = %.3f\n", (double)page_faults / address_count);
    fprintf(output_file, "TLB Hits = %d\n", tlb_hits);
    fprintf(output_file, "TLB Hit Rate = %.3f\n", (double)tlb_hits / address_count);
    fclose(output_file);
    fclose(backing_store);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <address_file> <fifo|lru>\n", argv[0]);
        return 1;
    }
    initialize();
    translate_addresses(argv[1], argv[2]);
    return 0;
}

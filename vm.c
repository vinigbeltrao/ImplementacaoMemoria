#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Function to extract page number and offset from a 32-bit address
void extract_page_number_and_offset(uint32_t address, uint8_t *page_number, uint8_t *offset) {
    uint16_t masked_address = address & 0xFFFF; // Mask the rightmost 16 bits
    *page_number = (masked_address >> 8) & 0xFF;
    *offset = masked_address & 0xFF;
}

// Function to read a page from the BACKING_STORE
void read_backing_store(uint8_t page_number, uint8_t *buffer) {
    FILE *file = fopen("BACKING_STORE.bin", "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin\n");
        exit(EXIT_FAILURE);
    }
    
    fseek(file, page_number * 256, SEEK_SET);
    fread(buffer, sizeof(uint8_t), 256, file);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <arquivo de endereços> <algoritmo de substituição>\n", argv[0]);
        return 1;
    }

    char *nome_arquivo = argv[1];
    char *algoritmo = argv[2];

    FILE *address_file = fopen(nome_arquivo, "r");
    if (address_file == NULL) {
        fprintf(stderr, "Error abrindo arquivo de leitura\n");
        return EXIT_FAILURE;
    }
    
    uint32_t address;
    uint8_t page_number, offset;
    uint8_t buffer[256]; // Buffer to hold the page data
    
    while (fscanf(address_file, "%u", &address) != EOF) {
        extract_page_number_and_offset(address, &page_number, &offset);
        read_backing_store(page_number, buffer);
        
        uint8_t value = buffer[offset];
        
        printf("Virtual Address: %u -> Page Number: %u, Offset: %u -> Value: %u\n", 
               address, page_number, offset, value);
    }
    
    fclose(address_file);
    return 0;
}

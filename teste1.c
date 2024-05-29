#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FRAME_SIZE 256
#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 128
#define BUFFER_SIZE 10
#define NUMBER_OF_FRAMES 128

// Estruturas de dados
int pagina_para_frame[PAGE_TABLE_SIZE];
int tlb_paginas[TLB_SIZE];
int tlb_frames[TLB_SIZE];
int frame_uso[NUMBER_OF_FRAMES];
int tempo = 0;

int numero_de_frames_usados = 0;
int tlb_index = 0;

// Estatísticas
int translated_addresses = 0;
int page_faults = 0;
int tlb_hits = 0;

// Funções
void inicializa_estruturas();
int traduz_endereco(int endereco_logico, char* algoritmo);
void carrega_pagina(int numero_pagina, int frame);
void atualiza_tlb(int numero_pagina, int frame);
int encontra_frame_para_substituicao(char *algoritmo);
void imprime_saida(int endereco_logico, int endereco_fisico, int valor, int tlb_index);
void imprime_estatisticas();

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <arquivo de endereços> <algoritmo de substituição>\n", argv[0]);
        return 1;
    }

    char *nome_arquivo = argv[1];
    char *algoritmo = argv[2];

    inicializa_estruturas();

    FILE *arquivo = fopen(nome_arquivo, "r");
    if (arquivo == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", nome_arquivo);
        return 1;
    }

    char linha[BUFFER_SIZE];
    while (fgets(linha, BUFFER_SIZE, arquivo) != NULL) {
        int endereco_logico = atoi(linha);
        int endereco_fisico = traduz_endereco(endereco_logico, algoritmo);

        // Lê o valor diretamente da memória secundária
        int offset = endereco_fisico;
        unsigned char valor;
        int fd = open("BACKING_STORE.bin", O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Erro ao abrir BACKING_STORE.bin\n");
            exit(1);
        }
        lseek(fd, offset, SEEK_SET);
        read(fd, &valor, sizeof(valor));
        close(fd);

        imprime_saida(endereco_logico, endereco_fisico, valor, tlb_index);
    }

    fclose(arquivo);
    imprime_estatisticas();
    return 0;
}

void inicializa_estruturas() {
    memset(pagina_para_frame, -1, sizeof(pagina_para_frame));
    memset(tlb_paginas, -1, sizeof(tlb_paginas));
    memset(tlb_frames, -1, sizeof(tlb_frames));
    memset(frame_uso, 0, sizeof(frame_uso));
}

int traduz_endereco(int endereco_logico, char* algoritmo) {
    int numero_pagina = (endereco_logico >> 8) & 0xFF;
    int deslocamento = endereco_logico & 0xFF;
    int frame = -1;

    translated_addresses++;

    // Verificar na TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb_paginas[i] == numero_pagina) {
            frame = tlb_frames[i];
            tlb_hits++;
            break;
        }
    }

    // Se não encontrar na TLB, verificar na tabela de páginas
    if (frame == -1) {
        frame = pagina_para_frame[numero_pagina % PAGE_TABLE_SIZE];
        if (frame == -1) {
            // Se não encontrar na tabela de páginas, carregar da memória secundária
            page_faults++;
            if (numero_de_frames_usados < NUMBER_OF_FRAMES) {
                frame = numero_de_frames_usados++;
            } else {
                frame = encontra_frame_para_substituicao(algoritmo);
            }
            carrega_pagina(numero_pagina, frame);
            pagina_para_frame[numero_pagina % PAGE_TABLE_SIZE] = frame;
        }
        atualiza_tlb(numero_pagina, frame);
    }

    frame_uso[frame] = tempo++; // Atualiza o tempo de uso para LRU
    return (frame * FRAME_SIZE) + deslocamento;
}

void carrega_pagina(int numero_pagina, int frame) {
    int offset = numero_pagina * FRAME_SIZE;
    unsigned char buffer[FRAME_SIZE];
    int fd = open("BACKING_STORE.bin", O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Erro ao abrir BACKING_STORE.bin\n");
        exit(1);
    }
    lseek(fd, offset, SEEK_SET);
    read(fd, buffer, FRAME_SIZE);
    close(fd);
    // Aqui, o buffer é carregado na memória secundária
}

void atualiza_tlb(int numero_pagina, int frame) {
    tlb_paginas[tlb_index] = numero_pagina;
    tlb_frames[tlb_index] = frame;
    tlb_index = (tlb_index + 1) % TLB_SIZE;
}

int encontra_frame_para_substituicao(char *algoritmo) {
    int frame_para_substituir = 0;
    if (strcmp(algoritmo, "fifo") == 0) {
        static int fifo_index = 0;
        frame_para_substituir = fifo_index;
        fifo_index = (fifo_index + 1) % NUMBER_OF_FRAMES;
    } else if (strcmp(algoritmo, "lru") == 0) {
        int lru_tempo = tempo;
        for (int i = 0; i < NUMBER_OF_FRAMES; i++) {
            if (frame_uso[i] < lru_tempo) {
                lru_tempo = frame_uso[i];
                frame_para_substituir = i;
            }
        }
    }
    return frame_para_substituir;
}

void imprime_saida(int endereco_logico, int endereco_fisico, int valor, int tlb_index) {
    printf("Virtual address: %d TLB: %d Physical address: %d Value: %d\n", endereco_logico, tlb_index, endereco_fisico, valor);
}

void imprime_estatisticas() {
    printf("Number of Translated Addresses = %d\n", translated_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", (float)page_faults / translated_addresses);
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", (float)tlb_hits / translated_addresses);
}

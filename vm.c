#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define FRAME_SIZE 256
#define TLB_SIZE 16
#define PAGE_TABLE_SIZE 256
#define BUFFER_SIZE 10

// Estruturas de dados
int pagina_para_frame[PAGE_TABLE_SIZE];
int tlb_paginas[TLB_SIZE];
int tlb_frames[TLB_SIZE];
int tlb_index = 0;

// Funções
void inicializa_estruturas();
int traduz_endereco(int endereco_logico, char* algoritmo);
void carrega_pagina(int numero_pagina, int frame);
void atualiza_tlb(int numero_pagina, int frame);

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
        printf("Endereço lógico: %d Endereço físico: %d\n", endereco_logico, endereco_fisico);
    }

    fclose(arquivo);
    return 0;
}

void inicializa_estruturas() {
    memset(pagina_para_frame, -1, sizeof(pagina_para_frame));
    memset(tlb_paginas, -1, sizeof(tlb_paginas));
    memset(tlb_frames, -1, sizeof(tlb_frames));
}

int traduz_endereco(int endereco_logico, char* algoritmo) {
    int numero_pagina = (endereco_logico >> 8) & 0xFF;
    int deslocamento = endereco_logico & 0xFF;
    int frame = -1;

    // Verificar na TLB
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb_paginas[i] == numero_pagina) {
            frame = tlb_frames[i];
            break;
        }
    }

    // Se não encontrar na TLB, verificar na tabela de páginas
    if (frame == -1) {
        frame = pagina_para_frame[numero_pagina];
        if (frame == -1) {
            // Se não encontrar na tabela de páginas, carregar da memória secundária
            carrega_pagina(numero_pagina, frame);
            frame = numero_pagina; // Assumindo um mapeamento direto para simplificar
            pagina_para_frame[numero_pagina] = frame;
        }
        atualiza_tlb(numero_pagina, frame);
    }

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
    // Aqui, você pode usar buffer conforme necessário. Estamos assumindo mapeamento direto.
}

void atualiza_tlb(int numero_pagina, int frame) {
    tlb_paginas[tlb_index] = numero_pagina;
    tlb_frames[tlb_index] = frame;
    tlb_index = (tlb_index + 1) % TLB_SIZE;
}

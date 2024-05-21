#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int binarioParaDecimal(int numeroBinario);

int main(int argc, char* argv[]){
    
    return 0;
}

int binarioParaDecimal(int numeroBinario) {
    int numeroDecimal = 0, base = 1, resto;

    while (numeroBinario != 0) {
        resto = numeroBinario % 10;
        numeroDecimal += resto * base;
        numeroBinario /= 10;
        base *= 2;
    }

    return numeroDecimal;
}
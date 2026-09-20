#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args.h"

int parse_arguments(int argc, char *argv[],
                     int *peerport, char **dsip, int *dsport)
{
    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-m") == 0) {

            if (i + 1 < argc) {
                *peerport = atoi(argv[i + 1]);
                i++;
            } else {
                printf("Erro: falta a peerport depois de -m.\n");
                return 0;
            }

        } else if (strcmp(argv[i], "-n") == 0) {

            if (i + 1 < argc) {
                *dsip = argv[i + 1];
                i++;
            } else {
                printf("Erro: falta o DSIP depois de -n.\n");
                return 0;
            }

        } else if (strcmp(argv[i], "-p") == 0) {

            if (i + 1 < argc) {
                *dsport = atoi(argv[i + 1]);
                i++;
            } else {
                printf("Erro: falta o DSport depois de -p.\n");
                return 0;
            }

        } else {

            printf("Erro: argumento desconhecido: %s\n", argv[i]);
            return 0;
        }
    }

    if (*peerport == -1) {
        printf("Erro: a peerport (-m) é obrigatória.\n");
        return 0;
    }

    return 1;
}

#ifndef ARGS_H
#define ARGS_H

/* Faz o parsing dos argumentos da linha de comandos */
int parse_arguments(int argc, char *argv[],
                     int *peerport, char **dsip, int *dsport);

#endif /* ARGS_H */

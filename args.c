#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "args.h"

static void print_usage(void)
{
    printf("Usage: ./user -m peerport [-n DSIP] [-p DSport]\n");
}

/* Converte um porto em texto. Devolve 1..65535 ou -1 se o texto não for
 * inteiramente um número decimal válido nesse intervalo. */
static int parse_port(const char *text)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' || value < 1 || value > 65535) {
        return -1;
    }

    return (int)value;
}

int parse_arguments(int argc, char *argv[],
                     int *peerport, char **dsip, int *dsport)
{
    int seen_m = 0, seen_n = 0, seen_p = 0;

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-m") == 0) {

            if (seen_m) {
                printf("Error: option -m given more than once.\n");
                return 0;
            }
            if (i + 1 >= argc) {
                printf("Error: missing peerport after -m.\n");
                print_usage();
                return 0;
            }

            *peerport = parse_port(argv[i + 1]);
            if (*peerport == -1) {
                printf("Error: invalid peerport '%s'.\n", argv[i + 1]);
                return 0;
            }
            seen_m = 1;
            i++;

        } else if (strcmp(argv[i], "-n") == 0) {

            if (seen_n) {
                printf("Error: option -n given more than once.\n");
                return 0;
            }
            if (i + 1 >= argc) {
                printf("Error: missing DSIP after -n.\n");
                print_usage();
                return 0;
            }

            *dsip = argv[i + 1];
            seen_n = 1;
            i++;

        } else if (strcmp(argv[i], "-p") == 0) {

            if (seen_p) {
                printf("Error: option -p given more than once.\n");
                return 0;
            }
            if (i + 1 >= argc) {
                printf("Error: missing DSport after -p.\n");
                print_usage();
                return 0;
            }

            *dsport = parse_port(argv[i + 1]);
            if (*dsport == -1) {
                printf("Error: invalid DSport '%s'.\n", argv[i + 1]);
                return 0;
            }
            seen_p = 1;
            i++;

        } else {

            printf("Error: unknown argument: %s\n", argv[i]);
            print_usage();
            return 0;
        }
    }

    if (!seen_m) {
        printf("Error: the peerport (-m) is mandatory.\n");
        print_usage();
        return 0;
    }

    return 1;
}
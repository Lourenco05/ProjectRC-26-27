#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_DSIP "193.136.138.142"
#define DEFAULT_DSPORT 59000

// Processar os argumentos
int parse_arguments(int argc, char *argv[], int *peerport, char **dsip, int *dsport){
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

    // Verificar se a peerport foi fornecida
    if (*peerport == -1) {
        printf("Erro: a peerport (-m) é obrigatória.\n");
        return 0;
    }

    // Mostrar os valores obtidos
    printf("peerport = %d\n", *peerport);
    printf("DSIP     = %s\n", *dsip);
    printf("DSport   = %d\n", *dsport);
    return 1;
}





int main(int argc, char *argv[]) {

    int peerport = -1;
    char *dsip = DEFAULT_DSIP;
    int dsport = DEFAULT_DSPORT;

    if (parse_arguments(argc, argv, &peerport, &dsip, &dsport) == 0) return 1;

    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    printf("Socket UDP criado com sucesso.\n");

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dsport);
    
    if (inet_pton(AF_INET, dsip, &server_addr.sin_addr) <= 0) {
        printf("Erro: endereço IP inválido.\n");
        close(sockfd);
        return 1;
    }

    printf("Endereço do DS configurado com sucesso.\n");

    char buffer[256];

    while (1) {

        printf("> ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        char command[20];
        char uid[20];
        char password[20];

        int fields = sscanf(buffer, "%19s %19s %19s",
                             command, uid, password);

        if(strcmp(command, "login") == 0){
            login(uid, password);
        }
        
        if (fields >= 2) {
            printf("UID: %s\n", uid);
        }

        if (fields >= 3) {
            printf("Password: %s\n", password);
        }
    }

    close(sockfd);

    return 0;
}


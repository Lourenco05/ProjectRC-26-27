#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_DSIP "193.136.138.142"
#define DEFAULT_DSPORT 59000




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

    printf("peerport = %d\n", *peerport);
    printf("DSIP     = %s\n", *dsip);
    printf("DSport   = %d\n", *dsport);

    return 1;
}




int communicate(int sockfd,
                struct sockaddr_in *server_addr,
                const char *msg,
                char *response,
                size_t resp_len)
{
    socklen_t addr_len = sizeof(*server_addr);

    /* Enviar mensagem para o DS */
    if (sendto(sockfd,
               msg,
               strlen(msg),
               0,
               (struct sockaddr *)server_addr,
               addr_len) < 0)
    {
        perror("sendto");
        return -1;
    }

    /* Receber resposta */
    struct sockaddr_in sender_addr;
    addr_len = sizeof(sender_addr);

    ssize_t bytes_recvd = recvfrom(sockfd,
                                   response,
                                   resp_len - 1,
                                   0,
                                   (struct sockaddr *)&sender_addr,
                                   &addr_len);

    if (bytes_recvd < 0) {
        perror("recvfrom");
        return -1;
    }

    response[bytes_recvd] = '\0';

    return 0;
}




int login(int sockfd,
          struct sockaddr_in *server_addr,
          const char *uid,
          const char *password,
          int peerport)
{
    char message[128];
    char response[256];

    sprintf(message,
            "LIN %s %s %d\n",
            uid,
            password,
            peerport);

    if (communicate(sockfd,
                    server_addr,
                    message,
                    response,
                    sizeof(response)) == 0)
    {
        char status[10];

        if (sscanf(response, "RLI %9s", status) == 1) {

            if (strcmp(status, "OK") == 0) {
                printf("Successful login.\n");
                return 1;

            } else if (strcmp(status, "NOK") == 0) {
                printf("Incorrect login attempt.\n");
                return 0;

            } else if (strcmp(status, "REG") == 0) {
                printf("New user registered.\n");
                return 1;

            } else if (strcmp(status, "ERR") == 0) {
                printf("Login request error.\n");
                return 0;

            } else {
                printf("Unexpected login response.\n");
                return 0;
            }
        }
    }

    return 0;
}



int logout(int sockfd,
           struct sockaddr_in *server_addr,
           const char *uid,
           const char *password)
{
    char message[128];
    char response[256];

    sprintf(message,
            "LOU %s %s\n",
            uid,
            password);

    if (communicate(sockfd,
                    server_addr,
                    message,
                    response,
                    sizeof(response)) == 0)
    {
        char status[10];

        if (sscanf(response, "RLO %9s", status) == 1) {

            if (strcmp(status, "OK") == 0) {
                printf("Successful logout.\n");
                return 1;

            } else if (strcmp(status, "NLG") == 0) {
                printf("User not logged in.\n");

            } else if (strcmp(status, "UNR") == 0) {
                printf("Unknown user.\n");

            } else if (strcmp(status, "WRP") == 0) {
                printf("Wrong password.\n");

            } else if (strcmp(status, "ERR") == 0) {
                printf("Logout request error.\n");

            } else {
                printf("Unexpected logout response.\n");
            }
        }
    }

    return 0;
}



int unregister_user(int sockfd,
                    struct sockaddr_in *server_addr,
                    const char *uid,
                    const char *password)
{
    char message[128];
    char response[256];

    sprintf(message,
            "UNR %s %s\n",
            uid,
            password);

    if (communicate(sockfd,
                    server_addr,
                    message,
                    response,
                    sizeof(response)) == 0)
    {
        char status[10];

        if (sscanf(response, "RUR %9s", status) == 1) {

            if (strcmp(status, "OK") == 0) {
                printf("Successful unregister.\n");
                return 1;

            } else if (strcmp(status, "NOK") == 0) {
                printf("User not logged in.\n");

            } else if (strcmp(status, "UNR") == 0) {
                printf("Unknown user.\n");

            } else if (strcmp(status, "WRP") == 0) {
                printf("Wrong password.\n");

            } else if (strcmp(status, "ERR") == 0) {
                printf("Unregister request error.\n");

            } else {
                printf("Unexpected unregister response.\n");
            }
        }
    }

    return 0;
}



int validar_UID(const char *UID)
{
    if (strlen(UID) != 6) {
        return 0;
    }

    for (int i = 0; i < 6; i++) {

        if (!isdigit((unsigned char)UID[i])) {
            return 0;
        }
    }

    return 1;
}


int validar_Password(const char *Password)
{
    if (strlen(Password) != 8) {
        return 0;
    }

    for (int i = 0; i < 8; i++) {

        if (!isalnum((unsigned char)Password[i])) {
            return 0;
        }
    }

    return 1;
}



int validar_Port(int Port)
{
    if (Port >= 1 && Port <= 65535) {
        return 1;
    }

    return 0;
}



int main(int argc, char *argv[])
{

    int peerport = -1;

    char *dsip = DEFAULT_DSIP;

    int dsport = DEFAULT_DSPORT;

    if (parse_arguments(argc,
                        argv,
                        &peerport,
                        &dsip,
                        &dsport) == 0)
    {
        return 1;
    }


    if (!validar_Port(peerport)) {
        printf("Erro: peerport inválida.\n");
        return 1;
    }


    if (!validar_Port(dsport)) {
        printf("Erro: DSport inválida.\n");
        return 1;
    }

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


    if (inet_pton(AF_INET,
                  dsip,
                  &server_addr.sin_addr) <= 0)
    {
        printf("Erro: endereço IP inválido.\n");

        close(sockfd);

        return 1;
    }

    printf("Endereço do DS configurado com sucesso.\n");


    int logged_in = 0;

    char current_uid[20] = "";
    char current_password[20] = "";
    char buffer[256];

    while (1) {

        printf("> ");

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }


        /* Remover '\n' */
        buffer[strcspn(buffer, "\n")] = '\0';


        if (strlen(buffer) == 0) {
            continue;
        }


        if (strcmp(buffer, "exit") == 0) {

            if (logged_in) {

                printf("Please logout before exiting.\n");

            } else {

                break;
            }

            continue;
        }


        int fields;

        char command[20];
        char uid[20];
        char password[20];
        char extra[20];

        command[0] = '\0';
        uid[0] = '\0';
        password[0] = '\0';
        extra[0] = '\0';


        fields = sscanf(buffer,
                        "%19s %19s %19s %19s",
                        command,
                        uid,
                        password,
                        extra);




        if (strcmp(command, "login") == 0) {


            if (fields != 3) {

                printf("Usage: login <UID> <Password>\n");

                continue;
            }


            /* Validar UID */

            if (!validar_UID(uid)) {

                printf("Invalid UID.\n");

                continue;
            }


            /* Validar password */

            if (!validar_Password(password)) {

                printf("Invalid password.\n");

                continue;
            }


            /* Enviar login para o DS */

            if (login(sockfd,
                      &server_addr,
                      uid,
                      password,
                      peerport))
            {

                logged_in = 1;

                strcpy(current_uid, uid);

                strcpy(current_password, password);
            }
        }


        else if (strcmp(command, "logout") == 0) {

            if (fields != 1) {

                printf("Usage: logout\n");

                continue;
            }


            if (logged_in) {

                if (logout(sockfd,
                           &server_addr,
                           current_uid,
                           current_password))
                {
                    logged_in = 0;

                    current_uid[0] = '\0';

                    current_password[0] = '\0';
                }

            } else {

                printf("User not logged in.\n");
            }
        }


        else if (strcmp(command, "unregister") == 0) {

            if (fields != 1) {

                printf("Usage: unregister\n");

                continue;
            }


            if (logged_in) {

                if (unregister_user(sockfd,
                                    &server_addr,
                                    current_uid,
                                    current_password))
                {
                    logged_in = 0;

                    current_uid[0] = '\0';

                    current_password[0] = '\0';
                }

            } else {

                printf("User not logged in.\n");
            }
        }


        else {

            printf("Unknown command.\n");
        }
    }

    close(sockfd);

    return 0;
}
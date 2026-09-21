#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "common.h"
#include "args.h"
#include "validation.h"
#include "ds_protocol.h"

int main(int argc, char *argv[])
{
    int peerport = -1;
    char *dsip = DEFAULT_DSIP;
    int dsport = DEFAULT_DSPORT;

    if (parse_arguments(argc, argv, &peerport, &dsip, &dsport) == 0) {
        return 1;
    }

    if (!validar_Port(peerport)) {
        printf("Error: Invalid peerport.\n");
        return 1;
    }

    if (!validar_Port(dsport)) {
        printf("Error: Invalid DSport.\n");
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    /* Timeout no recvfrom, para não bloquear se o DS não responder */
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dsport);

    if (inet_pton(AF_INET, dsip, &server_addr.sin_addr) <= 0) {
        printf("Error: Invalid IP adress.\n");
        close(sockfd);
        return 1;
    }

    int logged_in = 0;
    char current_uid[20] = "";
    char current_password[20] = "";
    char buffer[INPUT_BUF_SIZE];

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

            if (logged_in) {
                printf("Logout before another login.\n");
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
            cmd_result_t result = login(sockfd, &server_addr, uid, password, peerport);

            if (result == CMD_OK) {
                logged_in = 1;
                strcpy(current_uid, uid);
                strcpy(current_password, password);

            } else if (result == CMD_COMM_ERROR) {
                printf("Communication error during login.\n");
            }
        }

        else if (strcmp(command, "logout") == 0) {

            if (fields != 1) {
                printf("Usage: logout\n");
                continue;
            }

            if (logged_in) {
                cmd_result_t result = logout(sockfd,
                                              &server_addr,
                                              current_uid,
                                              current_password);

                if (result == CMD_OK || result == CMD_NO_SESSION) {
                    logged_in = 0;
                    current_uid[0] = '\0';
                    current_password[0] = '\0';

                } else if (result == CMD_COMM_ERROR) {
                    printf("Communication error during logout.\n");
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
                cmd_result_t result = unregister_user(sockfd,
                                                        &server_addr,
                                                        current_uid,
                                                        current_password);

                if (result == CMD_OK || result == CMD_NO_SESSION) {
                    logged_in = 0;
                    current_uid[0] = '\0';
                    current_password[0] = '\0';

                } else if (result == CMD_COMM_ERROR) {
                    printf("Communication error during unregister.\n");
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

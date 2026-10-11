#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

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
#include "ds_tcp.h"
#include "signals.h"

/* Resultados de read_command_line() */
#define READ_OK        1
#define READ_END       0     /* EOF (Ctrl+D) ou Ctrl+C */
#define READ_TOO_LONG -1     /* linha maior que o buffer (já descartada) */
#define READ_RETRY    -2     /* interrompido por um sinal sem importância */

/* Termina a sessão local (depois de logout/unregister bem sucedidos ou de
 * o DS indicar que não há sessão). */
static void clear_session(int *logged_in, char *uid, char *password)
{
    *logged_in = 0;
    uid[0] = '\0';
    password[0] = '\0';
}

/* Lê uma linha do teclado para buf, já sem o '\n'.
 * Se a linha não couber no buffer, o resto é descartado, para não ser
 * interpretado como um comando seguinte. */
static int read_command_line(char *buf, size_t size)
{
    errno = 0;

    if (fgets(buf, (int)size, stdin) == NULL) {
        if (errno == EINTR && !sigint_received()) {
            clearerr(stdin);
            return READ_RETRY;
        }
        return READ_END;
    }

    size_t len = strlen(buf);

    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        return READ_OK;
    }

    if (feof(stdin)) {                  /* última linha sem '\n' */
        return READ_OK;
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* descartar o resto da linha */
    }
    return READ_TOO_LONG;
}

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

    setup_signals();

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    /* Timeout no recvfrom, para não bloquear se o DS não responder */
    struct timeval tv;
    tv.tv_sec = UDP_TIMEOUT_SEC;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)dsport);

    if (inet_pton(AF_INET, dsip, &server_addr.sin_addr) <= 0) {
        printf("Error: Invalid IP address.\n");
        close(sockfd);
        return 1;
    }

    int logged_in = 0;
    char current_uid[20] = "";
    char current_password[20] = "";
    char buffer[INPUT_BUF_SIZE];

    while (!sigint_received()) {

        printf("> ");
        fflush(stdout);

        int rl = read_command_line(buffer, sizeof(buffer));

        if (rl == READ_END) {
            break;
        }
        if (rl == READ_RETRY) {
            continue;
        }
        if (rl == READ_TOO_LONG) {
            printf("Input line too long.\n");
            continue;
        }

        int fields;

        char command[TOKEN_SIZE];
        char arg1[TOKEN_SIZE];      /* UID  | filename */
        char arg2[TOKEN_SIZE];      /* password | label */
        char extra[TOKEN_SIZE];

        command[0] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';
        extra[0] = '\0';

        fields = sscanf(buffer,
                        "%63s %63s %63s %63s",
                        command,
                        arg1,
                        arg2,
                        extra);

        if (fields <= 0) {          /* linha vazia ou só com espaços */
            continue;
        }

        if (strcmp(command, "exit") == 0) {

            if (fields != 1) {
                printf("Usage: exit\n");
                continue;
            }

            if (logged_in) {
                printf("Please logout before exiting.\n");
                continue;
            }

            break;
        }

        else if (strcmp(command, "login") == 0) {

            if (fields != 3) {
                printf("Usage: login <UID> <Password>\n");
                continue;
            }

            if (logged_in) {
                printf("Logout before another login.\n");
                continue;
            }

            if (!validar_UID(arg1)) {
                printf("Invalid UID.\n");
                continue;
            }

            if (!validar_Password(arg2)) {
                printf("Invalid password.\n");
                continue;
            }

            cmd_result_t result = login(sockfd, &server_addr, arg1, arg2, peerport);

            if (result == CMD_OK) {
                logged_in = 1;
                strcpy(current_uid, arg1);
                strcpy(current_password, arg2);

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
                    clear_session(&logged_in, current_uid, current_password);

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
                    clear_session(&logged_in, current_uid, current_password);

                } else if (result == CMD_COMM_ERROR) {
                    printf("Communication error during unregister.\n");
                }

            } else {
                printf("User not logged in.\n");
            }
        }

        else if (strcmp(command, "publish") == 0) {

            if (fields != 3) {
                printf("Usage: publish <filename> <label>\n");
                continue;
            }

            if (!logged_in) {
                printf("User not logged in.\n");
                continue;
            }

            if (!validar_Filename(arg1)) {
                printf("Invalid filename (max 24 chars, format name.xxx).\n");
                continue;
            }

            if (!validar_Label(arg2)) {
                printf("Invalid label (1-20 chars: letters, digits, - and _).\n");
                continue;
            }

            cmd_result_t result = publish_file(sockfd,
                                                &server_addr,
                                                current_uid,
                                                current_password,
                                                arg1,
                                                arg2);

            if (result == CMD_NO_SESSION) {
                clear_session(&logged_in, current_uid, current_password);

            } else if (result == CMD_COMM_ERROR) {
                printf("Communication error during publish.\n");
            }
        }

        else if (strcmp(command, "remove") == 0) {

            if (fields != 2) {
                printf("Usage: remove <filename>\n");
                continue;
            }

            if (!logged_in) {
                printf("User not logged in.\n");
                continue;
            }

            if (!validar_Filename(arg1)) {
                printf("Invalid filename (max 24 chars, format name.xxx).\n");
                continue;
            }

            cmd_result_t result = remove_file(sockfd,
                                               &server_addr,
                                               current_uid,
                                               current_password,
                                               arg1);

            if (result == CMD_NO_SESSION) {
                clear_session(&logged_in, current_uid, current_password);

            } else if (result == CMD_COMM_ERROR) {
                printf("Communication error during remove.\n");
            }
        }

        else if (strcmp(command, "list") == 0) {

            if (fields != 1) {
                printf("Usage: list\n");
                continue;
            }

            /* Não requer login */
            cmd_result_t result = list_files(sockfd, &server_addr);

            if (result == CMD_COMM_ERROR) {
                printf("Communication error during list.\n");
            }
        }

        else if (strcmp(command, "versions") == 0) {

            if (fields != 2) {
                printf("Usage: versions <filename>\n");
                continue;
            }

            if (!validar_Filename(arg1)) {
                printf("Invalid filename (max 24 chars, format name.xxx).\n");
                continue;
            }

            /* O pedido VRS não leva UID, por isso não requer login */
            cmd_result_t result = versions_file(&server_addr, arg1);

            if (result == CMD_COMM_ERROR) {
                printf("Communication error during versions.\n");
            }
        }

        else {
            printf("Unknown command.\n");
        }
    }

    if (sigint_received()) {
        printf("\n");
    }

    if (logged_in) {
        ignore_sigint();
        logout(sockfd, &server_addr, current_uid, current_password);
    }

    close(sockfd);

    return 0;
}
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "ds_protocol.h"
#include "common.h"

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
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("No response from server (timeout).\n");
        } else {
            perror("recvfrom");
        }
        return -1;
    }

    response[bytes_recvd] = '\0';

    return 0;
}


cmd_result_t login(int sockfd,
                    struct sockaddr_in *server_addr,
                    const char *uid,
                    const char *password,
                    int peerport)
{
    char message[MSG_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];

    snprintf(message, sizeof(message),
             "LIN %s %s %d\n",
             uid,
             password,
             peerport);

    if (communicate(sockfd, server_addr, message, response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (sscanf(response, "RLI %9s", status) != 1) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Successful login.\n");
        return CMD_OK;
    } else if (strcmp(status, "REG") == 0) {
        printf("New user registered.\n");
        return CMD_OK;
    } else if (strcmp(status, "NOK") == 0) {
        printf("Incorrect login attempt.\n");
        return CMD_REJECTED;
    } else if (strcmp(status, "ERR") == 0) {
        printf("Login request error.\n");
        return CMD_REJECTED;
    }

    printf("Unexpected login response.\n");
    return CMD_REJECTED;
}


cmd_result_t logout(int sockfd,
                     struct sockaddr_in *server_addr,
                     const char *uid,
                     const char *password)
{
    char message[MSG_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];

    snprintf(message, sizeof(message),
             "LOU %s %s\n",
             uid,
             password);

    if (communicate(sockfd, server_addr, message, response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];

    if (sscanf(response, "RLO %9s", status) != 1) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Successful logout.\n");
        return CMD_OK;

    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in.\n");
        return CMD_NO_SESSION;

    } else if (strcmp(status, "UNR") == 0) {
        printf("Unknown user.\n");
        return CMD_NO_SESSION;

    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password.\n");
        return CMD_REJECTED;

    } else if (strcmp(status, "ERR") == 0) {
        printf("Logout request error.\n");
        return CMD_REJECTED;
    }

    printf("Unexpected logout response.\n");
    return CMD_REJECTED;
}


cmd_result_t unregister_user(int sockfd,
                              struct sockaddr_in *server_addr,
                              const char *uid,
                              const char *password)
{
    char message[MSG_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];

    snprintf(message, sizeof(message),
             "UNR %s %s\n",
             uid,
             password);

    if (communicate(sockfd, server_addr, message, response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (sscanf(response, "RUR %9s", status) != 1) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Successful unregister.\n");
        return CMD_OK;
    } else if (strcmp(status, "NOK") == 0) {
        printf("User not logged in.\n");
        return CMD_NO_SESSION;
    } else if (strcmp(status, "UNR") == 0) {
        printf("Unknown user.\n");
        return CMD_NO_SESSION;
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password.\n");
        return CMD_REJECTED;
    } else if (strcmp(status, "ERR") == 0) {
        printf("Unregister request error.\n");
        return CMD_REJECTED;
    }

    printf("Unexpected unregister response.\n");
    return CMD_REJECTED;
}

cmd_result_t publish_file(int sockfd,
                           struct sockaddr_in *server_addr,
                           const char *uid,
                           const char *password,
                           const char *filename,
                           const char *label)
{
    struct stat st;

    /* Validação da existencia do ficheiro */
    if (stat(filename, &st) != 0 || !S_ISREG(st.st_mode)) {
        printf("File '%s' does not exist in the local directory.\n", filename);
        return CMD_REJECTED;
    }

    if ((long long)st.st_size > MAX_FSIZE) {
        printf("File too large (maximum is %lld bytes).\n", MAX_FSIZE);
        return CMD_REJECTED;
    }

    char message[MSG_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];

    snprintf(message, sizeof(message),
             "PUB %s %s %s %lld %s\n",
             uid,
             password,
             filename,
             (long long)st.st_size,
             label);

    if (communicate(sockfd, server_addr, message, response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (sscanf(response, "RPB %9s", status) != 1) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Successful publication of '%s'.\n", filename);
        return CMD_OK;
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in.\n");
        return CMD_NO_SESSION;
    } else if (strcmp(status, "UNR") == 0) {
        printf("Unknown user.\n");
        return CMD_NO_SESSION;
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password.\n");
        return CMD_REJECTED;
    } else if (strcmp(status, "NOK") == 0) {
        printf("Unsuccessful publication.\n");
        return CMD_REJECTED;
    } else if (strcmp(status, "ERR") == 0) {
        printf("Publish request error.\n");
        return CMD_REJECTED;
    }

    printf("Unexpected publish response.\n");
    return CMD_REJECTED;
}


cmd_result_t remove_file(int sockfd,
                          struct sockaddr_in *server_addr,
                          const char *uid,
                          const char *password,
                          const char *filename)
{
    char message[MSG_BUF_SIZE];
    char response[RESPONSE_BUF_SIZE];

    snprintf(message, sizeof(message),
             "REM %s %s %s\n",
             uid,
             password,
             filename);

    if (communicate(sockfd, server_addr, message, response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (sscanf(response, "RRM %9s", status) != 1) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }

    if (strcmp(status, "OK") == 0) {
        printf("Successful removal of '%s'.\n", filename);
        return CMD_OK;
    } else if (strcmp(status, "NLG") == 0) {
        printf("User not logged in.\n");
        return CMD_NO_SESSION;
    } else if (strcmp(status, "UNR") == 0) {
        printf("Unknown user.\n");
        return CMD_NO_SESSION;
    } else if (strcmp(status, "WRP") == 0) {
        printf("Wrong password.\n");
        return CMD_REJECTED;
    } else if (strcmp(status, "NOK") == 0) {
        printf("Resource not found (not published by this user).\n");
        return CMD_REJECTED;
    } else if (strcmp(status, "ERR") == 0) {
        printf("Remove request error.\n");
        return CMD_REJECTED;
    }

    printf("Unexpected remove response.\n");
    return CMD_REJECTED;
}


cmd_result_t list_files(int sockfd, struct sockaddr_in *server_addr)
{
    char response[LIST_RESP_SIZE];
 
    if (communicate(sockfd, server_addr, "LST\n", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;
 
    /* A mensagem tem de terminar em '\n' */
    size_t len = strlen(response);
    if (len == 0 || response[len - 1] != '\n') {
        printf("Malformed message received from DS.\n");
        return CMD_REJECTED;
    }
    response[len - 1] = '\0';
 
    /* Prefixo "RLS " */
    if (strncmp(response, "RLS ", 4) != 0) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }
 
    char *p = response + 4;         /* ponteiro de leitura, depois de "RLS " */
    int consumed = 0;
    char status[10];
 
    if (sscanf(p, "%9s%n", status, &consumed) != 1) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }
    p += consumed;
 
    if (strcmp(status, "NOK") == 0) {
        printf("No resources currently available.\n");
        return CMD_OK;
    }
 
    if (strcmp(status, "ERR") == 0) {
        printf("List request error.\n");
        return CMD_REJECTED;
    }
 
    if (strcmp(status, "OK") != 0) {
        printf("Unexpected list response.\n");
        return CMD_REJECTED;
    }
 
    /* Ler todos os filenames para um array antes de mostrar nada, para não
     * imprimir uma lista parcial se a mensagem estiver mal formada. */
    char names[MAX_LIST_ENTRIES][LIST_TOKEN_SIZE];
    char name[LIST_TOKEN_SIZE];
    int count = 0;
 
    while (sscanf(p, "%31s%n", name, &consumed) == 1) {
 
        if (count >= MAX_LIST_ENTRIES || strlen(name) > FILENAME_MAX_LEN) {
            printf("Malformed message received from DS.\n");
            return CMD_REJECTED;
        }
 
        strcpy(names[count], name);
        count++;
        p += consumed;
    }
 
    if (count == 0) {
        printf("No resources currently available.\n");
        return CMD_OK;
    }
 
    printf("Available resources:\n");
    for (int i = 0; i < count; i++) {
        printf("  %2d. %s\n", i + 1, names[i]);
    }
 
    return CMD_OK;
}

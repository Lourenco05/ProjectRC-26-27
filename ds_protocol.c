#include <stdio.h>
#include <string.h>
#include <errno.h>
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

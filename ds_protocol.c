#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "ds_protocol.h"
#include "common.h"
#include "parse.h"
#include "signals.h"
#include "validation.h"

/* Descarta datagramas que estejam à espera no socket (por exemplo, uma
 * resposta que chegou depois do timeout do comando anterior). */
static void drain_socket(int sockfd)
{
    char junk[RESPONSE_BUF_SIZE];

    while (recvfrom(sockfd, junk, sizeof(junk), MSG_DONTWAIT, NULL, NULL) >= 0) {
        /* ignorar */
    }
}

/* O datagrama veio do DS para o qual enviámos o pedido? */
static int is_from_server(const struct sockaddr_in *sender,
                          const struct sockaddr_in *server)
{
    return sender->sin_addr.s_addr == server->sin_addr.s_addr &&
           sender->sin_port == server->sin_port;
}

/* A resposta é do tipo esperado ("<tag> ..." ou "<tag>\n") ou um "ERR\n"? */
static int is_expected_reply(const char *resp, const char *tag)
{
    size_t tag_len = strlen(tag);

    if (strncmp(resp, tag, tag_len) == 0 &&
        (resp[tag_len] == ' ' || resp[tag_len] == '\n')) {
        return 1;
    }

    return strcmp(resp, "ERR\n") == 0;
}

/* Valida estritamente uma resposta "<tag> <status>\n" e copia o status.
 * Aceita também "ERR\n" (resposta a mensagens desconhecidas/mal formadas).
 * Devolve 0 se estiver correta e -1 caso contrário (sem '\n' final, espaços
 * a mais, campos a mais ou a menos, tag errada). */
static int parse_status_reply(const char *response, const char *tag,
                              char *status, size_t status_size)
{
    char line[RESPONSE_BUF_SIZE];
    char *tok[3];
    size_t len = strlen(response);
    int n;

    if (len == 0 || response[len - 1] != '\n' || len >= sizeof(line)) {
        return -1;
    }

    memcpy(line, response, len - 1);
    line[len - 1] = '\0';

    n = split_tokens(line, tok, 3);

    if (n == 1 && strcmp(tok[0], "ERR") == 0) {
        snprintf(status, status_size, "ERR");
        return 0;
    }

    if (n != 2 || strcmp(tok[0], tag) != 0 || strlen(tok[1]) >= status_size) {
        return -1;
    }

    strcpy(status, tok[1]);
    return 0;
}


int communicate(int sockfd,
                struct sockaddr_in *server_addr,
                const char *msg,
                const char *expected_tag,
                char *response,
                size_t resp_len)
{
    socklen_t addr_len = sizeof(*server_addr);

    drain_socket(sockfd);

    for (int attempt = 1; attempt <= UDP_MAX_ATTEMPTS; attempt++) {

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

        /* Esperar pela resposta (ignorando lixo) até ao timeout */
        int discarded = 0;

        while (discarded <= UDP_MAX_DISCARDS) {

            struct sockaddr_in sender_addr;
            socklen_t sender_len = sizeof(sender_addr);

            ssize_t bytes_recvd = recvfrom(sockfd,
                                           response,
                                           resp_len - 1,
                                           0,
                                           (struct sockaddr *)&sender_addr,
                                           &sender_len);

            if (bytes_recvd < 0) {
                if (errno == EINTR) {
                    if (sigint_received()) {
                        return -1;          /* Ctrl+C: não insistir */
                    }
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;                  /* timeout: tentar de novo */
                }
                perror("recvfrom");
                return -1;
            }

            response[bytes_recvd] = '\0';

            if (strlen(response) != (size_t)bytes_recvd ||
                !is_from_server(&sender_addr, server_addr) ||
                !is_expected_reply(response, expected_tag)) {
                discarded++;
                continue;
            }

            return 0;
        }

        if (attempt < UDP_MAX_ATTEMPTS) {
            printf("No response from DS, retrying (%d/%d)...\n",
                   attempt + 1, UDP_MAX_ATTEMPTS);
        }
    }

    printf("No response from server (timeout).\n");
    return -1;
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

    if (communicate(sockfd, server_addr, message, "RLI", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (parse_status_reply(response, "RLI", status, sizeof(status)) == -1) {
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

    if (communicate(sockfd, server_addr, message, "RLO", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];

    if (parse_status_reply(response, "RLO", status, sizeof(status)) == -1) {
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

    if (communicate(sockfd, server_addr, message, "RUR", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (parse_status_reply(response, "RUR", status, sizeof(status)) == -1) {
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

    /* O ficheiro tem de poder ser lido (vai ser enviado a outros peers) */
    if (access(filename, R_OK) != 0) {
        printf("File '%s' is not readable.\n", filename);
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

    if (communicate(sockfd, server_addr, message, "RPB", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (parse_status_reply(response, "RPB", status, sizeof(status)) == -1) {
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

    if (communicate(sockfd, server_addr, message, "RRM", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    char status[10];
    if (parse_status_reply(response, "RRM", status, sizeof(status)) == -1) {
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

    if (communicate(sockfd, server_addr, "LST\n", "RLS", response, sizeof(response)) == -1)
        return CMD_COMM_ERROR;

    /* A mensagem tem de terminar em '\n' */
    size_t len = strlen(response);
    if (len == 0 || response[len - 1] != '\n') {
        printf("Malformed message received from DS.\n");
        return CMD_REJECTED;
    }
    response[len - 1] = '\0';

    /* RLS + status + até MAX_LIST_ENTRIES nomes (+1 para detetar excesso) */
    char *tok[MAX_LIST_ENTRIES + 3];
    int n = split_tokens(response, tok, MAX_LIST_ENTRIES + 3);

    if (n < 0) {
        /* espaços a mais, tokens vazios ou mais de 50 nomes */
        printf("Malformed message received from DS.\n");
        return CMD_REJECTED;
    }

    if (n == 1 && strcmp(tok[0], "ERR") == 0) {
        printf("List request error.\n");
        return CMD_REJECTED;
    }

    if (n < 2 || strcmp(tok[0], "RLS") != 0) {
        printf("Unexpected message received from DS.\n");
        return CMD_REJECTED;
    }

    const char *status = tok[1];

    if (strcmp(status, "NOK") == 0) {
        if (n != 2) {
            printf("Malformed message received from DS.\n");
            return CMD_REJECTED;
        }
        printf("No resources currently available.\n");
        return CMD_OK;
    }

    if (strcmp(status, "ERR") == 0) {
        if (n != 2) {
            printf("Malformed message received from DS.\n");
            return CMD_REJECTED;
        }
        printf("List request error.\n");
        return CMD_REJECTED;
    }

    if (strcmp(status, "OK") != 0) {
        printf("Unexpected list response.\n");
        return CMD_REJECTED;
    }

    /* Validar todos os filenames antes de mostrar nada, para não imprimir
     * uma lista parcial se a mensagem estiver mal formada. */
    int count = n - 2;

    if (count > MAX_LIST_ENTRIES) {
        printf("Malformed message received from DS.\n");
        return CMD_REJECTED;
    }

    for (int i = 0; i < count; i++) {
        if (!validar_Filename(tok[i + 2])) {
            printf("Malformed message received from DS.\n");
            return CMD_REJECTED;
        }
    }

    if (count == 0) {
        printf("No resources currently available.\n");
        return CMD_OK;
    }

    printf("Available resources:\n");
    for (int i = 0; i < count; i++) {
        printf("  %2d. %s\n", i + 1, tok[i + 2]);
    }

    return CMD_OK;
}
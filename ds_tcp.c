#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "ds_tcp.h"
#include "common.h"
#include "parse.h"
#include "signals.h"
#include "validation.h"


/* Uma linha da resposta RVR: UID Fsize label publication_time availability */
typedef struct {
    char uid[UID_LEN + 1];
    char fsize[9];                          /* até 8 dígitos */
    char label[LABEL_MAX_LEN + 1];
    char time[VERSIONS_TIME_SIZE];          /* publication_time (1 ou 2 tokens) */
    int  available;                         /* 1 = AVL, 0 = NAV */
} version_entry_t;


/* ------------------------------------------------------------------ */
/* Funções auxiliares de I/O                                           */
/* ------------------------------------------------------------------ */

/* write() pode escrever menos bytes do que pedido: repetir até acabar. */
static int write_all(int fd, const char *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = write(fd, buf + sent, len - sent);

        if (n < 0) {
            if (errno == EINTR) {
                if (sigint_received()) {
                    return -1;
                }
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Timeout sending request to DS.\n");
            } else {
                perror("write");
            }
            return -1;
        }
        if (n == 0) {
            return -1;
        }

        sent += (size_t)n;
    }

    return 0;
}

/* Lê do socket até encontrar '\n' (a resposta pode chegar em vários
 * read()). Devolve em *out um buffer malloc'ed com a linha (incluindo o
 * '\n', terminada em '\0').
 * Devolve 0 em sucesso, -1 em erro de comunicação (timeout, erro de read) e
 * -2 se a resposta for inválida (fecho da ligação antes do '\n', resposta
 * maior que VRS_MAX_RESP, bytes depois do '\n' ou caracteres NUL). */
static int read_reply(int fd, char **out)
{
    size_t cap = 1024;
    size_t len = 0;
    char *buf = malloc(cap);

    if (buf == NULL) {
        perror("malloc");
        return -1;
    }

    for (;;) {
        if (len + 1 >= cap) {
            if (cap >= VRS_MAX_RESP) {
                free(buf);
                return -2;
            }
            cap *= 2;
            char *bigger = realloc(buf, cap);
            if (bigger == NULL) {
                perror("realloc");
                free(buf);
                return -1;
            }
            buf = bigger;
        }

        ssize_t n = read(fd, buf + len, cap - len - 1);

        if (n < 0) {
            if (errno == EINTR) {
                if (sigint_received()) {
                    free(buf);
                    return -1;
                }
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("No response from server (timeout).\n");
            } else {
                perror("read");
            }
            free(buf);
            return -1;
        }

        if (n == 0) {                       /* DS fechou sem enviar '\n' */
            free(buf);
            return -2;
        }

        char *nl = memchr(buf + len, '\n', (size_t)n);
        len += (size_t)n;

        if (nl != NULL) {
            buf[len] = '\0';

            /* o '\n' tem de ser o último byte e não pode haver NULs */
            if ((size_t)(nl - buf) + 1 != len || strlen(buf) != len) {
                free(buf);
                return -2;
            }

            *out = buf;
            return 0;
        }
    }
}


/* ------------------------------------------------------------------ */
/* Parsing da resposta RVR                                             */
/* ------------------------------------------------------------------ */

/* 1 se for "AVL", 0 se for "NAV", -1 caso contrário */
static int availability_of(const char *token)
{
    if (strcmp(token, "AVL") == 0) return 1;
    if (strcmp(token, "NAV") == 0) return 0;
    return -1;
}

/* Um token de data/hora: 1 a VERSIONS_TIME_TOKEN_MAX caracteres imprimíveis */
static int valid_time_token(const char *token)
{
    size_t n = strlen(token);

    if (n < 1 || n > VERSIONS_TIME_TOKEN_MAX) {
        return 0;
    }
    for (size_t i = 0; i < n; i++) {
        if ((unsigned char)token[i] < 33 || (unsigned char)token[i] > 126) {
            return 0;
        }
    }
    return 1;
}

/* Converte os tokens a partir do índice 2 em entradas. Cada entrada tem
 *   UID Fsize label publication_time availability
 * em que publication_time tem 1 token (ex.: 2026-10-10T12:00:00) ou 2 tokens
 * (ex.: "2026-10-10 12:00:00"): o fim da data é sempre marcado por AVL/NAV.
 * Devolve 0 e preenche *entries (malloc) / *count, ou -1 se qualquer entrada
 * for inválida (nesse caso nada é devolvido). */
static int parse_entries(char *tok[], int ntok,
                         version_entry_t **entries, int *count)
{
    int max_entries = (ntok - 2) / 5 + 1;           /* mínimo de 5 tokens por entrada */
    version_entry_t *list = calloc((size_t)max_entries, sizeof(*list));
    int n = 0;
    int i = 2;

    if (list == NULL) {
        perror("calloc");
        return -1;
    }

    while (i < ntok) {
        int time_tokens;
        int avail;

        if (i + 4 >= ntok || n >= max_entries) {
            goto malformed;
        }

        if (!validar_UID(tok[i]) ||
            !validar_Fsize(tok[i + 1]) ||
            !validar_Label(tok[i + 2]) ||
            !valid_time_token(tok[i + 3])) {
            goto malformed;
        }

        if ((avail = availability_of(tok[i + 4])) != -1) {
            time_tokens = 1;
        } else if (i + 5 < ntok &&
                   valid_time_token(tok[i + 4]) &&
                   (avail = availability_of(tok[i + 5])) != -1) {
            time_tokens = 2;
        } else {
            goto malformed;
        }

        snprintf(list[n].uid, sizeof(list[n].uid), "%s", tok[i]);
        snprintf(list[n].fsize, sizeof(list[n].fsize), "%s", tok[i + 1]);
        snprintf(list[n].label, sizeof(list[n].label), "%s", tok[i + 2]);

        if (time_tokens == 1) {
            snprintf(list[n].time, sizeof(list[n].time), "%s", tok[i + 3]);
        } else {
            snprintf(list[n].time, sizeof(list[n].time), "%s %s",
                     tok[i + 3], tok[i + 4]);
        }
        list[n].available = avail;

        n++;
        i += 3 + time_tokens + 1;
    }

    *entries = list;
    *count = n;
    return 0;

malformed:
    free(list);
    return -1;
}


/* ------------------------------------------------------------------ */
/* Comando versions                                                    */
/* ------------------------------------------------------------------ */

cmd_result_t versions_file(struct sockaddr_in *server_addr,
                            const char *filename)
{
    cmd_result_t result = CMD_REJECTED;
    char message[MSG_BUF_SIZE];
    char *reply = NULL;
    char **tok = NULL;
    version_entry_t *entries = NULL;
    int count = 0;
    int ntok_max = 1;
    int ntok;
    int rc;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return CMD_COMM_ERROR;
    }

    /* Timeouts, para nunca ficar bloqueado se o DS não responder */
    struct timeval tv;
    tv.tv_sec = TCP_TIMEOUT_SEC;
    tv.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        perror("setsockopt");
        close(fd);
        return CMD_COMM_ERROR;
    }

    if (connect(fd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == ETIMEDOUT) {
            printf("Could not connect to DS (timeout).\n");
        } else {
            perror("connect");
        }
        close(fd);
        return CMD_COMM_ERROR;
    }

    snprintf(message, sizeof(message), "VRS %s\n", filename);

    if (write_all(fd, message, strlen(message)) == -1) {
        close(fd);
        return CMD_COMM_ERROR;
    }

    rc = read_reply(fd, &reply);
    close(fd);

    if (rc == -1) {
        return CMD_COMM_ERROR;
    }
    if (rc == -2) {
        printf("Malformed message received from DS.\n");
        return CMD_REJECTED;
    }

    /* Separar a resposta em tokens (alocar o máximo possível: nº de espaços + 1) */
    reply[strlen(reply) - 1] = '\0';                /* retirar o '\n' */

    for (const char *c = reply; *c != '\0'; c++) {
        if (*c == ' ') {
            ntok_max++;
        }
    }

    tok = malloc((size_t)ntok_max * sizeof(*tok));
    if (tok == NULL) {
        perror("malloc");
        free(reply);
        return CMD_COMM_ERROR;
    }

    ntok = split_tokens(reply, tok, ntok_max);

    if (ntok < 0) {
        printf("Malformed message received from DS.\n");
        goto done;
    }

    if (ntok == 1 && strcmp(tok[0], "ERR") == 0) {
        printf("Versions request error.\n");
        goto done;
    }

    if (ntok < 2 || strcmp(tok[0], "RVR") != 0) {
        printf("Unexpected message received from DS.\n");
        goto done;
    }

    if (strcmp(tok[1], "NOK") == 0) {
        if (ntok != 2) {
            printf("Malformed message received from DS.\n");
        } else {
            printf("No versions of '%s' are currently available.\n", filename);
            result = CMD_OK;
        }
        goto done;
    }

    if (strcmp(tok[1], "ERR") == 0) {
        if (ntok != 2) {
            printf("Malformed message received from DS.\n");
        } else {
            printf("Versions request error.\n");
        }
        goto done;
    }

    if (strcmp(tok[1], "OK") != 0) {
        printf("Unexpected versions response.\n");
        goto done;
    }

    if (ntok == 2) {                                /* "RVR OK" sem entradas */
        printf("No versions of '%s' are currently available.\n", filename);
        result = CMD_OK;
        goto done;
    }

    if (parse_entries(tok, ntok, &entries, &count) == -1) {
        printf("Malformed message received from DS.\n");
        goto done;
    }

    printf("Versions of '%s' (%d found):\n", filename, count);
    printf("  %-6s  %9s  %-*s  %-19s  %s\n",
           "UID", "SIZE (B)", LABEL_MAX_LEN, "LABEL", "PUBLISHED", "STATUS");

    for (int i = 0; i < count; i++) {
        printf("  %-6s  %9s  %-*s  %-19s  %s\n",
               entries[i].uid,
               entries[i].fsize,
               LABEL_MAX_LEN, entries[i].label,
               entries[i].time,
               entries[i].available ? "available" : "not available");
    }

    result = CMD_OK;

done:
    free(entries);
    free(tok);
    free(reply);
    return result;
}
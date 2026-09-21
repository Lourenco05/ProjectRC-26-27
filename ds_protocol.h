#ifndef DS_PROTOCOL_H
#define DS_PROTOCOL_H

#include <stddef.h>
#include <netinet/in.h>

typedef enum {
    CMD_OK,          /* sucesso confirmado pelo DS */
    CMD_NO_SESSION,  /* DS confirma que não há sessão ativa */
    CMD_REJECTED,    /* pedido recusado por outro motivo (WRP, ERR, resposta inesperada) */
    CMD_COMM_ERROR   /* falha de comunicação (timeout, sendto/recvfrom) */
} cmd_result_t;

/* Envia msg ao DS através de sockfd e aguarda a resposta.
 * Devolve 0 em caso de sucesso, -1 em caso de erro de comunicação.*/
int communicate(int sockfd,
                struct sockaddr_in *server_addr,
                const char *msg,
                char *response,
                size_t resp_len);

/* Envia um pedido LIN ao DS e interpreta a resposta RLI */
cmd_result_t login(int sockfd,
                    struct sockaddr_in *server_addr,
                    const char *uid,
                    const char *password,
                    int peerport);

/* Envia um pedido LOU ao DS e interpreta a resposta RLO */
cmd_result_t logout(int sockfd,
                     struct sockaddr_in *server_addr,
                     const char *uid,
                     const char *password);

/* Envia um pedido UNR ao DS e interpreta a resposta RUR */
cmd_result_t unregister_user(int sockfd,
                              struct sockaddr_in *server_addr,
                              const char *uid,
                              const char *password);

#endif /* DS_PROTOCOL_H */

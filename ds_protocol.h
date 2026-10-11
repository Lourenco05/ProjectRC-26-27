#ifndef DS_PROTOCOL_H
#define DS_PROTOCOL_H

#include <stddef.h>
#include <netinet/in.h>

typedef enum {
    CMD_OK,          /* sucesso confirmado pelo DS */
    CMD_NO_SESSION,  /* DS confirma que não há sessão ativa */
    CMD_REJECTED,    /* pedido recusado por outro motivo (WRP, ERR, resposta inesperada) */
    CMD_COMM_ERROR   /* falha de comunicação (timeout, sendto/recvfrom, connect, ...) */
} cmd_result_t;

/* Envia msg ao DS através de sockfd (UDP) e aguarda a resposta.
 *  - Reenvia o pedido até UDP_MAX_ATTEMPTS vezes se não houver resposta.
 *  - Ignora datagramas que não venham do DS ou que não sejam do tipo esperado
 *    (expected_tag, por exemplo "RLI"; "ERR\n" é sempre aceite), evitando
 *    confundir respostas atrasadas de pedidos anteriores com a resposta atual.
 * Devolve 0 em caso de sucesso, -1 em caso de erro de comunicação. */
int communicate(int sockfd,
                struct sockaddr_in *server_addr,
                const char *msg,
                const char *expected_tag,
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

/* Verifica localmente que o ficheiro existe e é legível, envia um pedido PUB
 * ao DS e interpreta a resposta RPB. */
cmd_result_t publish_file(int sockfd,
                           struct sockaddr_in *server_addr,
                           const char *uid,
                           const char *password,
                           const char *filename,
                           const char *label);

/* Envia um pedido REM ao DS e interpreta a resposta RRM */
cmd_result_t remove_file(int sockfd,
                          struct sockaddr_in *server_addr,
                          const char *uid,
                          const char *password,
                          const char *filename);

/* Envia um pedido LST ao DS, interpreta a resposta RLS e mostra a lista. */
cmd_result_t list_files(int sockfd,
                         struct sockaddr_in *server_addr);

#endif /* DS_PROTOCOL_H */
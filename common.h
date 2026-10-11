#ifndef COMMON_H
#define COMMON_H

/* Valores por omissão do Directory Server, usados quando -n / -p
 * não são indicados na linha de comandos. */
#define DEFAULT_DSIP   "193.136.138.142"
#define DEFAULT_DSPORT 59000

/* Tamanhos usados na validação e nos buffers do protocolo UDP. */
#define UID_LEN      6
#define PASSWORD_LEN 8

#define MSG_BUF_SIZE      128
#define RESPONSE_BUF_SIZE 256
#define INPUT_BUF_SIZE    256

#define FILENAME_MAX_LEN  24
#define FILENAME_EXT_LEN  3
#define LABEL_MAX_LEN     20
#define MAX_FSIZE         10000000LL
#define MAX_LIST_ENTRIES  50            /* máximo de filenames numa RLS */
#define LIST_RESP_SIZE    2048          /* Buffer para a resposta RLS ao comando list. */
#define TOKEN_SIZE        64            /* Tamanho dos tokens lidos do teclado em user.c. */

/* Comunicação UDP com o DS: cada pedido é enviado até UDP_MAX_ATTEMPTS vezes,
 * esperando UDP_TIMEOUT_SEC segundos pela resposta em cada tentativa.
 * UDP_MAX_DISCARDS limita quantos datagramas inválidos (origem errada ou tipo
 * de resposta inesperado) são ignorados em cada tentativa. */
#define UDP_TIMEOUT_SEC   3
#define UDP_MAX_ATTEMPTS  3
#define UDP_MAX_DISCARDS  5

/* Comunicação TCP com o DS (comando versions). */
#define TCP_TIMEOUT_SEC    5            /* connect / read / write */
#define VRS_MAX_RESP       (1 << 20)    /* limite de 1 MiB para a resposta RVR */
#define VERSIONS_TIME_SIZE 48           /* buffer da publication_time (1 ou 2 tokens) */
#define VERSIONS_TIME_TOKEN_MAX 20      /* tamanho máximo de cada token da data */

#endif /* COMMON_H */
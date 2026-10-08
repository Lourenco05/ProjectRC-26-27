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
#define LIST_TOKEN_SIZE   32            /* Tamanho do buffer de cada filename lido da resposta RLS */
#define TOKEN_SIZE        64            /* Tamanho dos tokens lidos do teclado em user.c. */

#endif /* COMMON_H */
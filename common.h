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

#endif /* COMMON_H */

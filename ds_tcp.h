#ifndef DS_TCP_H
#define DS_TCP_H

#include <netinet/in.h>

#include "ds_protocol.h"     /* cmd_result_t */

/* Mostra a lista de versões disponiveis do ficheiro "filename" através de uma ligação TCP */
cmd_result_t versions_file(struct sockaddr_in *server_addr,
                            const char *filename);

#endif /* DS_TCP_H */
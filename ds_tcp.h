#ifndef DS_TCP_H
#define DS_TCP_H

#include <netinet/in.h>

#include "ds_protocol.h"     /* cmd_result_t */

/* Comando versions: abre uma ligação TCP ao DS (mesmo IP e número de porto
 * usados para o UDP), envia "VRS filename\n", lê a resposta
 * "RVR status [UID Fsize label publication_time availability]*\n",
 * valida-a e mostra a lista de versões de forma legível.
 *
 * Devolve CMD_OK (inclui o caso "sem versões"), CMD_REJECTED (resposta
 * ERR/inesperada/mal formada) ou CMD_COMM_ERROR (connect, timeout, ...). */
cmd_result_t versions_file(struct sockaddr_in *server_addr,
                            const char *filename);

#endif /* DS_TCP_H */
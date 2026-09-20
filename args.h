#ifndef ARGS_H
#define ARGS_H

/* Faz o parsing dos argumentos da linha de comandos:
 *   ./user -m peerport [-n DSIP] [-p DSport]
 *
 * peerport, dsip e dsport são preenchidos com os valores lidos.
 * dsip e dsport mantêm o valor por omissão já presente nas variáveis
 * apontadas caso a respetiva opção não seja fornecida.
 *
 * Retorna 1 em caso de sucesso, 0 em caso de erro (mensagem já
 * impressa para o utilizador).
 */
int parse_arguments(int argc, char *argv[],
                     int *peerport, char **dsip, int *dsport);

#endif /* ARGS_H */

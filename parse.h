#ifndef PARSE_H
#define PARSE_H

/* Divide a linha (alterando-a) em tokens separados por EXATAMENTE um espaço,
 * como exige o protocolo ("a separação entre quaisquer dois items é um único
 * espaço"). Guarda em tokens[] ponteiros para o início de cada token.
 *
 * Devolve o número de tokens (0 para linha vazia) ou -1 se:
 *   - existirem mais de max tokens;
 *   - existir um token vazio (espaço no início/fim ou dois espaços seguidos). */
int split_tokens(char *line, char *tokens[], int max);

#endif /* PARSE_H */
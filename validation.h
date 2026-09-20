#ifndef VALIDATION_H
#define VALIDATION_H

/* Valida um UID: exatamente 6 dígitos decimais. */
int validar_UID(const char *UID);

/* Valida uma password: exatamente 8 caracteres alfanuméricos. */
int validar_Password(const char *Password);

/* Valida um número de porto: inteiro entre 1 e 65535. */
int validar_Port(int Port);

#endif /* VALIDATION_H */

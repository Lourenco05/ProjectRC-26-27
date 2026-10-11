#ifndef VALIDATION_H
#define VALIDATION_H

/* Valida um UID: exatamente 6 dígitos decimais. */
int validar_UID(const char *UID);

/* Valida uma password: exatamente 8 caracteres alfanuméricos. */
int validar_Password(const char *Password);

/* Valida um número de porto: inteiro entre 1 e 65535. */
int validar_Port(int Port);

/* Valida um filename: "base.ext" com no máximo 24 caracteres no total.
 * base: letras, dígitos, '-' e '_'. ext: exatamente 3 alfanuméricos. */
int validar_Filename(const char *Filename);

/* Valida uma label: 1 a 20 caracteres (letras, dígitos, '-' e '_'). */
int validar_Label(const char *Label);

/* Valida um Fsize recebido como texto: 1 a 8 dígitos, valor <= 10 000 000. */
int validar_Fsize(const char *Fsize);

#endif /* VALIDATION_H */
#include <string.h>
#include <ctype.h>

#include "validation.h"
#include "common.h"

int validar_UID(const char *UID)
{
    if (strlen(UID) != 6) {
        return 0;
    }

    for (int i = 0; i < 6; i++) {

        if (!isdigit((unsigned char)UID[i])) {
            return 0;
        }
    }

    return 1;
}


int validar_Password(const char *Password)
{
    if (strlen(Password) != 8) {
        return 0;
    }

    for (int i = 0; i < 8; i++) {

        if (!isalnum((unsigned char)Password[i])) {
            return 0;
        }
    }

    return 1;
}


int validar_Port(int Port)
{
    if (Port >= 1 && Port <= 65535) {
        return 1;
    }

    return 0;
}



int validar_Filename(const char *Filename)
{
    size_t n = strlen(Filename);
    size_t ext = FILENAME_EXT_LEN;
    size_t base = 0;

    /* base >= 1 + '.' + extensão */
    if (n < 1 + 1 + ext || n > FILENAME_MAX_LEN) {
        return 0;
    }

    base = n - ext - 1;             /* comprimento da base */

    if (Filename[base] != '.') {
        return 0;
    }

    for (size_t i = 0; i < base; i++) {
        unsigned char c = (unsigned char)Filename[i];
        if (!isalnum(c) && c != '-' && c != '_') {
            return 0;
        }
    }

    for (size_t i = base + 1; i < n; i++) {
        if (!isalnum((unsigned char)Filename[i])) {
            return 0;
        }
    }

    return 1;
}


int validar_Label(const char *Label)
{
    size_t n = strlen(Label);

    if (n < 1 || n > LABEL_MAX_LEN) {
        return 0;
    }

    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)Label[i];
        if (!isalnum(c) && c != '-' && c != '_') {
            return 0;
        }
    }

    return 1;
}
#include <string.h>
#include <ctype.h>

#include "validation.h"

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

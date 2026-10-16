// Passwords: cuatro letras para volver a empezar en el jefe de cada mundo (una por mundo y dificultad)
#include "juego.h"

static const char claves[4][3][5] = {
    { "GENV", "BOTR", "XELF" },     // bosque: fácil, medio, difícil
    { "SIPE", "MIGY", "QEQP" },     // volcán
    { "LAMF", "RUMV", "NERY" },     // mar
    { "WITL", "PIPJ", "FEDC" },     // cielo
};

const char* password_de(u16 mundo, u16 dif) {
    if (mundo > 3) mundo = 3;
    if (dif > 2) dif = 2;
    return claves[mundo][dif];
}

bool password_validar(const char* clave, u16* mundo, u16* dif) {
    for (u16 m = 0; m < 4; m++)
        for (u16 d = 0; d < 3; d++)
            if (clave[0] == claves[m][d][0] && clave[1] == claves[m][d][1] &&
                clave[2] == claves[m][d][2] && clave[3] == claves[m][d][3]) { *mundo = m; *dif = d; return TRUE; }
    return FALSE;
}

#include <msp432p401r.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "lib_movement.h"
typedef uint8_t byte;


void moveForward(void){

    byte parametros[16];

    parametros[0] = 0x21;
    parametros[1] = 380 & 0xFF;
    parametros[2] = ((CW << 2) & 0x04) | ((380 >> 8) & 0x03);
    TxPacket(id_1, 3, write, parametros);
    RxPacket();
    parametros[2] = ((CWW << 2) & 0x04) | ((velocidad >> 8) & 0x03);
    TxPacket(id_2, 3, write, parametros);
    RxPacket();


}



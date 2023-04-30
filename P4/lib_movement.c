#include <msp432p401r.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "lib_movement.h"
typedef uint8_t byte;


void moveForward(void){

    byte parametros[3];

    parametros[0] = 20;
    parametros[1] = 30;
    parametros[2] = 30;


    TxPacket(0x01, 3, 0x03, parametros);
    RxPacket();


    TxPacket(0x02, 3, 0x03, parametros);
    RxPacket();


}



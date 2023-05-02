#include <msp432p401r.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "lib_movement.h"
typedef uint8_t byte;


void moveForward(void){

    byte parametros[16];

    parametros[0] = 0x20; //Direccion inicial de la ubicacion donde se escriben los datos
    parametros[1] = 380 & 0xFF; // Primer dato a escribir
    parametros[2] = ((0x01 << 2) & 0x04) | ((380 >> 8) & 0x03); // Primer dato a escribir (1=
    TxPacket(0x02, 3, 0x03, parametros);
    RxPacket();

    parametros[2] = ((0x00 << 2) & 0x04) | ((380 >> 8) & 0x03);
    TxPacket(0x01, 3, 0x03, parametros); //Enviamos al motor 3
    RxPacket();


}

void moveBackward(void){

    byte parametros[16];

    parametros[0] = 0x20; //Direccion inicial de la ubicacion donde se escriben los datos
    parametros[1] = 380 & 0xFF; // Primer dato a escribir
    parametros[2] = ((0x00 << 2) & 0x04) | ((380 >> 8) & 0x03); // Primer dato a escribir (1=
    TxPacket(0x02, 3, 0x03, parametros);
    RxPacket();

    parametros[2] = ((0x01 << 2) & 0x04) | ((380 >> 8) & 0x03);
    TxPacket(0x01, 3, 0x03, parametros); //Enviamos al motor 3
    RxPacket();


}


void stopMovement(void){

    byte parametros[16];

    parametros[0] = 0x20;                //ponemos a 0 la direccion 0x20
    parametros[1] = 0;
    parametros[2] = 0;
    TxPacket(0x01, 3, 0x03, parametros);
    RxPacket();                                 //Comprobamos que se ha recibido correctamente
    TxPacket(0x02, 3, 0x03, parametros);
    RxPacket();                                 //Comprobamos que se ha recibido correctamente
}

void turnRight(void){

    byte parametros[16];

    parametros[0] = 0x20;
    parametros[1] = 380 & 0xFF;
    parametros[2] =  ((0x00 << 2) & 0x04) | ((380 >> 8) & 0x03);
    TxPacket(0x02, 3, 0x03, parametros);
    RxPacket();

}

void turnLeft(void){

    byte parametros[16];

    parametros[0] = 0x20;
    parametros[1] = 380 & 0xFF;
    parametros[2] =  ((0x01 << 2) & 0x04) | ((380 >> 8) & 0x03);
    TxPacket(0x01, 3, 0x03, parametros);
    RxPacket();

}




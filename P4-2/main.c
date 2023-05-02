/******************************
 *
 * Practica_03_PAE Timers i interrupcions
 * UB, 03/2021.
 *
 *****************************/

#include <msp432p401r.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "lib_movement.h"
#include "lib_PAE.h"

#define TXD2_READY (UCA2IFG & UCTXIFG)
#define TXD0_READY (UCA0IFG & UCTXIFG)


#define LED_V_BIT BIT0
#define LED_RGB_R BIT0
#define LED_RGB_G BIT1
#define LED_RGB_B BIT2


#define SW1_POS 1
#define SW2_POS 4
#define SW1_INT 0x04
#define SW2_INT 0x0A
#define SW1_BIT BIT(SW1_POS)
#define SW2_BIT BIT(SW2_POS)
typedef uint8_t byte;
volatile byte lecturaDato_UART;
volatile bool Byte_Recibido;
volatile byte timeOut;

volatile bool emulada=true;


volatile uint8_t sensorL;
volatile uint8_t sensorC;
volatile uint8_t sensorR;


typedef struct RxReturn{
    byte StatusPacket[32];
    byte time_out;
    byte bChecksum;
}RxReturn;

byte TimeOut(int time){
    if( timeOut < time) return 0;
    return 1;
}


void init_puerto(){
    P3SEL0 &= ~(BIT0); //P3 como GPIOs
    P3SEL1 &= ~(BIT0);

    P3DIR |= BIT0; // Port P3.0 como salida (Data Direction selector Tx/Rx)
    P3OUT &= ~BIT0; ////Inicializamos el sentido de los datos a 0 (Rx)
}


void Sentit_Dades_Rx(void)
{ //Configuraci� del Half Duplex dels motors: RECEPTOR
   P3OUT &= ~BIT0; //El pin P3.0 (DIRECTION_PORT) el posem a 0 (Rx)
}

void Sentit_Dades_Tx(void)
{ //Configuraci� del Half Duplex dels motors: TRANSMISOR
    P3OUT |= BIT0; //El pin P3.0 (DIRECTION_PORT) el posem a 1 (Tx)
}


void TxUACx(uint8_t bTxdData)
{
    if(emulada){
        while(!TXD0_READY); // Espera a que estigui preparat el buffer de transmissi
        UCA0TXBUF = bTxdData;
    } else{
        while(!TXD2_READY); // Espera a que estigui preparat el buffer de transmissi
        UCA2TXBUF = bTxdData;
}
}

void TA1_0_IRQHandler(void)
{
    TA1CCTL0 &= ~TIMER_A_CCTLN_CCIFG; //Clear interrupt flag
    timeOut+=1;
    TA1CCTL0 |= TIMER_A_CCTLN_CCIE;
}

void reset_timeout(void){
    timeOut=0;
}

void init_UART(void)
{
    if(emulada){
        UCA0CTLW0 |= UCSWRST;
        UCA0CTLW0 |= UCSSEL__SMCLK;
        UCA0MCTLW = UCOS16;
        UCA0BRW = 3;
        UCA0MCTLW |= (0x25 << 8);
        P1SEL0 |= BIT2 | BIT3;
        P1SEL1 &= ~ (BIT2 | BIT3);
        UCA0CTLW0 &= ~UCSWRST;
        EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;
    }else{
        UCA2CTLW0 |= UCSWRST;
        UCA2CTLW0 |= UCSSEL__SMCLK;
        UCA2MCTLW = UCOS16;
        UCA2BRW = 3;
        P3SEL0 |= BIT2 | BIT3;
        P3SEL1 &= ~ (BIT2 | BIT3);
        UCA2CTLW0 &= ~UCSWRST;
        EUSCI_A2->IFG &= ~EUSCI_A_IFG_RXIFG;
        EUSCI_A2->IE |= EUSCI_A_IE_RXIE;
    }
}

void init_timer(void){
    TIMER_A1->CTL=TIMER_A_CTL_ID__1 | TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_CLR | TIMER_A_CTL_MC__UP;
    TIMER_A1->CCR[0]= 240 - 1;
    TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;
}

void Activa_TimerA1_TimeOut(void){
    timeOut=0;
    TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;

}

void Desactiva_TimerA1_TimeOut(){
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIE;
}

struct RxReturn RxPacket(void)
{

    byte bCount, bLength, bChecksum;
    byte Rx_time_out = 0;
    struct RxReturn respuesta;
    respuesta.time_out = 0;
    Activa_TimerA1_TimeOut(); //Activamos el timer para el timeOut
    Sentit_Dades_Rx(); //Ponemos la linea half duplex en Rx


    for (bCount = 0; bCount < 4; bCount++)
    {
        reset_timeout();
        Byte_Recibido = false;
        while (!Byte_Recibido) //Se_ha_recibido_Byte())
        {
            Rx_time_out=TimeOut(100); // tiempo en decenas de microsegundos (ara 10ms)
            if (Rx_time_out) break; //sale del while
        }
        if (Rx_time_out) break; //sale del for si ha habido TimeOut
        respuesta.StatusPacket[bCount] = lecturaDato_UART; //Get_Byte_Leido_UART();
    }
    if (!Rx_time_out)
    {
        // LA LONGUTIUD SE ENCUENTRA EN EL CUARTO BYTE DE LA TRAMA
        bLength = respuesta.StatusPacket[3];
        for (bCount = 0; bCount < bLength; bCount++)
        {
            reset_timeout();
            Byte_Recibido = false;
            while (!Byte_Recibido)
            {
                Rx_time_out = TimeOut(100);
                if (Rx_time_out) break;
            }
            if (Rx_time_out) break;
            respuesta.StatusPacket[bCount + 4] = lecturaDato_UART;
        }

        bChecksum = 0;
        if (!Rx_time_out)
             {
                for (bCount = 2; bCount < bLength - 1; bCount++){
                    bChecksum += respuesta.StatusPacket[bCount];
                }
                if(~bChecksum == respuesta.StatusPacket[bLength+4]){
                     Desactiva_TimerA1_TimeOut();
                     return respuesta;
                }
             }

    }
    respuesta.time_out = Rx_time_out;
    Desactiva_TimerA1_TimeOut();
    return respuesta;

}

void init_interrupciones()
{
    // Configuracion al estilo MSP430 "clasico":
    // --> Enable Port 4 interrupt on the NVIC.
    // Segun el Datasheet (Tabla "6-39. NVIC Interrupts", apartado "6.7.2 Device-Level User Interrupts"),
    // la interrupcion del puerto 1 es la User ISR numero 35.
    // Segun el Technical Reference Manual, apartado "2.4.3 NVIC Registers",
    // hay 2 registros de habilitacion ISER0 y ISER1, cada uno para 32 interrupciones (0..31, y 32..63, resp.),
    // accesibles mediante la estructura NVIC->ISER[x], con x = 0 o x = 1.
    // Asimismo, hay 2 registros para deshabilitarlas: ICERx, y dos registros para limpiarlas: ICPRx.

    if(emulada){
        NVIC->ICPR[0] |= 1 << (EUSCIA0_IRQn & 31);  //Limpia interrupciones habilita interrupciones
        NVIC->ISER[0] |= 1 << (EUSCIA0_IRQn & 31);
    }else{
        NVIC->ICPR[0] |= 1 << (EUSCIA2_IRQn & 31);  //Limpia interrupciones habilita interrupciones
        NVIC->ISER[0] |= 1 << (EUSCIA2_IRQn & 31);
    }
    // Timer A1
    NVIC->ICPR[0] |= 1 << (TA1_0_IRQn & 31); // Limpia cualquier interrupciÃ³n residual pendiente
    NVIC->ISER[0] |= 1 << (TA1_0_IRQn & 31); // Habilita la interrupciÃ³n del Timer A1

}



void init_timers(void)
{
    //TODO
    //Timer A0, used for red LED PWM

    //Timer A1, used for RGB LEDs
    //Divider = 1; CLK source is ACLK; clear the counter; MODE is up

    //CTL --> Registro de control --> Configuraciones generales
    //TIMER_A_CTL_ID__1 --> Divide la fuente de reloj entre 1
    //TIMER_A_CTL_SSEL__ACLK --> Fuente de reloj seleccionada para el contador. El ACLK tiene una frecuencia pequeÃ±a
    // TIMER_A_CTL_CLR --> Volver a 0 una vez se ha alcanzado el valor
    //TIMER_A_CTL_MC__UP --> Modo de conteo: en este caso ascendente


    TIMER_A1->CTL=TIMER_A_CTL_ID__1 | TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_CLR | TIMER_A_CTL_MC__UP;
    TIMER_A1->CCR[0]=240000;
    TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;

}


void EUSCIA0_IRQHandler(void)
{
    //interrupcion de recepcion en la UART A0
    EUSCI_A0->IFG &=~ EUSCI_A_IFG_RXIFG; // Clear interrupt
    UCA0IE &= ~UCRXIE; //Interrupciones desactivadas en RX
    lecturaDato_UART = UCA0RXBUF;
    Byte_Recibido= 1;
    UCA0IE |= UCRXIE; //Interrupciones reactivadas en RX
}


void EUSCIA2_IRQHandler(void)
{ //interrupcion de recepcion en la UART A2
    EUSCI_A2->IFG &=~EUSCI_A_IFG_RXIFG; //Limpia la interrupcion
    UCA2IE &= ~UCRXIE; //Interrupciones desactivadas en RX
    lecturaDato_UART = UCA2RXBUF;
    Byte_Recibido=1;
    UCA2IE |= UCRXIE; //Interrupciones reactivadas en RX
}

byte TxPacket(byte bID, byte bParameterLength, byte bInstruction, byte Parametros[16])
{
    byte bCount,bCheckSum,bPacketLength;
    byte TxBuffer[32];
    Sentit_Dades_Tx();
    TxBuffer[0] = 0xff;
    TxBuffer[1] = 0xff;
    TxBuffer[2] = bID;
    TxBuffer[3] = bParameterLength+2;
    TxBuffer[4] = bInstruction;
    // no lo usamos??????
  //  char error[] = "adr. no permitida";

    if ((Parametros[0] < 6) && (bInstruction == 3)){
        //halLcdPrintLine(error, 8, INVERT_TEXT);
        return 0;
    }

    for(bCount = 0; bCount < bParameterLength; bCount++)
    {
        TxBuffer[bCount+5] = Parametros[bCount];
    }

    bCheckSum = 0;
    bPacketLength = bParameterLength+4+2;

    for(bCount = 2; bCount < bPacketLength-1; bCount++)
    {
        bCheckSum += TxBuffer[bCount];
    }
    TxBuffer[bCount] = ~bCheckSum;
    for(bCount = 0; bCount < bPacketLength; bCount++)
    {
        TxUACx(TxBuffer[bCount]);
    }

    if(emulada){
        while((UCA0STATW & UCBUSY));
    }
    else {
        while((UCA2STATW & UCBUSY));
    }


    Sentit_Dades_Rx();

    return(bPacketLength);
}

void read_sensores(void)
{
    byte id;
    if(emulada){
        id = 3;
    }else{
        id = 100;
    }
    byte parameters[16];
    parameters[0] = 0x1a;  //comencem pel 0x1a i demanem 3 (0x1a,0x1b,0x1c)=esquerra,centre,drtea
    parameters[1] = 3;
    TxPacket(id, 2, 2, parameters);
}

void trama_motors_inicial(void){

    byte parametros[5];
    parametros[0] = 0x06;
    parametros[1] = 0x00; // Valor del l�mite CW (LSB)
    parametros[2] = 0x00; // Valor del l�mite CW (MSB)
    parametros[3] = 0x00;
    parametros[4] = 0x00;
    if(emulada){

    }else{
        TxPacket(0x02, 5, 0x03, parametros);
        RxPacket();
        TxPacket(0x03, 5, 0x03, parametros);
        RxPacket();
    }


    if(emulada){
        byte parametres[2];
        parametres[0] = 0x19;
        parametres[1] = 0x01;
        TxPacket(0x01, 2, 0x03, parametres);
        RxPacket();
        TxPacket(0x02, 2, 0x03, parametres);
        RxPacket();
    }else{
        byte parametres[3];
        parametres[0] = 0x18;
        parametres[1] = 0x01;
        parametres[2] = 0x01;
        TxPacket(0x02, 3, 0x03, parametres);
        RxPacket();
        TxPacket(0x03, 3, 0x03, parametres);
        RxPacket();
    }

}

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer

    //Inicializaciones:
    init_ucs_24MHz();
    init_puerto();
    init_interrupciones(); //Configurar y activar las interrupciones de los botones
    init_timers();
    init_UART();

    __enable_interrupts();

    trama_motors_inicial();


    byte current = 1;
    byte anterior = 0;

    while(true){

       read_sensores();
       RxReturn respuesta = RxPacket();

       if(respuesta.time_out==0){

          sensorL=respuesta.StatusPacket[5];
          sensorC=respuesta.StatusPacket[6];
          sensorR=respuesta.StatusPacket[7];

          if(emulada){
              if(sensorC<150){
                  if(sensorL<sensorR){
                      current = 2;
                  }else{
                      current=3;
                  }
              }else if(sensorC >=150 && sensorL>150 && sensorR>150){
                  current = 1;
              }else if(sensorL <=150 || sensorR<150){
                  current = 2;
              }else if(sensorR <=150 || sensorL<150){
                  current = 3;
              }
          }else{
              if (sensorC >= 30 || sensorL >= 30){
                  current = 2;
              }else if(sensorC >= 30 || sensorR >= 30){
                  current = 3;
              }else{
                  current = 1;
              }
          }

          if(current != anterior){
              anterior = current;
              switch(current){
                  case(1):
                      moveForward();
                      break;
                  case(2):
                       stopMovement();
                       turnRight();
                       break;
                  case(3):
                       stopMovement();
                       turnLeft();
                       break;
              }

          }

       }
    }




}





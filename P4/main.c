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

#define init_ucs_24MHz();

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


volatile uint8_t sensorL;
volatile uint8_t sensorC;
volatile uint8_t sensorR;


typedef struct RxReturn{
    byte StatusPacket[32];
    byte time_out;
}RxReturn;

byte TimeOut(int time){
    if( timeOut < time) return 0;
    return 1;
}


void init_puerto(){
    P3SEL0 &= ~(BIT0); //P3 como GPIOs
    P3SEL1 &= ~(BIT0);

    P1DIR |= BIT0; // Port P3.0 como salida (Data Direction selector Tx/Rx)
    P1OUT &= ~BIT0; ////Inicializamos el sentido de los datos a 0 (Rx)
}


void Sentit_Dades_Rx(void)
{ //Configuraciï¿½ del Half Duplex dels motors: RECEPTOR
    P3OUT &= ~BIT0; //El pin P3.0 (DIRECTION_PORT) el posem a 0 (Rx)
}

void Sentit_Dades_Tx(void)
{ //Configuraciï¿½ del Half Duplex dels motors: TRANSMISOR
    P3OUT |= BIT0; //El pin P3.0 (DIRECTION_PORT) el posem a 1 (Tx)
}


void TxUACx(uint8_t bTxdData)
{
    while(!TXD0_READY); // Espera a que estigui preparat el buffer de transmissiï¿½
    UCA0TXBUF = bTxdData;
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
UCA0CTLW0 |= UCSWRST; //Fem un reset de la USCI, desactiva la USCI
UCA0CTLW0 |= UCSSEL__SMCLK; //UCSYNC=0 mode asíncron
//UCMODEx=0 seleccionem mode UART
//UCSPB=0 nomes 1 stop bit
//UC7BIT=0 8 bits de dades
//UCMSB=0 bit de menys pes primer
//UCPAR=x ja que no es fa servir bit de paritat
//UCPEN=0 sense bit de paritat
//Triem SMCLK (24MHz) com a font del clock BRCLK
UCA0MCTLW = UCOS16; // Necessitem sobre-mostreig => bit 0 = UCOS16 = 1
UCA0BRW = 13; //Prescaler de BRCLK fixat a 13. Com SMCLK va a24MHz,
//volem un baud rate de 115200 bps i fem sobre-mostreig de 16
//el rellotge de la UART ha de ser de ~1.85MHz (24MHz/13).
UCA0MCTLW |= (0x25 << 8); //UCBRSx, part fractional del baud rate
//Configurem els pins de la UART
P1SEL0 |= BIT2 | BIT3; //I/O funció: P1.3 = UART0TX, P1.2 = UART0RX
P1SEL1 &= ~ (BIT2 | BIT3);
UCA0CTLW0 &= ~UCSWRST; //Reactivem la línia de comunicacions sèrie
EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG; // Clear eUSCI RX interrupt flag
//EUSCI_A0->IE |= EUSCI_A_IE_RXIE; // Enable USCI_A0 RX interrupt, nomes quan tinguem la recepcio
}

void init_timer(void){
    TIMER_A1->CTL=TIMER_A_CTL_ID__1 | TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_CLR | TIMER_A_CTL_MC__UP;
    TIMER_A1->CCR[0]= 240000;
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

    //Int. port 1 = 35 corresponde al bit 3 del segundo registro ISER1:
    NVIC->ICPR[0] |= 1 << (EUSCIA0_IRQn & 31);  //Limpia interrupciones habilita interrupciones
    NVIC->ISER[0] |= 1 << (EUSCIA0_IRQn & 31);

    //TODO
    // Timer A1
    NVIC->ICPR[0] |= 1 << (TA1_0_IRQn & 31); // Limpia cualquier interrupciÃƒÂ³n residual pendiente
    NVIC->ISER[0] |= 1 << (TA1_0_IRQn & 31); // Habilita la interrupciÃƒÂ³n del Timer A1

}



void init_timers(void)
{
    //TODO
    //Timer A0, used for red LED PWM

    //Timer A1, used for RGB LEDs
    //Divider = 1; CLK source is ACLK; clear the counter; MODE is up

    //CTL --> Registro de control --> Configuraciones generales
    //TIMER_A_CTL_ID__1 --> Divide la fuente de reloj entre 1
    //TIMER_A_CTL_SSEL__ACLK --> Fuente de reloj seleccionada para el contador. El ACLK tiene una frecuencia pequeÃƒÂ±a
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

byte TxPacket(byte bID, byte bParameterLength, byte bInstruction, byte Parametros[16])
{
    byte bCount,bCheckSum,bPacketLength;
    byte TxBuffer[32];
    TxBuffer[0] = 0xff;
    TxBuffer[1] = 0xff;
    TxBuffer[2] = bID;
    TxBuffer[3] = bParameterLength+2;
    TxBuffer[4] = bInstruction;

    char error[] = "adr. no permitida";

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
    Sentit_Dades_Tx();
    for(bCount = 0; bCount < bPacketLength; bCount++)
    {
        TxUACx(TxBuffer[bCount]);
    }

    while((UCA0STATW & UCBUSY));

    Sentit_Dades_Rx();
    return(bPacketLength);
}

void trama_motors_inicial(void){
    //Motor 1
    byte parametros[3];
    parametros[0] = 6;
    parametros[1] = 0xFF; // Valor del límite CW (LSB)
    parametros[2] = 0x03; // Valor del límite CW (MSB)
    TxPacket(0x01, 3, 0x03, parametros);
    RxPacket();

    //Motor 1
    byte parametros2[3];
    parametros2[0] = 8;
    parametros2[1] = 0x01; // Valor del límite CCW (LSB)
    parametros2[2] = 0x00; // Valor del límite CCW (MSB)
    TxPacket(0x01, 3, 0x03, parametros2);
    RxPacket();

    //Motor 2
    byte parametros3[3];
    parametros3[0] = 6;
    parametros3[1] = 0xFF; // Valor del límite CW (LSB)
    parametros3[2] = 0x03; // Valor del límite CW (MSB)
    TxPacket(0x02, 3, 0x03, parametros3);
    RxPacket();

    //Motor 2
    byte parametros4[3];
    parametros4[0] = 8;
    parametros4[1] = 0x01; // Valor del límite CCW (LSB)
    parametros4[2] = 0x00; // Valor del límite CCW (MSB)
    TxPacket(0x02, 3, 0x03, parametros4);
    RxPacket();
}


void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer

    //Inicializaciones:
   init_ucs_24MHz();
   // init_puerto();
    init_interrupciones(); //Configurar y activar las interrupciones de los botones
    init_timers();
    init_UART();

    __enable_interrupts();

    //trama_motors_inicial();

    //moveForward();

    byte parametros[16];

        parametros[0] =0x19;
        parametros[1] = 0x01;

        TxPacket(0x02, 2, 0x03, parametros);
        struct RxReturn nsq = RxPacket();
        while(true){

        }


    }


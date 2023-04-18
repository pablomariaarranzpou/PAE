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
volatile byte DatoLeido_UART;
volatile bool Byte_Recibido;
volatile byte timeOut;


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

void EUSCIA2_IRQHandler(void)
{ //interrupcion de recepcion en la UART A0
    EUSCI_A2->IFG &=~EUSCI_A_IFG_RXIFG; //Limpia la interrupcion
    UCA2IE &= ~UCRXIE; //Interrupciones desactivadas en RX
    DatoLeido_UART = UCA2RXBUF;
    Byte_Recibido=true;
    UCA2IE |= UCRXIE; //Interrupciones reactivadas en RX
    UCA2MCTLW |= (0x25 << 8);
}

void TxUACx(uint8_t bTxdData)
{
    while(!TXD2_READY); // Espera a que estigui preparat el buffer de transmissi�
    UCA2TXBUF = bTxdData;
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
    UCA2CTLW0 |= UCSWRST; //Fem un reset de la USCI, desactiva la USCI (Uniersal Serial Comunication Interface)
    UCA2CTLW0 |= UCSSEL__SMCLK; //UCSYNC=0 mode as�ncron
    UCA2MCTLW = UCOS16; // Necessitem sobre-mostreig => bit 0 = UCOS16 = 1; 16 muestras por cada bit tramitido.
    UCA2BRW = 1;
    P3SEL0 |= BIT1 | BIT2; //I/O funci�: P1.3 = UART0TX, P1.2 = UART0RX
    P3SEL1 &= ~ (BIT1 | BIT2); // lo configura como primer funcion alternativa
    UCA2CTLW0 &= ~UCSWRST; //Reactivem la l�nia de comunicacions s�rie
    EUSCI_A2->IFG &= ~EUSCI_A_IFG_RXIFG; // Clear eUSCI RX interrupt flag
    //Mantenemos siempre por defecto RXD la cambiamos a TXD no mas cuando queremos transitir datos a algun modulo.
    EUSCI_A2->IE |= EUSCI_A_IE_RXIE; // Enable USCI_A0 RX interrupt, nomes quan tinguem la recepcio
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
            Rx_time_out=TimeOut(1000); // tiempo en decenas de microsegundos (ara 10ms)
            if (Rx_time_out) break; //sale del while
        }
        if (Rx_time_out) break; //sale del for si ha habido TimeOut
        respuesta.StatusPacket[bCount] = DatoLeido_UART; //Get_Byte_Leido_UART();

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
            respuesta.StatusPacket[bCount + 4] = DatoLeido_UART;
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
    NVIC->ICPR[1] |= 1 << (PORT1_IRQn & 31); //Primero, me aseguro de que no quede ninguna interrupcion residual pendiente para este puerto,
    NVIC->ISER[1] |= 1 << (PORT1_IRQn & 31); //y habilito las interrupciones del puerto

    //InterrupciÃ³n del Puerto 2 es la User ISR nÃºmero 38.
    //Para habilitarla, debemos activar el bit 6 del segundo registro ISER1:
    NVIC->ICPR[1] |= 1 << (PORT2_IRQn & 31); //Primero, me aseguro de que no quede ninguna interrupciÃ³n residual pendiente para este puerto,
    NVIC->ISER[1] |= 1 << (PORT2_IRQn & 31); //y habilito las interrupciones del puerto

    //TODO
    // Timer A1
    NVIC->ICPR[0] |= 1 << (TA1_0_IRQn & 31); // Limpia cualquier interrupciÃ³n residual pendiente
    NVIC->ISER[0] |= 1 << (TA1_0_IRQn & 31); // Habilita la interrupciÃ³n del Timer A1

    // Timer A0
    NVIC->ICPR[0] |= 1 << (TA0_0_IRQn & 31); // Limpia cualquier interrupciÃ³n residual pendiente
    NVIC->ISER[0] |= 1 << (TA0_0_IRQn & 31); // Habilita la interrupciÃ³n del Timer A0

    NVIC->ICPR[0] |= 1 << (EUSCIA2_IRQn & 31);  //Limpia interrupciones habilita interrupciones
    NVIC->ISER[0] |= 1 << (EUSCIA2_IRQn & 31);


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
    TIMER_A1->CCR[0]= 240000;
    TIMER_A1->CCTL[0] |= TIMER_A_CCTLN_CCIE;

}


byte TxPacket(byte bID, byte bParameterLength, byte bInstruction, byte Parametros[16])
{
    byte bCount,bCheckSum,bPacketLength;
    byte TxBuffer[32];

    /*
     * TRAMA = [00 id longutiudParametros idInstrucción]
     *
     */
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

    while((UCA2STATW & UCBUSY));

    Sentit_Dades_Rx();
    return(bPacketLength);
}



void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer

    //Inicializaciones:
    //init_ucs_24MHz();
    init_puerto();
    init_interrupciones(); //Configurar y activar las interrupciones de los botones
    init_UART();
    init_timers();
    __enable_interrupts();

    //Bucle principal (infinito):
    while (true)
    {
        moveForward();
    }

}


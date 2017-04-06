// ECE 2534:        myUART.c
//
// Purpose:         User-generated UART implementation. You will have
//                  to complete this implementation for your HW. Right now
//                  the API implementation does not do anything except initialize
//                  the UART (not completely correctly) and send a few bytes
//                  over the Tx to test a terminal running on a computer.
//
// Resources:       UART only
//
// Developed by:    JST
// Last modified:   PEP (9/25/2016)

#include <plib.h>
#include "myDebug.h"
#include "myUART.h"

void UARTInit_TO_BE_REMOVED() {

    // Starter init UART code, that is hard-wired. Not to be used except for
    // the initial example.
    //
    // See the UART PLIB header files for information on these function calls...
    // You can find the definitions by right-clicking on the function name
    // and going to "Navigate" -> "Go to Declaration/Definition"
    UARTConfigure(UART1, UART_ENABLE_PINS_TX_RX_ONLY);
    //
    UARTSetLineControl(UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    // 
    UARTSetDataRate(UART1, 10000000, 9600);
    // 
    UARTEnable(UART1, (UART_ENABLE | UART_PERIPHERAL | UART_TX));
}

void UARTSendAFewCharacters_TO_BE_REMOVED(char *message) {
    int index = 0;
    
    // Send a char at a time from the array to the UART.
    // What happens when you try to send a long message?
    while (message[index] != NULL) {
        DBG_ON(MASK_DBG0); // Use debugging to see when this happens...
        UARTSendDataByte(UART1, message[index]);
        DBG_OFF(MASK_DBG0);
        index++;
    }
}
    
void UARTInit(UART_MODULE uart, unsigned int sourceClock, unsigned int dataRate) {
    // You will have to implement this function to accomplish the described function
    UARTConfigure(uart, UART_ENABLE_PINS_TX_RX_ONLY);
    //
    UARTSetLineControl(uart, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);
    // 
    UARTSetDataRate(uart, sourceClock, dataRate);
    // 
    UARTEnable(uart, (UART_ENABLE | UART_PERIPHERAL | UART_TX | UART_RX));
}

char UARTReceiveByte(UART_MODULE uart) {
    char byte_received = 0;
    // You have to implement this function here...
    while (!UARTReceivedDataIsAvailable(uart))
    {
    }
    byte_received = UARTGetDataByte(uart);
    return byte_received;
}

void UARTSendByte(UART_MODULE uart, char byte_sent) {
    // You will have to implement this function...
    if (UARTTransmitterIsReady(uart)) {
        UARTSendDataByte(uart, byte_sent);
    }
}

void UARTSendString(UART_MODULE uart, const char* string) {
    // Does nothing right now, you will have to implement this function...
    int index = 0;
    while (string[index] != NULL) {
        if (UARTTransmitterIsReady(uart)) {
            UARTSendDataByte(uart, string[index]);
            index++;
        }
    }
}
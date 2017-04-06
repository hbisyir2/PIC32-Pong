// ECE 2534:        myUART.h
//
// Purpose:         User-generated UART API. You shouldn't have to edit
//                  anything in this file for the HW. You only have to
//                  develop the implementation of the UART API specified
//                  here. Note that the Javadoc comments in this file can
//                  be seen whenever you left-click on the corresponding
//                  function in your source (that includes this file) and
//                  click control-space on the keyboard.
//
// Resources:       UART only
// Developed by:    JST
// Last modified:   9/25/16 PEP (Added Javadoc Documentation)

#include <plib.h>

#ifndef MYUART_H
#define MYUART_H

/**
 * UARTInit_TO_BE_REMOVED(); <br>
 * 
 * This is a function that is hard-wired to demonstrate that the UART works.
 * You will replace this function with one specified in the HW.
 */
void UARTInit_TO_BE_REMOVED();

/**
 * UARTSendAFewCharacters_TO_BE_REMOVED(); <br>
 * 
 * This is a function that is hard-wired to demonstrate that the UART works.
 * You will replace this function with one specified in the HW.
 */
void UARTSendAFewCharacters_TO_BE_REMOVED(char *message);

/**
 * 
 * UARTInit(UARTPMODULE module, unsigned int clockSource, unsigned in baudRate) <br>
 * 
 * Description: Initializes UART on the PIC32 for transmission and reception
 * using a data frame of  8 data bits, no parity, and 1 stop bit.
 * Also turns on the UART. <br>
 * 
 * @param uart - The PIC32 UART module to be used.
 * @param sourceClock - The Cerebot peripheral clock frequency
 * @param dataRate - the baud rate of the UART channel
 */
void UARTInit(UART_MODULE uart, unsigned int sourceClock, unsigned int dataRate);

/**
 * UARTReceiveByte(UART_MODULE uart); <br>
 * 
 * Description: Receives a byte from UART. Note that this function should
 * only return once a byte has been received from the UART (that is, it should
 * "block" until a byte arrives. <br>
 * 
 * Returns an char (byte read from UART).<br>
 * 
 * @param uart - The PIC32 UART module to be used.
 * @return byte_received - the byte received.
 */
char UARTReceiveByte(UART_MODULE uart);

/**
 * UARTSendByte(UART_MODULE uart, char byte_sent); <br>
 * 
 * Sends a byte to the specified UART.<br>
 * 
 * @param uart
 * @param byte_sent
 */
void UARTSendByte(UART_MODULE uart, char byte_sent);

/**
 * UARTSendString(UART_MODULE uart, const char* string);<br>
 * 
 * Sends the C-string passed by the argument pointer to the specified UART. <br>
 * 
 * @param uart
 * @param string (A pointer to the C-string)
 */
void UARTSendString(UART_MODULE uart, const char* string);

#endif

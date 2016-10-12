#ifndef UART_CMD_H
#define UART_CMD_H

/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Demo application includes. */
#include "serial.h"

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE		1024

/* Dimentions a buffer to be used by the UART driver, if the UART driver uses a
 * buffer at all. */
#define cmdQUEUE_LENGTH			2048

/* DEL acts as a backspace. */
#define cmdASCII_DEL			(0x7F)
#define cmdASCII_ESC			(0x1B)
#define cmdASCII_BELL			(0x07)

/* VT100 termunal commands */
#define cmdVT100_CURSOR_BACKWARD		"\033[D"
#define cmdVT100_CURSOR_FORWARD			"\033[C"
#define cmdVT100_CLEAR_SCREEN			"\033[2J"
#define cmdVT100_CURSOR_HOME			"\033[0;0H"
#define cmdVT100_CURSOR_BOTTOM_2_LINES	"\033[2B"
#define cmdVT100_CURSOR_BOTTOM			"\033[B"
#define cmdVT100_CURSOR_UP				"\033[A"

/* The maximum time to wait for the mutex that guards the UART to become
 * available. */
#define cmdMAX_MUTEX_WAIT		pdMS_TO_TICKS(300)

#ifndef configCLI_BAUD_RATE
	#define configCLI_BAUD_RATE	115200
#endif

/*
 * The task that implements the command console processing.
 */
void prvUARTCommandConsoleTask(void *pvParameters);
void vUARTCommandConsoleStart(uint16_t usStackSize, UBaseType_t uxPriority);

/*----------------------------------------------------------------------------*/

#endif

/*
 FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
 All rights reserved

 VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

 This file is part of the FreeRTOS distribution.

 FreeRTOS is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License (version 2) as published by the
 Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

 ***************************************************************************
 >>!   NOTE: The modification to the GPL is included to allow you to     !<<
 >>!   distribute a combined work that includes FreeRTOS without being   !<<
 >>!   obliged to provide the source code for proprietary components     !<<
 >>!   outside of the FreeRTOS kernel.                                   !<<
 ***************************************************************************

 FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  Full license text is available on the following
 link: http://www.freertos.org/a00114.html

 ***************************************************************************
 *                                                                       *
 *    FreeRTOS provides completely free yet professionally developed,    *
 *    robust, strictly quality controlled, supported, and cross          *
 *    platform software that is more than just the market leader, it     *
 *    is the industry's de facto standard.                               *
 *                                                                       *
 *    Help yourself get started quickly while simultaneously helping     *
 *    to support the FreeRTOS project by purchasing a FreeRTOS           *
 *    tutorial book, reference manual, or both:                          *
 *    http://www.FreeRTOS.org/Documentation                              *
 *                                                                       *
 ***************************************************************************

 http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
 the FAQ page "My application does not run, what could be wrong?".  Have you
 defined configASSERT()?

 http://www.FreeRTOS.org/support - In return for receiving this top quality
 embedded software for free we request you assist our global community by
 participating in the support forum.

 http://www.FreeRTOS.org/training - Investing in training allows your team to
 be as productive as possible as early as possible.  Now you can receive
 FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
 Ltd, and the world's leading authority on the world's leading RTOS.

 http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
 including FreeRTOS+Trace - an indispensable productivity tool, a DOS
 compatible FAT file system, and our tiny thread aware UDP/IP stack.

 http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
 Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

 http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
 Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
 licenses offer ticketed support, indemnification and commercial middleware.

 http://www.SafeRTOS.com - High Integrity Systems also provide a safety
 engineered and independently SIL3 certified version for use in safety and
 mission critical applications that require provable dependability.

 1 tab == 4 spaces!
 */

/*
 * NOTE:  This file uses a third party USB CDC driver.
 */

#include "cli-uart.h"
#include "cli-core.h"
#include "microrl.h"

/*----------------------------------------------------------------------------*/
/* Private functions definitions. */
void prvOutputString(const char * const pcMessage);
char prvGetChar(void);
int prvExecute(const char * const arg);

/*----------------------------------------------------------------------------*/

/* Create microrl object and pointer on it. */
microrl_t rl;
microrl_t * prl = &rl;

/* Const messages output by the command console. */
static const char * const pcWelcomeMessage =
cmdVT100_CLEAR_SCREEN
cmdVT100_CURSOR_HOME
cmdVT100_CURSOR_BOTTOM
".-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-.\r\n"
"|  _______        _        _______      __     __ |\r\n"
"| |__   __|      (_)      |__   __|     \\ \\   / / |\r\n"
"!    | |_ __ __ _ _  ___  ___| | ___  _ _\\ \\_/ /  !\r\n"
":    | | '__/ _` | |/ _ \\/ __| |/ _ \\| '__\\   /   :\r\n"
".    | | | | (_| | |  __/ (__| | (_) | |   | |    .\r\n"
":    |_|_|  \\__,_| |\\___|\\___|_|\\___/|_|   |_|    :\r\n"
"!               _/ |                              !\r\n"
"|    ver. 0.0  |__/        Trajectory CNC         |\r\n"
"`-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-'\r\n"
"\r\n"
" Type \"help\" to view a list of available commands.\r\n"
"\r\n";

/* Used to guard access to the UART in case messages are sent to the UART from
 * more than one task. */
static SemaphoreHandle_t xTxMutex = NULL;

/* The handle to the UART port, which is not used by all ports. */
static xComPortHandle xPort = 0;

/*----------------------------------------------------------------------------*/

void vUARTCommandConsoleStart(uint16_t usStackSize, UBaseType_t uxPriority)
{
	/* Create the semaphore used to access the UART Tx. */
	xTxMutex = xSemaphoreCreateMutex();
	configASSERT(xTxMutex);

	/* Create that task that handles the console itself. */
	xTaskCreate(
			/* The task that implements the command console. */
			prvUARTCommandConsoleTask,
			/* Text name assigned to the task.  This is just to assist
			 * debugging.  The kernel does not use this name itself. */
			"CLI",
			/* The size of the stack allocated to the task. */
			usStackSize,
			/* The parameter is not used, so NULL is passed. */
			NULL,
			/* The priority allocated to the task. */
			uxPriority,
			 /* A handle is not required, so just pass NULL. */
			NULL
	);
}
/*----------------------------------------------------------------------------*/

//void prvUARTCommandConsoleTask(void *pvParameters)
//{
//	signed char cRxedChar;
//	BaseType_t xEscSequenceCounter = 0;
//	uint8_t ucInputIndex = 0;
//	char *pcOutputString;
//	static char cInputString[cmdMAX_INPUT_SIZE];
//	static char cLastInputString[cmdMAX_INPUT_SIZE];
//	BaseType_t xReturned;
//	xComPortHandle xPort;
//
//	(void) pvParameters;
//
//	/* Obtain the address of the output buffer.  Note there is no mutual
//	 * exclusion on this buffer as it is assumed only one command console
//	 * interface will be used at any one time. */
//	pcOutputString = FreeRTOS_CLIGetOutputBuffer();
//
//	/* Initialise the UART. */
//	xPort = xSerialPortInitMinimal(configCLI_BAUD_RATE, cmdQUEUE_LENGTH);
//
//	/* Send the welcome message. */
//	vSerialPutString(
//			xPort,
//			(signed char *) pcWelcomeMessage,
//			(unsigned short) strlen(pcWelcomeMessage)
//	);
//
//	for (;;) {
//		/* Wait for the next character.  The while loop is used in case
//		 * INCLUDE_vTaskSuspend is not set to 1 - in which case
//		 * portMAX_DELAY will be a genuine block time rather than an infinite
//		 * block time. */
//		while (xSerialGetChar(xPort, &cRxedChar, portMAX_DELAY) != pdPASS)
//			;
//
//		/* Ensure exclusive access to the UART Tx. */
//		if (xSemaphoreTake(xTxMutex, cmdMAX_MUTEX_WAIT) == pdPASS) {
//			/* Echo the character back. */
//			xSerialPutChar(xPort, cRxedChar, portMAX_DELAY);
//
//			/* Was it the end of the line? */
//			if (cRxedChar == '\n' || cRxedChar == '\r') {
//				/* Just to space the output from the input. */
//				vSerialPutString(
//						xPort,
//						(signed char *) pcNewLine,
//						(unsigned short) strlen(pcNewLine)
//				);
//
//				/* See if the command is empty, indicating that the last command
//				 * is to be executed again. */
//				if (ucInputIndex == 0) {
//					/* Copy the last command back into the input string. */
//					strcpy(cInputString, cLastInputString);
//				}
//
//				/* Pass the received command to the command interpreter.  The
//				 * command interpreter is called repeatedly until it returns
//				 * pdFALSE	(indicating there is no more output) as it might
//				 * generate more than one string. */
//				do {
//					/* Get the next output string from the command
//					 * interpreter. */
//					xReturned = FreeRTOS_CLIProcessCommand(
//							cInputString,
//							pcOutputString,
//							configCOMMAND_INT_MAX_OUTPUT_SIZE
//					);
//
//					/* Write the generated string to the UART. */
//					vSerialPutString(
//							xPort,
//							(signed char *) pcOutputString,
//							(unsigned short) strlen(pcOutputString)
//					);
//
//				} while (xReturned != pdFALSE);
//
//				/* All the strings generated by the input command have been
//				 * sent.  Clear the input string ready to receive the next
//				 * command.  Remember the command that was just processed first
//				 * in case it is to be processed again. */
//				strcpy(cLastInputString, cInputString);
//				ucInputIndex = 0;
//				memset(cInputString, 0x00, cmdMAX_INPUT_SIZE);
//
//				vSerialPutString(
//						xPort,
//						(signed char *) pcEndOfOutputMessage,
//						(unsigned short) strlen(pcEndOfOutputMessage)
//				);
//			} else {
//				if (cRxedChar == '\r') {
//					/* Ignore the character. */
//				} else if (cRxedChar == cmdASCII_ESC) {
//					/* Escape sequence start. */
//					xEscSequenceCounter++;
//				} else if ((cRxedChar == '\b') || (cRxedChar == cmdASCII_DEL)) {
//					/* Backspace was pressed.  Erase the last character in the
//					 * string - if any. */
//					if (ucInputIndex > 0) {
//						ucInputIndex--;
//						cInputString[ucInputIndex] = '\0';
//					} else {
//						vSerialPutString(
//								xPort,
//								(signed char *) cmdVT100_CURSOR_FORWARD,
//								(unsigned short) strlen(cmdVT100_CURSOR_FORWARD)
//						);
//					}
//				} else {
//					/* A character was entered.  Add it to the string entered so
//					 * far.  When a \n is entered the complete	string will be
//					 *  passed to the command interpreter. */
//					if (xEscSequenceCounter > 0) {
//						/* Process escape sequence. */
//						switch (cRxedChar) {
//							case '[':
//								xEscSequenceCounter++;
//								break;
//							case 'A':
//							case 'B':
//							case '~':
//							case cmdASCII_ESC:
//								xEscSequenceCounter = 0;
//								break;
//							case 'C':
//							case 'D':
//								xEscSequenceCounter = 0;
//								break;
//							default:
//								if (xEscSequenceCounter > 1) {
//									xEscSequenceCounter++;
//								}
//								break;
//						}
//					} else {
//						if ((cRxedChar >= ' ') && (cRxedChar <= '~')) {
//							if (ucInputIndex < cmdMAX_INPUT_SIZE) {
//								cInputString[ucInputIndex] = cRxedChar;
//								ucInputIndex++;
//							}
//						}
//					}
//				}
//			}
//
//			/* Must ensure to give the mutex back. */
//			xSemaphoreGive(xTxMutex);
//		}
//	}
//}
///*--------------------------------------------------------------------------*/

void prvUARTCommandConsoleTask(void *pvParameters __attribute__((unused)))
{
	/* Initialise the UART. */
	/* TODO: remove from here later. */
	xPort = xSerialPortInitMinimal(configCLI_BAUD_RATE, cmdQUEUE_LENGTH);

	prvOutputString(pcWelcomeMessage);

	/* Call init with ptr to microrl instance and print callback. */
	microrl_init(prl, prvOutputString);
	/* Set callback for execute. */
	microrl_set_execute_callback(prl, prvExecute);
#ifdef _USE_COMPLETE
	/* Set callback for completion. */
	microrl_set_complete_callback(prl, NULL);
#endif
	/* Set callback for Ctrl+C. */
	microrl_set_sigint_callback(prl, NULL);

	for (;;) {
		/* Put received char from stdin to microrl lib. */
		microrl_insert_char(prl, prvGetChar());
	}
}

/*----------------------------------------------------------------------------*/

void prvOutputString(const char * const pcMessage)
{
//	if (xSemaphoreTake(xTxMutex, cmdMAX_MUTEX_WAIT) == pdPASS) {
		vSerialPutString(
				xPort,
				(signed char *) pcMessage,
				(unsigned short) strlen(pcMessage)
		);
//		xSemaphoreGive(xTxMutex);
//	}
}
/*----------------------------------------------------------------------------*/

char prvGetChar(void)
{
	signed char cRxedChar;
	while (xSerialGetChar(xPort, &cRxedChar, portMAX_DELAY) != pdPASS)
		;
	return (char)cRxedChar;
}
/*----------------------------------------------------------------------------*/

int prvExecute(const char * const cInputString)
{
	char *pcOutputString;
	BaseType_t xReturned;

	pcOutputString = FreeRTOS_CLIGetOutputBuffer();

	/* Pass the received command to the command interpreter.  The
	 * command interpreter is called repeatedly until it returns
	 * pdFALSE	(indicating there is no more output) as it might
	 * generate more than one string. */
	do {
		/* Get the next output string from the command
		 * interpreter. */
		xReturned = FreeRTOS_CLIProcessCommand(
				cInputString,
				pcOutputString,
				configCOMMAND_INT_MAX_OUTPUT_SIZE
		);

		/* Write the generated string to the UART. */
		prvOutputString(pcOutputString);

	} while (xReturned != pdFALSE);

	return 0;
}
/*----------------------------------------------------------------------------*/

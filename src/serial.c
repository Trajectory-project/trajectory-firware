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
 BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR UART0.
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* Library includes. */
#include "stm32f4xx_hal.h"

/* Demo application includes. */
#include "serial.h"
/*-----------------------------------------------------------*/

/* Misc defines. */
#define serINVALID_QUEUE				( ( QueueHandle_t ) 0 )
#define serNO_BLOCK						( ( TickType_t ) 0 )
#define serTX_BLOCK_TIME				( 40 / portTICK_PERIOD_MS )

/*-----------------------------------------------------------*/

/* The queue used to hold received characters. */
static QueueHandle_t xRxedChars;
static QueueHandle_t xCharsForTx;

UART_HandleTypeDef huart3;

/*-----------------------------------------------------------*/

/* UART interrupt handler. */
void vUARTInterruptHandler(void);

/*-----------------------------------------------------------*/

/*
 * See the serial2.h header file.
 */
xComPortHandle
xSerialPortInitMinimal(
		unsigned long ulWantedBaud,
		unsigned portBASE_TYPE uxQueueLength
) {
	xComPortHandle xReturn;

	/* Create the queues used to hold Rx/Tx characters. */
	xRxedChars = xQueueCreate(
			uxQueueLength,
			(unsigned portBASE_TYPE) sizeof(signed char)
	);
	xCharsForTx = xQueueCreate(
			uxQueueLength + 1,
			(unsigned portBASE_TYPE) sizeof(signed char)
	);

	/* If the queue/semaphore was created correctly then setup the serial port
	 hardware. */
	if ((xRxedChars != serINVALID_QUEUE) && (xCharsForTx != serINVALID_QUEUE)) {
		__HAL_RCC_GPIOC_CLK_ENABLE();

		huart3.Instance = USART3;
		huart3.Init.BaudRate = ulWantedBaud;
		huart3.Init.WordLength = UART_WORDLENGTH_8B;
		huart3.Init.StopBits = UART_STOPBITS_1;
		huart3.Init.Parity = UART_PARITY_NONE;
		huart3.Init.Mode = UART_MODE_TX_RX;
		huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		huart3.Init.OverSampling = UART_OVERSAMPLING_16;

		if (HAL_UART_Init(&huart3) != HAL_OK) {
			Error_Handler();
		}
	} else {
		xReturn = (xComPortHandle) 0;
	}

	/* This demo file only supports a single port but we have to return
	 something to comply with the standard demo header file. */
	return xReturn;
}
/*----------------------------------------------------------------------------*/

signed portBASE_TYPE
xSerialGetChar(
		xComPortHandle pxPort,
		signed char *pcRxedChar,
		TickType_t xBlockTime
) {
	/* The port handle is not required as this driver only supports one port. */
	(void) pxPort;

	/* Get the next character from the buffer.  Return false if no characters
	 are available, or arrive before xBlockTime expires. */
	if (xQueueReceive(xRxedChars, pcRxedChar, xBlockTime)) {
		return pdTRUE;
	} else {
		return pdFALSE;
	}
}
/*----------------------------------------------------------------------------*/

void
vSerialPutString(
		xComPortHandle pxPort,
		const signed char * const pcString,
		unsigned short usStringLength
) {
	signed char *pxNext;

	/* A couple of parameters that this port does not use. */
	(void) usStringLength;
	(void) pxPort;

	/* NOTE: This implementation does not handle the queue being full as no
	 block time is used! */

	/* The port handle is not required as this driver only supports UART1. */
	(void) pxPort;

	/* Send each character in the string, one at a time. */
	pxNext = (signed char *) pcString;
	while (*pxNext) {
		xSerialPutChar(pxPort, *pxNext, serNO_BLOCK);
		pxNext++;
	}
}
/*----------------------------------------------------------------------------*/

signed portBASE_TYPE
xSerialPutChar(
		xComPortHandle pxPort __attribute__((unused)),
		signed char cOutChar,
		TickType_t xBlockTime
) {
	signed portBASE_TYPE xReturn;

	if ( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) == pdPASS) {
		xReturn = pdPASS;
		__HAL_UART_ENABLE_IT(&huart3, UART_IT_TXE);
	} else {
		xReturn = pdFAIL;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

void vSerialClose(xComPortHandle xPort __attribute__((unused)))
{
	/* Not supported as not required by the demo application. */
}
/*-----------------------------------------------------------*/

void vUARTInterruptHandler(void)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	char cChar;

	if ((__HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_TXE) &&
		__HAL_UART_GET_FLAG(&huart3, UART_FLAG_TXE)) != RESET) {
		/* The interrupt was caused by the THR becoming empty.  Are there any
		 more characters to transmit? */
		if (xQueueReceiveFromISR(
				xCharsForTx,
				&cChar,
				&xHigherPriorityTaskWoken
		) == pdTRUE) {
			/* A character was retrieved from the queue so can be sent to the
			 THR now. */
			huart3.Instance->DR = (uint8_t) (cChar & (uint8_t) 0xFF);
		} else {
			__HAL_UART_DISABLE_IT(&huart3, UART_IT_TXE);
		}
	}

	if ((__HAL_UART_GET_IT_SOURCE(&huart3, UART_IT_RXNE) != RESET) &&
		(__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE) != RESET))
	{
		cChar = (uint8_t)(huart3.Instance->DR & (uint8_t)0x00FF);
		xQueueSendFromISR(xRxedChars, &cChar, &xHigherPriorityTaskWoken);
		__HAL_UART_CLEAR_FLAG(&huart3, UART_FLAG_RXNE);
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);

	return;
}

/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "queue.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"

#define BUTTON_1_PERIOD 50
#define BUTTON_2_PERIOD 50
#define PERIODIC_TRANSMITTER_PERIOD 100
#define UART_RECEIVER_PERIOD 20
#define LOAD_1_SIMULATION_PERIOD 10
#define LOAD_2_SIMULATION_PERIOD 100

/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )

/* Task Handles */
TaskHandle_t Button_1_Monitor_Handler = NULL;
TaskHandle_t Button_2_Monitor_Handler = NULL;
TaskHandle_t Periodic_Transmitter_Handler = NULL;
TaskHandle_t Uart_Receiver_Handler = NULL;
TaskHandle_t Load_1_Simulation_Handler = NULL;
TaskHandle_t Load_2_Simulation_Handler = NULL;


/* Queue Handles */
QueueHandle_t xQueue1;
QueueHandle_t xQueue2;
QueueHandle_t xQueue3;

/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/

/* Task to be created. */
void Button_1_Monitor( void * pvParameters )
{
		pinState_t Button_1_NewState;
		pinState_t Button_1_OldState = GPIO_read(PORT_0 , PIN0);
		TickType_t xLastWakeTime = xTaskGetTickCount();
		uint8_t Edge;
	
    for( ;; )
    {
			Button_1_NewState = GPIO_read(PORT_0,PIN0);
			
			if( (Button_1_OldState == PIN_IS_LOW) && (Button_1_NewState == PIN_IS_HIGH))
			{
				Edge = 1; /* Rising Edge */
			}
			else if( (Button_1_OldState == PIN_IS_HIGH) && (Button_1_NewState == PIN_IS_LOW))
			{
				Edge = 0; /* Falling Edge */
			}
			else
			{
				Edge = 2;
			}
			
			Button_1_OldState = Button_1_NewState;
			xQueueOverwrite( xQueue1, &Edge );
			vTaskDelayUntil(&xLastWakeTime , BUTTON_1_PERIOD);
    }
}

/* Task to be created. */
void Button_2_Monitor( void * pvParameters )
{
		pinState_t Button_2_NewState;
		pinState_t Button_2_OldState = GPIO_read(PORT_0 , PIN1);
		TickType_t xLastWakeTime = xTaskGetTickCount();
		uint8_t Edge;
	
    for( ;; )
    {
			Button_2_NewState = GPIO_read(PORT_0,PIN1);
			
			if( (Button_2_OldState == PIN_IS_LOW) && (Button_2_NewState == PIN_IS_HIGH))
			{
				Edge = 1; /* Rising Edge */
			}
			else if( (Button_2_OldState == PIN_IS_HIGH) && (Button_2_NewState == PIN_IS_LOW))
			{
				Edge = 0; /* Falling Edge */
			}
			else
			{
				Edge = 2;
			}
			
			Button_2_OldState = Button_2_NewState;
			
			xQueueOverwrite( xQueue2, &Edge );
			vTaskDelayUntil(&xLastWakeTime , BUTTON_2_PERIOD);
    }
}

/* Task to be created. */
void Periodic_Transmitter( void * pvParameters )
{
		TickType_t xLastWakeTime = xTaskGetTickCount();
		uint8_t i = 0;
		char periodicString[19];
		strcpy(periodicString, "\nHave a nice day!");
	
    for( ;; )
    {
			for(i = 0; i < 19; i++)
			{
				xQueueSend( xQueue3, (periodicString+i), 100);
			}
			
			vTaskDelayUntil(&xLastWakeTime , PERIODIC_TRANSMITTER_PERIOD);
    }
}

/* Task to be created. */
void Uart_Receiver( void * pvParameters )
{
		TickType_t xLastWakeTime = xTaskGetTickCount();
		uint8_t Button_1_Edge;
		uint8_t Button_2_Edge;
		char periodicRx[17];
		uint8_t i = 0;
	
    for( ;; )
    {
			if(xQueueReceive(xQueue1, &Button_1_Edge, 0) && Button_1_Edge != 2)
			{
				xSerialPutChar('\n');	
				if(Button_1_Edge == 1)
				{
					char String1[35]= "Rising Edge Detected in Button 1\n";
					vSerialPutString((signed char *)String1, strlen(String1));
				}
				else
				{
					char String1[36]= "Falling Edge Detected in Button 1\n";
					vSerialPutString((signed char *)String1, strlen(String1));
				}
			}
			else
			{
				
			}
			
			if(xQueueReceive(xQueue2, &Button_2_Edge, 0) && Button_2_Edge != 2)
			{
				xSerialPutChar('\n');	
				if(Button_2_Edge == 1)
				{
					char String1[35]= "Rising Edge Detected in Button 2\n";
					vSerialPutString((signed char *)String1, strlen(String1));
				}
				else
				{
					char String1[36]= "Falling Edge Detected in Button 2\n";
					vSerialPutString((signed char *)String1, strlen(String1));
				}
			}
			else
			{
				
			}
			
			if(uxQueueMessagesWaiting(xQueue3) != 0)
			{
				for(i = 0; i<18 ; i++)
				{
					xQueueReceive(xQueue3, (periodicRx+i), 0);
				}
				vSerialPutString( (signed char *) periodicRx, strlen(periodicRx));
				xQueueReset(xQueue3);
			}
			vTaskDelayUntil( &xLastWakeTime , UART_RECEIVER_PERIOD);
    }
}

/* Task to be created. */
void Load_1_Simulation( void * pvParameters )
{
		TickType_t xLastWakeTime = xTaskGetTickCount();
		uint32_t i = 0;
		uint32_t Period = 12000*5; /* (XTAL / 1000U)*time_in_ms  */
	
    for( ;; )
    {
			for(i = 0; i <= Period; i++)
			{
				
			}
			vTaskDelayUntil( &xLastWakeTime , LOAD_1_SIMULATION_PERIOD);
    }
}

/* Task to be created. */
void Load_2_Simulation( void * pvParameters )
{
		TickType_t xLastWakeTime = xTaskGetTickCount();
		uint32_t i = 0;
		uint32_t Period = 12000*12; /* (XTAL / 1000U)*time_in_ms  */
	
    for( ;; )
    {
			for(i = 0; i <= Period; i++)
			{
				
			}
			vTaskDelayUntil( &xLastWakeTime , LOAD_2_SIMULATION_PERIOD);
    }
}

/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	xQueue1 = xQueueCreate( 1, sizeof( char ) );
	xQueue2 = xQueueCreate( 1, sizeof( char ) );
	xQueue3 = xQueueCreate( 19, sizeof( char ) );
	
    /* Create Tasks here */
	 /* Create the task, storing the handle. */
    xTaskPeriodicCreate(
                    Button_1_Monitor,       /* Function that implements the task. */
                    "Button_1_Monitor Task",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Button_1_Monitor_Handler,
										BUTTON_1_PERIOD);      /* Used to pass out the created task's handle. */
										

    xTaskPeriodicCreate(
                    Button_2_Monitor,       /* Function that implements the task. */
                    "Button_2_Monitor Task",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Button_2_Monitor_Handler,
										BUTTON_2_PERIOD);      /* Used to pass out the created task's handle. */
										
		xTaskPeriodicCreate(
                    Periodic_Transmitter,       /* Function that implements the task. */
                    "Periodic_Transmitter Task",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Periodic_Transmitter_Handler,
										PERIODIC_TRANSMITTER_PERIOD);      /* Used to pass out the created task's handle. */

		xTaskPeriodicCreate(
                    Uart_Receiver,       /* Function that implements the task. */
                    "Uart_Receiver Task",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Uart_Receiver_Handler,
										UART_RECEIVER_PERIOD);      /* Used to pass out the created task's handle. */
										
		xTaskPeriodicCreate(
                    Load_1_Simulation,       /* Function that implements the task. */
                    "Load_1_Simulation Task",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Load_1_Simulation_Handler,
										LOAD_1_SIMULATION_PERIOD);      /* Used to pass out the created task's handle. */
										
		xTaskPeriodicCreate(
                    Load_2_Simulation,       /* Function that implements the task. */
                    "Load_2_Simulation Task",          /* Text name for the task. */
                    100,      /* Stack size in words, not bytes. */
                    ( void * ) 0,    /* Parameter passed into the task. */
                    1,/* Priority at which the task is created. */
                    &Load_2_Simulation_Handler,
										LOAD_2_SIMULATION_PERIOD);      /* Used to pass out the created task's handle. */


	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationTickHook (void)
{
	GPIO_write(PORT_0,PIN2,PIN_IS_HIGH);
	GPIO_write(PORT_0,PIN2,PIN_IS_LOW);
}

void vApplicationIdleHook( void )
{
	GPIO_write(PORT_1, PIN0, PIN_IS_HIGH);
}

/* Function to reset timer 1 */
void timer1Reset(void)
{
	T1TCR |= 0x2;
	T1TCR &= ~0x2;
}

/* Function to initialize and start timer 1 */
static void configTimer1(void)
{
	T1PR = 1000;
	T1TCR |= 0x1;
}

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();
	
	/* Config trace timer 1 and read T1TC to get current tick */
	configTimer1();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/



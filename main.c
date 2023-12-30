/*******************************************************************************
* Copyright (C) 2019-2023 Maxim Integrated Products, Inc., All rights Reserved.
*
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************/

// RTDemo_MAX_alpha
// This file was @generated by ai8xize.py --test-dir synthed_net --prefix RTDemo_MAX_alpha --checkpoint-file trained/tinyfallnet-qat8-q.pth.tar --config-file networks/tinyfallnet.yaml --sample-input tests/sample_kfall.npy --device MAX78000 --board-name FTHR_RevA --compact-data --mexpress --timer 0 --display-checkpoint --verbose --overwrite

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mxc.h"
#include "cnn.h"
#include "led.h"
#include "board.h"
#include "pb.h"
#include "gpio.h"
#include "uart.h"

mxc_uart_req_t uart0_trans_req;

uint8_t WorkMode = 0U;		//0: Off	1: From host	2: From sensor
uint8_t SwitchRequest = 0U;
//uint8_t AccGyrRequest = 0U;
//uint8_t MagRequest = 0U;
uint8_t HostRequest = 0U;
uint8_t NewDataFetched = 0U;
uint8_t FallDetected = 0U;

int8_t RecvBuffer[1][50][6];
uint8_t RecvBufferPTR = 0U;

volatile uint32_t cnn_time; // Stopwatch

void Peripheral_Init(void);
void Peripheral_Reconfig(void);
void DataFetchHandle(void);
void ModeSwitchHandle(void);
void UART0_Handler(void);
void UARTRxCallback(mxc_uart_req_t *req, int result);
void GPIO_ISR(void *cbdata);

void load_input(void)
{
  // This function loads the sample data input -- replace with actual data
	uint32_t* ptr = (uint32_t *)0x50400000;
	uint32_t tmp = 0U;
	for(uint8_t i=0; i<50; i++)
	{
		tmp = 0U;
		for(int8_t j=3; j>=0; j--)
		{
			tmp = tmp << 8;
			tmp |= (uint8_t)RecvBuffer[0][i][j];
		}
		*(ptr++) = tmp;
	}
	ptr = (uint32_t *)0x50408000;
	for(uint8_t i=0; i<50; i++)
	{
		tmp = 0U;
		for(int8_t j=5; j>=4; j--)
		{
			tmp = tmp << 8;
			tmp |= (uint8_t)RecvBuffer[0][i][j];
		}
		*(ptr++) = tmp;
	}
}

static int32_t ml_data32[(CNN_NUM_OUTPUTS + 3) / 4];

int main(void)
{
	MXC_ICC_Enable(MXC_ICC0); // Enable cache

	// Switch to 100 MHz clock
	MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
	SystemCoreClockUpdate();

	Peripheral_Init();
	Peripheral_Reconfig();
	printf("リンクスタート！\r\n");

	while (1)
	{
		DataFetchHandle();
		if(NewDataFetched)
		{
			load_input(); // Load data input
			cnn_start(); // Start CNN processing
			while (cnn_time == 0)
				MXC_LP_EnterSleepMode(); // Wait for CNN
			cnn_unload((uint32_t *) ml_data32);

			#ifdef CNN_INFERENCE_TIMER
				printf("Inference completed, output=[%d, %d], elapsed time: %uus.\r\n", *(int8_t*)ml_data32, *((int8_t*)ml_data32+1), cnn_time);
			#elif
		  		printf("Inference completed, output=[%d, %d].\r\n", *(int8_t*)ml_data32, *((int8_t*)ml_data32+1));
		  	#endif

		  	NewDataFetched = 0U;
		}
		ModeSwitchHandle();
	}
}

void Peripheral_Init(void)
{
    MXC_GPIO_RegisterCallback(&pb_pin[0], GPIO_ISR, (void*)&pb_pin[0]);
    MXC_GPIO_IntConfig(&pb_pin[0], MXC_GPIO_INT_FALLING);
    MXC_GPIO_EnableInt(pb_pin[0].port, pb_pin[0].mask);
    MXC_NVIC_SetVector(MXC_UART_GET_IRQ(0), UART0_Handler);
    NVIC_EnableIRQ(MXC_UART_GET_IRQ(0));
    NVIC_EnableIRQ(MXC_GPIO_GET_IRQ(MXC_GPIO_GET_IDX(pb_pin[0].port)));
}

void Peripheral_Reconfig(void)
{
	MXC_UART_AbortTransmission(MXC_UART_GET_UART(0));
	if(WorkMode == 1U)
	{
		// Enable peripheral, enable CNN interrupt, turn on CNN clock
		// CNN clock: APB (50 MHz) div 1
		cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);
		cnn_init(); // Bring state machine into consistent state
		cnn_load_weights(); // Load kernels
		cnn_load_bias();
		cnn_configure(); // Configure state machine

		uart0_trans_req.uart = MXC_UART_GET_UART(0);
		uart0_trans_req.rxData = (uint8_t*)RecvBuffer;
		uart0_trans_req.rxLen = 9;
		uart0_trans_req.txLen = 0;
		uart0_trans_req.callback = UARTRxCallback;
		MXC_UART_TransactionAsync(&uart0_trans_req);
		LED_On(LED1);
	}
	else
	{
		cnn_disable(); // Shut down CNN clock, disable peripheral
		LED_Off(LED1);
	}

}

void DataFetchHandle(void)
{
	if(WorkMode == 1U)
	{
		if(HostRequest)
		{
			printf("Echo\r\n");
			uart0_trans_req.rxLen = sizeof(RecvBuffer);
			if(MXC_UART_Transaction(&uart0_trans_req) == E_NO_ERROR)
			{
				printf("Data received.\r\n");
				NewDataFetched = 1U;
			}
			else
			{
				printf("Transmission failed.\r\n");
			}
			HostRequest = 0U;
			uart0_trans_req.rxLen = 9;
			MXC_UART_TransactionAsync(&uart0_trans_req);
		}
	}
}

void ModeSwitchHandle(void)
{
	if(SwitchRequest)
	{
		WorkMode = (WorkMode + 1U) % 2U;
		Peripheral_Reconfig();
		printf("Mode %u selected.\r\n", WorkMode);
//		AccGyrRequest = 0U;
//		MagRequest = 0U;
		HostRequest = 0U;
		RecvBufferPTR = 0U;
		SwitchRequest = 0U;
	}
}

void UART0_Handler(void)
{
    MXC_UART_AsyncHandler(MXC_UART_GET_UART(0));
}

void UARTRxCallback(mxc_uart_req_t *req, int result)
{
	if(req == &uart0_trans_req)
	{
		if(WorkMode==1U && strstr((char*)RecvBuffer, "Connect"))
		{
			HostRequest = 1U;
		}
	}
}

void GPIO_ISR(void *cbdata)
{
	mxc_gpio_cfg_t *cfg = cbdata;
	if(cfg == &pb_pin[0])
	{
		SwitchRequest = 1U;
	}
}

/***
 *      _        _    ____    _____ 
 *     | |      / \  | __ )  |___ / 
 *     | |     / _ \ |  _ \    |_ \ 
 *     | |___ / ___ \| |_) |  ___) |
 *     |_____/_/   \_\____/  |____/ 
 *                                  
 */

#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_emc.h"
#include "pin_mux.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "Math.h"
#include "eGFX.h"
#include "eGFX_Driver.h"
#include "FONT_5_7_1BPP.h"
#include "OCR_A_Extended__20px__Bold__SingleBitPerPixelGridFit_1BPP.h"
#include "pin_mux.h"
#include "fsl_device_registers.h"
#include "fsl_i2c.h"
#include "fsl_i2s.h"
#include "fsl_wm8904.h"
#include "Audio.h"


/*

In this lab,   you will test the onboard audio CODEC.   If you look at the Schematic for the LPCXpresso54608 
board,   you will find that there is a CODEC (combined ADC and DAC) hooked to microcontroller through 2 I2S ports.

Some Data converters are easy to set up and use as they only require a few hardware configuration pins.   The WM8904
has a more complicated startup procedure.   In addition to the I2S ports for the ADC and DAC,   there is a connection 
to the micocontroller via an I2C connection.   I2C is a standard prototcol for inter chip communication.   The
Wikipedia page is a good place to start.

This Lab will give you an initialization function for the CODEC.   The intialization function will be commented so you can
follow along.

Once the CODEC is initialized,   you will have 2 interrupts handlers to work with.  One will be used for the ADC and the other for the DAC.
The CODEC will be used in such a way that we can do per-sample real time processing.

*/


volatile uint32_t NextSampleOut = 0;

/*
	This union will be used to split 32-bit fifo data into 2 int16_t samples.
	Unions are uses to overlay several variable across the same memory.
*/
typedef union 
{
	uint32_t Data;
	int16_t Channel[2];
	
}I2S_FIFO_Data_t;
/*
	Special Note!!  The routines betwen are the interrupt handlers
for Flexcomm 6 and 7.    We are directly processing the data in the interrupts.
I commented out the default handlers in fsl_flexcomm.c   The NXP I2s routines for
transmitting and recieving data are quiet bulky.  We are going define our own interrupt handlers here.

*/

/***
 *      ___ ____  ____    _______  __  ___       _                             _   
 *     |_ _|___ \/ ___|  |_   _\ \/ / |_ _|_ __ | |_ ___ _ __ _ __ _   _ _ __ | |_ 
 *      | |  __) \___ \    | |  \  /   | || '_ \| __/ _ \ '__| '__| | | | '_ \| __|
 *      | | / __/ ___) |   | |  /  \   | || | | | ||  __/ |  | |  | |_| | |_) | |_ 
 *     |___|_____|____/    |_| /_/\_\ |___|_| |_|\__\___|_|  |_|   \__,_| .__/ \__|
 *                                                                      |_|        
 */
 
void FLEXCOMM6_DriverIRQHandler(void)
{
    if (I2S0->FIFOINTSTAT & I2S_FIFOINTSTAT_TXLVL_MASK)
    {
        /*
					NextSampleOut Holds the last value from the I2S RX Interrupt.
				  It is also ready in the "packed" FIFO format
			  */
				I2S0->FIFOWR = NextSampleOut;
		
				 /* Clear TX level interrupt flag */
        I2S0->FIFOSTAT = I2S_FIFOSTAT_TXLVL(1U);
		}
}

/***
 *      ___ ____  ____    ____  __  __  ___       _                             _   
 *     |_ _|___ \/ ___|  |  _ \ \ \/ / |_ _|_ __ | |_ ___ _ __ _ __ _   _ _ __ | |_ 
 *      | |  __) \___ \  | |_) | \  /   | || '_ \| __/ _ \ '__| '__| | | | '_ \| __|
 *      | | / __/ ___) | |  _ <  /  \   | || | | | ||  __/ |  | |  | |_| | |_) | |_ 
 *     |___|_____|____/  |_| \_\/_/\_\ |___|_| |_|\__\___|_|  |_|   \__,_| .__/ \__|
 *                                                                       |_|        
 */
void FLEXCOMM7_DriverIRQHandler(void)
{
		register float LeftChannel;
		register float RightChannel;
		register float ALLChannels;
		
	  I2S_FIFO_Data_t FIFO_Data; 
	
     /* Clear RX level interrupt flag */
     I2S1->FIFOSTAT = I2S_FIFOSTAT_RXLVL(1U);
	
	   /*
				Read the Recieve FIFO.   Data is packed as two samples in one 32-bit word.  We will immediately store the data
				in a variable that is used is the transmit routine to send incoming data back out.
		 */
	    FIFO_Data.Data = I2S1->FIFORD;
	    NextSampleOut = FIFO_Data.Data; //dump the data back out!
	
	  /*
			In the configuration for this lab,  2 channels of data are packed
			in one 32-bit word.  The Right Channel is in the upper 16-bits and the Left-Channel in the lower
		  Notice between we can use a "union" (I2S_FIFO_Data_t) to read the data in as 32-bit and access it as two 16-bit signed numbers.
	  */
	   
	   LeftChannel = (float)(FIFO_Data.Channel[0])/32768.0f;
	   RightChannel = (float)(FIFO_Data.Channel[1])/32768.0f;
		 ALLChannels = (LeftChannel + RightChannel)/32768.0f;
		/*
			Do something with the Left and Right channel here
		
		*/
	
		
}

int main(void)
{

    CLOCK_EnableClock(kCLOCK_InputMux);
		
    CLOCK_EnableClock(kCLOCK_Iocon);
	
    CLOCK_EnableClock(kCLOCK_Gpio0);
  
    CLOCK_EnableClock(kCLOCK_Gpio1);

  	/* USART0 clock */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* Initialize the rest */
    BOARD_InitPins();
	
    BOARD_BootClockRUN();
  
    BOARD_InitDebugConsole();

		BOARD_InitSDRAM();

		eGFX_InitDriver();

      /*
				This function initializes the WM8904 CODEC and two I2S ports to send and recieve audio.
				Read through the comments in the function. (See Audio.c)
			*/

   	InitAudio_CODEC();
	
		while(1)
		{
				/*
						Audio Data is processed in the IRQ routine.   Do what you want here to display data, etc.
				*/
		   eGFX_ImagePlane_Clear(&eGFX_BackBuffer);
			
			 eGFX_printf(&eGFX_BackBuffer,
										200,250,   //The x and y coordinate of where to draw the text.
										&FONT_5_7_1BPP,   //Long font name!
									  "[Hello World!]");
							
			
			eGFX_Dump(&eGFX_BackBuffer);
		}
}




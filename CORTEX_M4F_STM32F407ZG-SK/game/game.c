#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx_usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "game.h"

#define __DEBUG

//Player1
int16_t player1X = 10;
int16_t player1Y = 10;
uint16_t player1W = 60;
uint16_t player1H = 10;
uint8_t player1IsReversed = 1;

//Player2
int16_t player2X = LCD_PIXEL_WIDTH - 20;
int16_t player2Y = LCD_PIXEL_HEIGHT - 20;
uint16_t player2W = 60;
uint16_t player2H = 10;
int8_t player2IsReversed = 0;

//Ball
uint16_t ballSize = 5;
int16_t ballX = ( LCD_PIXEL_WIDTH - 5 ) / 2;
int16_t ballY = ( LCD_PIXEL_HEIGHT - 5 ) / 2;
int16_t ballVX = 5;
int16_t ballVY = 5;
uint8_t ballIsRun = 0;

//Mode
uint8_t demoMode = 0;
#define PLAYER1_IS_NPC

void
BallReset()
{
	ballX = ( LCD_PIXEL_WIDTH - 5 ) / 2;
	ballY = ( LCD_PIXEL_HEIGHT - 5 ) / 2;

	ballVX = 5;
	ballVY = 5;

	ballIsRun = 1;
}

void uart1Init(void)
{
    USART_InitTypeDef   uartConfig;
    GPIO_InitTypeDef    GPIO_InitStructure;
    
    uartConfig.USART_BaudRate = 115200;
    uartConfig.USART_WordLength = USART_WordLength_8b;
    uartConfig.USART_StopBits = USART_StopBits_1;
    uartConfig.USART_Parity = USART_Parity_No;
    uartConfig.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    uartConfig.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    /* following step refs to stm32f4xx_usart.c */
    /* Enable peripheral clock */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    /* According to the USART mode, enable the GPIO clocks */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    /* Connect the pin to the desired peripherals' Alternate 
            Function (AF) using GPIO_PinAFConfig() function */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    /* Call GPIO_Init() function */
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    /* USART_Init() */
    USART_Init(USART1, &uartConfig);
    /* TODO: using the DMA mode  */
    /* Enable the USART */
    USART_Cmd(USART1, ENABLE);
    /* TODO: Enable DMA */
    
}

char* itoa(int value, char* result, int base)
{
	if (base < 2 || base > 36) {
		*result = '\0';
		return result;
	}
	char *ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while (value);

	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

void USART_puts(USART_TypeDef* USARTx, volatile char *s)
{
    while(*s)
    { // wait until data register is empty
        while( !(USARTx->SR & 0x00000040) );
        USART_SendData(USARTx, *s);
        *s++;
    }
}

void __DEBUG printLocation(int x, int y, int z)
{
    char    str[16];
    
    USART_puts(USART1, "X: ");
    USART_puts(USART1, itoa(x, str, 10));
    USART_puts(USART1, ", Y: ");
    USART_puts(USART1, itoa(y, str, 10));
    USART_puts(USART1, ", Z: ");
    USART_puts(USART1, itoa(z, str, 10));
    USART_puts(USART1, "\n\r");
    return;
}

void
GAME_EventHandler1()
{
	if( STM_EVAL_PBGetState( BUTTON_USER ) ){

		player1IsReversed = 0;

		while( STM_EVAL_PBGetState( BUTTON_USER ) );

		player1IsReversed = 1;
	}
}

void
GAME_EventHandler2()
{
    TP_STATE *ptp_state;
    
    ptp_state = IOE_TP_GetState();
    
    printLocation((int)ptp_state->X, (int)ptp_state->Y, (int)ptp_state->Z);
    
	if( ptp_state->TouchDetected ){

		if (ptp_state->X >= 120) {
		    if (ptp_state->X >= 180) {
		        player2IsReversed = 2;
		    } else {
		        player2IsReversed = 1;
		    }
		} else {
		    if (ptp_state->X >= 60) {
		        player2IsReversed = -1;
		    } else {
		        player2IsReversed = -2;
		    }
		}
		
	} else {
	    player2IsReversed = 0;
	}
}

void
GAME_EventHandler3()
{
	if( ballIsRun == 0 ){
		BallReset();
	}
}

void
GAME_Update()
{
	//Player1
	LCD_SetTextColor( LCD_COLOR_BLACK );
	LCD_DrawFullRect( player1X, player1Y, player1W, player1H );
	LCD_DrawFullRect( player2X, player2Y, player2W, player2H );

	if( demoMode == 0 ){
	    
#ifndef PLAYER1_IS_NPC
		if( player1IsReversed )
			player1X -= 5;
		else
			player1X += 5;

		if( player1X <= 0 )
			player1X = 0;
		else if( player1X + player1W >= LCD_PIXEL_WIDTH )
			player1X = LCD_PIXEL_WIDTH - player1W;
#else			
		//Player1 move
			if( ballVY < 0 ){
				if( player1X + player1W/2 < ballX + ballSize/2 ){
					player1X += 6;
				}
				else{
					player1X -= 6;
				}
			}

			if( player1X <= 0 )
				player1X = 0;
			else if( player1X + player1W >= LCD_PIXEL_WIDTH )
				player1X = LCD_PIXEL_WIDTH - player1W;
#endif			

		//Player2
		switch (player2IsReversed) {
		    case 1:
		        player2X += 5;
		        break;
		    case 2:
		        player2X += 7;
		        break;
		    case -1:
		        player2X -= 5;
		        break;
		    case -2:
		        player2X -= 7;
		        break;
		    default:
		        break;
		}

		if( player2X <= 0 )
			player2X = 0;
		else if( player2X + player2W >= LCD_PIXEL_WIDTH )
			player2X = LCD_PIXEL_WIDTH - player2W;

		//Ball
		if( ballIsRun == 1 ){

			LCD_SetTextColor( LCD_COLOR_BLACK );
			LCD_DrawFullRect( ballX, ballY, ballSize, ballSize );

			//Touch wall
			ballX += ballVX;
			if( ballX <= 0 ){
				ballX = 0;
				ballVX *= -1;
			}
			else if( ballX + ballSize >= LCD_PIXEL_WIDTH ){
				ballX = LCD_PIXEL_WIDTH - ballSize;
				ballVX *= -1;
			}

			//PONG!
			ballY += ballVY;
			if( ballY + ballSize >= player2Y ){
				if( ballX + ballSize >= player2X && ballX <= player2X + player2W ){
					if( ballX - ballSize <= player2Y + player2W/4 ){
						ballVY =-3;
						ballVX =-7;
					}
					else if( ballX >= player2Y + player2W - player2W/4 ){
						ballVY =-3;
						ballVX = 7;
					}
					else if( ballX + ballSize < player2Y + player2W/2 ){
						ballVY =-7;
						ballVX =-3;
					}
					else if( ballX > player2Y + player2W/2 ){
						ballVY =-7;
						ballVX = 3;
					}
					else{
						ballVY =-9;
						ballVX = 0;
					}
				}
				else
					BallReset();
			}

			if( ballY <= player1Y + player1H ){
					if( ballX + ballSize >= player1X && ballX <= player1X + player1W ){
						if( ballX - ballSize <= player1Y + player1W/4 ){
							ballVY = 3;
							ballVX =-7;
						}
						else if( ballX >= player1Y + player1W - player1W/4 ){
							ballVY = 3;
							ballVX = 7;
						}
						else if( ballX + ballSize < player1Y + player1W/2 ){
							ballVY = 7;
							ballVX =-3;
						}
						else if( ballX > player1Y + player1W/2 ){
							ballVY = 7;
							ballVX = 3;
						}
						else{
							ballVY = 9;
							ballVX = 0;
						}
					}
					else
						BallReset();
				}
			}
		}
		else{	//if demoMode == 1

			//Player1 move
			if( ballVY < 0 ){
				if( player1X + player1W/2 < ballX + ballSize/2 ){
					player1X += 8;
					player2X += 2;
				}
				else{
					player1X -= 8;
					player2X -= 2;
				}
			}

			//Player2 move
			if( ballVY > 0 ){
				if( player2X + player2W/2 < ballX + ballSize/2 ){
					player1X += 2;
					player2X += 8;
				}
				else{
					player1X -= 2;
					player2X -= 8;
				}

			}

			if( player1X <= 0 )
				player1X = 0;
			else if( player1X + player1W >= LCD_PIXEL_WIDTH )
				player1X = LCD_PIXEL_WIDTH - player1W;

			if( player2X <= 0 )
				player2X = 0;
			else if( player2X + player2W >= LCD_PIXEL_WIDTH )
				player2X = LCD_PIXEL_WIDTH - player2W;


			//Ball
			if( ballIsRun == 1 ){

				LCD_SetTextColor( LCD_COLOR_BLACK );
				LCD_DrawFullRect( ballX, ballY, ballSize, ballSize );

				//Touch wall
				ballX += ballVX;
				if( ballX <= 0 ){
					ballX = 0;
					ballVX *= -1;
				}
				else if( ballX + ballSize >= LCD_PIXEL_WIDTH ){
					ballX = LCD_PIXEL_WIDTH - ballSize;
					ballVX *= -1;
				}

				//PONG!
				ballY += ballVY;
				if( ballY + ballSize >= player2Y ){
					if( ballX + ballSize >= player2X && ballX <= player2X + player2W ){
					if( ballX - ballSize <= player2Y + player2W/4 ){
						ballVY =-3;
						ballVX =-7;
					}
					else if( ballX >= player2Y + player2W - player2W/4 ){
						ballVY =-3;
						ballVX = 7;
					}
					else if( ballX + ballSize < player2Y + player2W/2 ){
						ballVY =-7;
						ballVX =-3;
					}
					else if( ballX > player2Y + player2W/2 ){
						ballVY =-7;
						ballVX = 3;
					}
					else{
						ballVY =-9;
						ballVX = 0;
					}
				}
				else
					BallReset();
			}

			if( ballY <= player1Y + player1H ){
				if( ballX + ballSize >= player1X && ballX <= player1X + player1W ){
					if( ballX - ballSize <= player1Y + player1W/4 ){
						ballVY = 3;
						ballVX =-7;
					}
					else if( ballX >= player1Y + player1W - player1W/4 ){
						ballVY = 3;
						ballVX = 7;
					}
					else if( ballX + ballSize < player1Y + player1W/2 ){
						ballVY = 7;
						ballVX =-3;
					}
					else if( ballX > player1Y + player1W/2 ){
						ballVY = 7;
						ballVX = 3;
					}
					else{
						ballVY = 9;
						ballVX = 0;
					}
				}
				else
					BallReset();
			}
		}
	}
}

	void
GAME_Render()
{
	LCD_SetTextColor( LCD_COLOR_WHITE );
	LCD_DrawFullRect( player1X, player1Y, player1W, player1H );
	LCD_DrawFullRect( player2X, player2Y, player2W, player2H );
	LCD_DrawFullRect( ballX, ballY, ballSize, ballSize );
	LCD_DrawLine( 10, LCD_PIXEL_HEIGHT / 2, LCD_PIXEL_WIDTH - 20, LCD_DIR_HORIZONTAL );
}

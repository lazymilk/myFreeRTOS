#ifndef GAME_H
#define GAME_H

void uart1Init(void);
void USART_puts(USART_TypeDef* USARTx, volatile char *s);
char* itoa(int value, char* result, int base);

void
EventHandler1();

void
EventHandler2();

void
EventHandler3();

void
GAME_Update();

void
GAME_Render();

#endif

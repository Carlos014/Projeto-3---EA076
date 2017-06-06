/* ###################################################################
 **     Filename    : main.c
 **     Project     : Datalogger
 **     Processor   : MKL25Z128VLK4
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2017-05-11, 14:50, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 01.01
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */         
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "WAIT1.h"
#include "GI2C1.h"
#include "UART.h"
#include "ASerialLdd1.h"
#include "KSDK1.h"
#include "UTIL1.h"
#include "CLS1.h"
#include "CS1.h"
#include "CI2C1.h"
#include "WAIT2.h"
#include "IRout.h"
#include "ExtIntLdd1.h"
#include "PWM1.h"
#include "PwmLdd1.h"
#include "TU1.h"
#include "TI1.h"
#include "TimerIntLdd1.h"
#include "TU2.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

/* Flags globais para controle de processos da interrupcao */
volatile int flag_check_command = 0;
extern int rx;
extern int meas;
extern int timeh;

/* User includes (#include below this line is not maintained by Processor Expert) */

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */

void Write(char *s) {
	// Funcao para escrita de strings na UART.
	int j, f;
	for(j = 0;j < strlen(s);j++){
		UART_SendChar(s[j]);
		for(f=0;f<4000;f++);
	}
}

int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
	/* Write your local variable definition here */
	int buffer[26], buffert[6], i, x=0, k, f, value = 0, avg_25=0, avg_5=0, count=0, numsize=0, R=0, duty=0, error=0;
	extern int sum;
	char outbuffer[50], RPM[10], c;
	/*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
	PE_low_level_init();
	/*** End of Processor Expert internal initialization.                    ***/

	/* Write your code here */
	
	PWM1_SetDutyMS(100); // Inicializa com o motor parado.
	sprintf(outbuffer, "Digite a velocidade desejada em RPM: ");
	Write(outbuffer);
	
	while(!x) {
		/* Ao receber evento da UART */
		if(rx) {
			// Caso não tenham havido problemas na recepção do caracter.
			if(UART_RecvChar(&c) == ERR_OK) {
				if(c == '\r') {
					x = 1;
					RPM[count] = '\0';
					UART_SendChar('\n');
					WAIT1_Waitms(50);
					UART_SendChar('\r');					
				}
				else {
					RPM[count] = c;
					UART_SendChar(c); // Eco para melhor visualização.
					count++;					
				}
			}
			rx = 0;
		}
	}

	while(x) {
		numsize = strlen(RPM); // Pega a magnitude do valor de velocidade desejado.
		for(i=0;i<numsize;i++) {
			// Converte o caractere em um número e calcula a velocidade de referência.
			R += pow(10,(numsize-i-1))*(RPM[i]-'0'); 
		}
		x=0;
	}

	// Pega o valor de duty cycle mais próximo da velocidade de referência R.
	if(R < 2250) duty=90;
	else if(R < 3960) duty=80;
	else if(R < 4728) duty=70;
	else if(R < 5140) duty=60;
	else if(R < 5430) duty=50;
	else if(R < 5630) duty=40;
	else if(R < 5770) duty=30;
	else if(R < 5880) duty=20;
	else if(R < 5984) duty=10;
	else if(R < 6070) duty=0;

	PWM1_SetDutyMS(duty);
	for(i=0;i<26;i++) buffer[i]=0;
	for(i=0;i<6;i++) buffert[i]=0;

	i=0; k=0;
	for(;;) {
		// Dado o sinal de interrupção do timer, é tomada uma medida do sensor IR.
		if (meas){ 
			meas=0;
			if(timeh >= 5){ // Espera 5 interrupções de tempo (0.5s) para executar a realimentação. 
				error = R - avg_5*60; // Calcula o erro entre a referência e a velocidade medida.
				// Decide a ação de controle a ser tomada a partir do erro.
				if(error < -59) { 
					// Caso a velocidade medida seja maior que a de referência, diminui a rotação do motor.
					if(duty < 99) duty++;
					PWM1_SetDutyMS(duty);
				}
				else if(error > 59){
					// Caso a velocidade medida seja menor que a de referência, aumenta a rotação do motor.
					if(duty > 0) duty--;
					PWM1_SetDutyMS(duty);
				}
				timeh = 0;
			}
			if(i==25) i=0; // contadores de buffer circular
			if(k==5)  k=0;

			avg_25  -= buffer[i];  // avg_5 contém 0,5s de medidas tomadas.
			avg_5 -= buffert[k]; // avg_25 contém 2,5s de medidas tomadas.  

			value = sum*300; // Conversão do valor para RPM. Hélice tem 2 pás (Giros = interrupções/2).
			// nº interrupções/100ms = 600* interrup/min = 300* giros/min.
			buffer[i] = sum;
			buffert[k] = sum;
			sum = 0;

			avg_25  += buffer[i]; 
			avg_5 += buffert[k];

			sprintf(outbuffer, "%.4d %.4d %.4d %.2d \r", value, avg_5*60, avg_25*12, duty);
			Write(outbuffer);
			i++; k++;
		}
	}
	/*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
}

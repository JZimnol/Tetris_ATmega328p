#define F_CPU 8000000UL      /* 8 MHz */

#include <avr/io.h>          /* AVR core lib */
#include <avr/interrupt.h>   /* AVR interrupt lib */
#include "Tetris.h"

int main(void) {
    
    buttonsInit();
    SPI_MasterInit();
    TIM0_Init();
	sei();			  

    displayPLAY();
    while( !PC1_PUSHED )
        ;

	timer_ms = 0;
    while( timer_ms < 250 )
        ;

    framebufferInit();
    displayNewBlock();

    /* buttons control has been implemented using polling, but there are 
       no contraindications to use interrupts */
    while(1) {  
        if( timer_ms > ((500 - lvl)<<1) ) {
            moveBlockDown();
            timer_ms = 0;
        }
        if( PC2_PUSHED && debounce > 300 ) {
            moveBlockLeft();
			debounce = 0;
        }
        if( PC3_PUSHED && debounce > 300 ) {
            moveBlockDown();
			debounce = 0;
        }
        if( PC0_PUSHED && debounce > 300 ) {
            moveBlockRight();
			debounce = 0;
        }
        if( PC1_PUSHED && debounce > 300 ) {
            rotateBlockRight();
			debounce = 0;
        }
    }
    return (0);
}

/* interruption every 0.512 ms */
ISR(TIMER0_COMPA_vect) {
        timer_ms++;
		debounce++;
		SPI_MasterTransmit_16bit(~frameBuffer.main[iteratorSPI]);  /* send frame buffer */
        /* 0x80000000 == 0b10000000000000000000000000000000 */
		SPI_MasterTransmit_32bit(0x80000000>>iteratorSPI);         /* send one-hot row */
		iteratorSPI++;
		if (iteratorSPI == 32) iteratorSPI = 0;
}
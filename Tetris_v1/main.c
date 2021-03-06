#define F_CPU 8000000UL       /* 8 MHz */

// #include <inttypes.h>      /* optional */
// #include <unistd.h>        /* optional */
#include <stdlib.h>           
#include <avr/io.h>           /* AVR lib */
#include <avr/interrupt.h>
#include <util/delay.h>
// #include <avr/iom328p.h>   ///< comment before compilation; it's used sometimes to
                              ///< gain access to AVR variables

/***********************************************\
* Macro declarations 
\***********************************************/
#define LT_ON PORTB |= (1<<PB2)       /* latch on */
#define LT_OFF PORTB &= ~(1<<PB2)     /* latch off */

#define PC0_PUSHED !(PINC & (1<<PC0)) 	/* rotate */
#define PC1_PUSHED !(PINC & (1<<PC1)) 	/* right */
#define PC2_PUSHED !(PINC & (1<<PC2)) 	/* down */
#define PC3_PUSHED !(PINC & (1<<PC3)) 	/* left */

#define BLOCK_I 0
#define BLOCK_J 1
#define BLOCK_L 2
#define BLOCK_O 3
#define BLOCK_S 4
#define BLOCK_T 5
#define BLOCK_Z 6

/***********************************************\
* Global variables 
\***********************************************/
uint16_t fb_main[32];         /* main frame buffer */
uint16_t fb_blocks[32];       /* blocks frame buffer */
uint16_t fb_floor[32];        /* board frame buffer (walls, floor, static blocks etc.) */
uint16_t fb_nextblock[2];     /* next block frame buffer */

uint16_t fb_points100[5];     /* hundreds digit frame buffer */
uint16_t fb_points010[5];     /* tens digit frame buffer */
uint16_t fb_points001[5];     /* unity digit frame buffer */

uint8_t cordx = 8;            /* x coordinate of core field of block */
uint8_t cordy = 8;            /* y coordinate of core field of block */

uint16_t points = 0;          /* points counter */
uint8_t hundreds = 0;
uint8_t tens = 0;
uint8_t unity = 0;

uint8_t nextblock;
uint8_t currentblock;
uint8_t lvl;                  /* level counter; it's used to calculate the speed of falling of the block */

volatile uint16_t tim_ms = 0;      /* timer variable */
volatile uint8_t lock = 0;         /* lock is used for locking infinite lopp during interruption */
volatile uint8_t iterator = 0;     /* timer variable */
volatile uint16_t debounce = 0;    /* variable used to get the button debounce effect */

/***********************************************\
* Function declarations 
\***********************************************/
void SPI_MasterInit();                               /* initialize SPI */
void SPI_MasterTransmit_16bit(uint16_t data_bytes);  /* send frame buffer */
void SPI_MasterTransmit_32bit(uint32_t data_bytes);  /* send column number */
void TIM0_Init();                                    /* initialize timer */
void updatepoints();                                 /* increment points */
void deletelevel();                                  /* delete row full of blocks */
void nextblockdisplay();                             /* display next block */
void newblock();                                     /* choose and display new block */
int spacedown();                                     /* space down validation */
void movedown();                                     /* move block down */
int spaceleft();                                     /* space left validation */
void moveleft();                                     /* move block left */
int spaceright();                                    /* space right validation */
void moveright();                                    /* move block right */
void updateframebuffer();                            /* sum all frame buffers */
void rotateRight();                                  /* rotate block */
void fb_init();                                      /* initialize frame buffer */
void gameover();                                     /* display some things after gameover */

/***********************************************\
* Main 
\***********************************************/
int main(void) {
    DDRC = 0;         /* initialize buttons */
    PORTC = 255;
	
    SPI_MasterInit();   /* initialize SPI */
    TIM0_Init();        /* initialize TIMER */
    sei();	        /* enable interruptions */
	
    /* display "PLAY" untill any button == 1 */
    fb_floor[14] = 0xe8ea;
    fb_floor[15] = 0xa8aa;
    fb_floor[16] = 0xe8e4;
    fb_floor[17] = 0x88a4;
    fb_floor[18] = 0x8ea4;
    updateframebuffer();
    while (!PC1_PUSHED)
	    ;

    fb_init();
    nextblock = tim_ms%7;
    newblock();
    updateframebuffer();

    while (~lock) {
        lock = 1;  
        if (tim_ms > ((500 - lvl)<<1)) {
            movedown();
            tim_ms = 0;
        }
        if (PC2_PUSHED && debounce > ((500 - lvl)<<1)/4) {
            moveleft();
	    debounce = 0;
        }
        if (PC3_PUSHED && debounce > ((500 - lvl)<<1)/4) {
            movedown();
	    debounce = 0;
        }
        if (PC0_PUSHED && debounce > ((500 - lvl)<<1)/4) {
            moveright();
	    debounce = 0;
        }
        if (PC1_PUSHED && debounce > ((500 - lvl)<<1)/4) {
            rotateRight();
	    debounce = 0;
        }
    }
    return (0);
}
/***********************************************\
* Function codes
\***********************************************/
/**
 * @brief Initialize SPI transmission
 */
void SPI_MasterInit(void) {
    /* Set MOSI, CS and SCK output, all others input */
    DDRB = (1<<PB3) | (1<<PB5) | (1<<PB2);
    /* Enable SPI, Master, set clock rate fck/16, LSB first */
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<DORD);
}
/**
 * @brief Send two data bytes through SPI
 *
 * @param Two data bytes
 */
void SPI_MasterTransmit_16bit(uint16_t data_bytes) {
    /* Start transmission of first byte (most significant byte) */
    SPDR = data_bytes>>8;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF)))
	    ;
    /* Start transmission of second byte (least significant byte) */
    SPDR = data_bytes;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF)))
	    ;
}
/**
 * @brief Send four data bytes through SPI
 *
 * @param Four data bytes
 */
void SPI_MasterTransmit_32bit(uint32_t data_bytes) {
    /* Start transmission of first byte (most significant byte) */
    SPDR = data_bytes>>24;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF)))
	    ;
    /* Start transmission of second byte */
    SPDR = data_bytes>>16;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF)))
	    ;
    /* Start transmission of third byte */
    SPDR = data_bytes>>8;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF)))
	    ;
    /* Start transmission of last byte (least significant byte) */
    SPDR = data_bytes;
    /* Wait for transmission complete */
    while(!(SPSR & (1<<SPIF)))
	    ;
    /* Latch the transmission */
    LT_ON;
    LT_OFF;
}
/**
 * @brief Initialize TIMER0. Trigger interruptions every 0.512 ms
 */
void TIM0_Init() {			            
    TCCR0A = (1<<WGM01);	         /* set CTC mode */
    TCCR0B = (1<<CS00) | (1<<CS02);  /* fck/1024 */
    OCR0A = 3;
    TIMSK0 = (1<<OCIE0A);
}
/**
 * @brief Delete every full row. 
 *	 If any level has been deleted, call updatepoints()
 *  1. scan levels and find the full one
 *  2. if you find any, delete level, increment points
 *  3. move whole board one block down 
 */
void deletelevel() {
    uint8_t i = 31;
    while (i>7) {
        if (fb_floor[i] == 0xffff) {
            for (uint8_t j=i; j>8; j--) fb_floor[j] = fb_floor[j - 1];
            fb_floor[8] = 0xc003;
            updatepoints();
            continue;
        }
        i--;
    }
}
/**
 * @brief End the game, display user's points.
 */
void gameover() {
    tim_ms = 0;
    while (tim_ms < 500)
	;
    for (uint8_t j=0; j<5; j++) {
        for (uint8_t i=0; i<32; ++i) fb_main[i] = 0xffff;
        tim_ms = 0;
        while (tim_ms < 500)
		;
        for (uint8_t i=0; i<32; ++i) fb_main[i] = 0x0000;
        tim_ms = 0;
        while (tim_ms < 500)
		;
    }
    points--;
    for (uint8_t i=0; i<32; ++i) {
        fb_floor[i] = 0x0000;
        fb_blocks[i] = 0x0000;
    }
    fb_nextblock[0] = 0x0000;
    fb_nextblock[1] = 0x0000;
    updatepoints();
	
    for (;;)  /* display points until RESET is pressed */
	;                            
}
/**
 * @brief Determine if there is space under the block
 *
 * @return True or False.
 */
int spacedown() {
    if (fb_blocks[31]) {
        return 0;
    }
    for (int8_t i=30; i>=7; i--) {
        if (fb_blocks[i] & fb_floor[i + 1]) {
            return 0;
	}
    }
    return 1;
}
/**
 * @brief Move block one pixel down.
 */
void movedown() {
    if (spacedown()) {
        for (int8_t i=31; i>7; i--) {
            fb_blocks[i] = fb_blocks[i - 1];
        }
        cordy++;
        updateframebuffer();
    } else {
        for (int8_t i=31; i>7; i--) {
            fb_floor[i] += fb_blocks[i];
        }
        deletelevel();
        newblock();
    }
}
/**
 * @brief Determine if there is space to the left the block
 *
 * @return True or False.
 */
int spaceleft() {
    for (uint8_t i=-2; i<=2; ++i) {
        if (fb_blocks[cordy + i] & fb_floor[cordy + i] >> 1) {
            return 0;
	}
    }
    return 1;
}
/**
 * @brief Move block one pixel to the left.
 */
void moveleft() {
    if (spaceleft()) {
        for (int8_t y=-1; y<3; y++) {
            fb_blocks[cordy + y] = (fb_blocks[cordy + y] << 1);
        }
        cordx++;
        updateframebuffer();
    }
}
/**
 * @brief Determine if there is space to the right the block
 *
 * @return True or False.
 */
int spaceright() {
    for (uint8_t i=-2; i<=2; ++i) {
        if (fb_blocks[cordy + i] & fb_floor[cordy + i] << 1) {
            return 0;
	}
    }
    return 1;
}
/**
 * @brief Move block one pixel to the right.
 */
void moveright() {
    if (spaceright()) {
        for (int8_t y=-1; y<3; y++) {
            fb_blocks[cordy + y] = (fb_blocks[cordy + y] >> 1);
        }
        cordx--;
        updateframebuffer();
    }
}
/**
 * @brief Sum all frame buffers, store result in fb_main[]
 */
void updateframebuffer() {
    for (uint8_t i=0; i<32; i++) {
        fb_main[i] = fb_blocks[i] + fb_floor[i];
    }
    fb_main[3] = fb_nextblock[0];
    fb_main[4] = fb_nextblock[1];
    for (uint8_t i=1; i<6; i++) {
        fb_main[i] += fb_points100[i - 1] + fb_points010[i - 1] + fb_points001[i - 1];
    }
}
/**
 * @brief Initialize frame buffer at the begging of the game (after PLAY)
 */
void fb_init() {
    /* set unused bits of block and floor frame buffers to "0" */
    for (uint8_t i=0; i<8; ++i) {
        fb_blocks[i] = 0;
        fb_floor[i] = 0;
    }
    /* set block and floor frame buffers */
    for (uint8_t i=7; i<32; ++i) {
        fb_blocks[i] = 0;
        fb_floor[i] = 0xc003;
    }
    fb_floor[7] = 0xffff;
	
    /* display "000" */
    fb_points100[0] = 0xe000;
    fb_points100[1] = 0xa000;
    fb_points100[2] = 0xa000;
    fb_points100[3] = 0xa000;
    fb_points100[4] = 0xe000;
    fb_points010[0] = 0x0e00;
    fb_points010[1] = 0x0a00;
    fb_points010[2] = 0x0a00;
    fb_points010[3] = 0x0a00;
    fb_points010[4] = 0x0e00;
    fb_points001[0] = 0x00e0;
    fb_points001[1] = 0x00a0;
    fb_points001[2] = 0x00a0;
    fb_points001[3] = 0x00a0;
    fb_points001[4] = 0x00e0;
}
/**
 * @brief Turn the block clockwise 
 */
void rotateRight() {
	if (currentblock) { /* if currentblock != BLOCK_I */
		int8_t crossArr[5] = { };
		int8_t diamondArr[4] = { };
		int8_t c = 0;
		int8_t d = 0;
		for (int8_t i=-1; i<2; i++) {
			for (int8_t j=-1; j<2; j++) {
				if (fb_blocks[cordy + i] & (1 << (cordx + j))) {
					if ((i + j) % 2) {
						diamondArr[c] = 1;
						c++;
					} else {
						crossArr[d] = 1;
						d++;
					}
				} else {
					if ((i + j) % 2) {
						diamondArr[c] = 0;
						c++;
					} else {
						crossArr[d] = 0;
						d++;
					}
				}
			}
		}
		int8_t newTable[9] = { };

		newTable[0] = crossArr[1];
		newTable[1] = diamondArr[2];
		newTable[2] = crossArr[4];
		newTable[3] = diamondArr[0];
		newTable[4] = crossArr[2];
		newTable[5] = diamondArr[3];
		newTable[6] = crossArr[0];
		newTable[7] = diamondArr[1];
		newTable[8] = crossArr[3];

		/* validation process */
		c = 1;
		uint8_t g;
		for (int8_t i=-1; i<2; i++) {
			g = fb_floor[cordy + i];
			for (int8_t j=-1; j<2; j++) {
				if (newTable[i * 3 + j + 4]) {
					if (g & (1 << (cordx + j))) {
						c = 0;  /* validation == failed */
					}
				}
			}
		}

		if (c) { /* if validation successful */
			for (int8_t i=-1; i<2; i++) {
				g = 0;
				for (int8_t j=-1; j<2; j++) {
					if (newTable[i * 3 + j + 4]) g = g ^ (1 << (cordx + j));
				}
				fb_blocks[cordy + i] = g;
			}
			updateframebuffer();
		}
	} else { /* case for BLOCK_I */
		/* check position */
		if (fb_blocks[cordy - 1] == (1 << cordx)) { /* vertical */
			/* check if validation vertical -> horizontal == successful */
			if (!((15 << (cordx - 2)) & fb_floor[cordy])) {
				fb_blocks[cordy - 1] = 0;
				fb_blocks[cordy] = 15 << (cordx - 2);
				fb_blocks[cordy + 1] = 0;
				fb_blocks[cordy + 2] = 0;
				updateframebuffer();
			}
		} else {  /* horizontal */
			int8_t c = 1;
			/* check if validation horizontal -> vertical == successful */
			for (int8_t i=-1; i<3; i++) {
				if (fb_floor[cordy + i] & (1 << cordx)) c = 0;
			}
			if (c) {
				for (int8_t i=-1; i<3; i++) fb_blocks[cordy + i] = 1 << cordx;
				updateframebuffer();
			}
		}
	}
}
/**
 * @brief Generate new block on the top of the board.
 */
void newblock() {
	/* delete previous block from block frame buffer */
	for (uint8_t i=-2; i<=2; ++i) fb_blocks[cordy + i] = 0x0000;
	
	/* assign coordinates of new core field of new block */
	cordx = 8;
	cordy = 8;
	currentblock = nextblock;
	nextblock = tim_ms%7;

	switch (currentblock) {
		case BLOCK_I:
			fb_blocks[8] = 0x03c0;
			fb_blocks[9] = 0x0000;
			break;
		case BLOCK_J:
			fb_blocks[8] = 0x0380;
			fb_blocks[9] = 0x0080;
			break;
		case BLOCK_L:
			fb_blocks[8] = 0x0380;
			fb_blocks[9] = 0x0200;
			break;
		case BLOCK_O:
			fb_blocks[8] = 0x0180;
			fb_blocks[9] = 0x0180;
			break;
		case BLOCK_S:
			fb_blocks[8] = 0x0180;
			fb_blocks[9] = 0x0300;
			break;
		case BLOCK_T:
			fb_blocks[8] = 0x0380;
			fb_blocks[9] = 0x0100;
			break;
		case BLOCK_Z:
			fb_blocks[8] = 0x0300;
			fb_blocks[9] = 0x0180;
			break;
	}
	nextblockdisplay();
	updateframebuffer();
	
	/* check if a new block has space to be spawned */
	for (uint8_t i=9; i>=8; i--) {
		if (fb_blocks[i] & fb_floor[i]) gameover();
	}
}
/**
 * @brief Generate next block.
 */
void nextblockdisplay() {
	switch (nextblock) {
		case BLOCK_I:
			fb_nextblock[0] = 0x000f;
			fb_nextblock[1] = 0x0000;
			break;
		case BLOCK_J:
			fb_nextblock[0] = 0x000e;
			fb_nextblock[1] = 0x0002;
			break;
		case BLOCK_L:
			fb_nextblock[0] = 0x000e;
			fb_nextblock[1] = 0x0008;
			break;
		case BLOCK_O:
			fb_nextblock[0] = 0x0006;
			fb_nextblock[1] = 0x0006;
			break;
		case BLOCK_S:
			fb_nextblock[0] = 0x0006;
			fb_nextblock[1] = 0x000c;
			break;
		case BLOCK_T:
			fb_nextblock[0] = 0x000e;
			fb_nextblock[1] = 0x0004;
			break;
		case BLOCK_Z:
			fb_nextblock[0] = 0x000c;
			fb_nextblock[1] = 0x0006;
			break;
	}
	updateframebuffer();
}
/**
 * @brief Uptade points counter.
 */
void updatepoints() {
	points++;
	
	/* use lvl variable to scale the speed of blocks */
	if (points < 100) lvl = points * 3;

	/* calculate very digit of points value */
	hundreds = points / 100;
	tens = points % 100;
	tens = tens / 10;
	unity = points % 10;

	/* display hundreds digit */
	switch (hundreds) {
		case 0:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0xa000;
			fb_points100[2] = 0xa000;
			fb_points100[3] = 0xa000;
			fb_points100[4] = 0xe000;
			break;
		case 1:
			fb_points100[0] = 0x4000;
			fb_points100[1] = 0xc000;
			fb_points100[2] = 0x4000;
			fb_points100[3] = 0x4000;
			fb_points100[4] = 0xe000;
			break;
		case 2:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0x2000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0x8000;
			fb_points100[4] = 0xe000;
			break;
		case 3:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0x2000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0x2000;
			fb_points100[4] = 0xe000;
			break;
		case 4:
			fb_points100[0] = 0xa000;
			fb_points100[1] = 0xa000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0x2000;
			fb_points100[4] = 0x2000;
			break;
		case 5:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0x8000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0x2000;
			fb_points100[4] = 0xe000;
			break;
		case 6:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0x8000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0xa000;
			fb_points100[4] = 0xe000;
			break;
		case 7:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0xa000;
			fb_points100[2] = 0x2000;
			fb_points100[3] = 0x2000;
			fb_points100[4] = 0x2000;
			break;
		case 8:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0xa000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0xa000;
			fb_points100[4] = 0xe000;
			break;
		case 9:
			fb_points100[0] = 0xe000;
			fb_points100[1] = 0xa000;
			fb_points100[2] = 0xe000;
			fb_points100[3] = 0x2000;
			fb_points100[4] = 0xe000;
			break;
	}
	
	/* display tens digit */
	switch (tens) {
		case 0:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0a00;
			fb_points010[2] = 0x0a00;
			fb_points010[3] = 0x0a00;
			fb_points010[4] = 0x0e00;
			break;
		case 1:
			fb_points010[0] = 0x0400;
			fb_points010[1] = 0x0c00;
			fb_points010[2] = 0x0400;
			fb_points010[3] = 0x0400;
			fb_points010[4] = 0x0e00;
			break;
		case 2:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0200;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0800;
			fb_points010[4] = 0x0e00;
			break;
		case 3:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0200;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0200;
			fb_points010[4] = 0x0e00;
			break;
		case 4:
			fb_points010[0] = 0x0a00;
			fb_points010[1] = 0x0a00;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0200;
			fb_points010[4] = 0x0200;
			break;
		case 5:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0800;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0200;
			fb_points010[4] = 0x0e00;
			break;
		case 6:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0800;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0a00;
			fb_points010[4] = 0x0e00;
			break;
		case 7:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0a00;
			fb_points010[2] = 0x0200;
			fb_points010[3] = 0x0200;
			fb_points010[4] = 0x0200;
			break;
		case 8:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0a00;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0a00;
			fb_points010[4] = 0x0e00;
			break;
		case 9:
			fb_points010[0] = 0x0e00;
			fb_points010[1] = 0x0a00;
			fb_points010[2] = 0x0e00;
			fb_points010[3] = 0x0200;
			fb_points010[4] = 0x0e00;
			break;
	}

	/* display unity digit */
	switch (unity) {
		case 0:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x00a0;
			fb_points001[2] = 0x00a0;
			fb_points001[3] = 0x00a0;
			fb_points001[4] = 0x00e0;
			break;
		case 1:
			fb_points001[0] = 0x0040;
			fb_points001[1] = 0x00c0;
			fb_points001[2] = 0x0040;
			fb_points001[3] = 0x0040;
			fb_points001[4] = 0x00e0;
			break;
		case 2:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x0020;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x0080;
			fb_points001[4] = 0x00e0;
			break;
		case 3:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x0020;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x0020;
			fb_points001[4] = 0x00e0;
			break;
		case 4:
			fb_points001[0] = 0x00a0;
			fb_points001[1] = 0x00a0;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x0020;
			fb_points001[4] = 0x0020;
			break;
		case 5:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x0080;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x0020;
			fb_points001[4] = 0x00e0;
			break;
		case 6:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x0080;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x00a0;
			fb_points001[4] = 0x00e0;
			break;
		case 7:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x00a0;
			fb_points001[2] = 0x0020;
			fb_points001[3] = 0x0020;
			fb_points001[4] = 0x0020;
			break;
		case 8:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x00a0;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x00a0;
			fb_points001[4] = 0x00e0;
			break;
		case 9:
			fb_points001[0] = 0x00e0;
			fb_points001[1] = 0x00a0;
			fb_points001[2] = 0x00e0;
			fb_points001[3] = 0x0020;
			fb_points001[4] = 0x00e0;
			break;
	}
	updateframebuffer();
}
/**
 * @brief Trigger and interrupt every 0.512 ms
 */
ISR(TIMER0_COMPA_vect) {
        tim_ms++;
	debounce++;
        lock=0;
	SPI_MasterTransmit_16bit(~fb_main[iterator]);   // send frame buffer
	SPI_MasterTransmit_32bit(0x80000000>>iterator); // send one-hot row 
	iterator++;
	if (iterator == 32) iterator = 0;
}

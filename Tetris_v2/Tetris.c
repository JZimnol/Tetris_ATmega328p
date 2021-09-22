/*
 * @file Tetris.c
 * @author: JZimnol
 * @brief File containing definitions for Tetris game
 */ 

#include <avr/io.h>   
#include "Tetris.h"

/*************************************************************************\
                                 VARIABLES
\*************************************************************************/

FrameBuffer frameBuffer;
Coordinates coords = {8, 8};

BlockType nextBlock = 0;
BlockType currentBlock = 0;

uint16_t lvl = 0;
uint16_t pointsCounter = 0;
volatile uint16_t timer_ms = 0;
volatile uint8_t iteratorSPI = 0;
volatile uint16_t debounce = 0;

/*************************************************************************\
                                 FUNCTIONS
\*************************************************************************/

void SPI_MasterInit() {
    /* Set MOSI, CS and SCK output, all others input */
    DDRB = (1<<PB3) | (1<<PB5) | (1<<PB2);
    /* Enable SPI, Master, set clock rate fck/16, LSB first */
    SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0) | (1<<DORD);
}

void SPI_MasterTransmit_16bit(uint16_t data_bytes) {
    /* Start transmission of first byte (most significant byte) */
    SPDR = data_bytes>>8;
    while( !(SPSR & (1<<SPIF)) )
        ;
    /* Start transmission of second byte (least significant byte) */
    SPDR = data_bytes;
    while( !(SPSR & (1<<SPIF)) )
        ;
}

void SPI_MasterTransmit_32bit(uint32_t data_bytes) {
    /* Start transmission of first byte (most significant byte) */
    SPDR = data_bytes>>24;
    while( !(SPSR & (1<<SPIF)) )
        ;
    SPDR = data_bytes>>16;           /* Start transmission of second byte */
    while( !(SPSR & (1<<SPIF)) )
        ;
    SPDR = data_bytes>>8;            /* Start transmission of third byte */
    while( !(SPSR & (1<<SPIF)) )
        ;
    /* Start transmission of last byte (least significant byte) */
    SPDR = data_bytes;
    while( !(SPSR & (1<<SPIF)) )
        ;
    LT_ON;    	/* Latch on the transmission */
    LT_OFF;     /* Latch off the transmission */
}

void TIM0_Init() {
    TCCR0A = (1<<WGM01);              /* set CTC mode */
    TCCR0B = (1<<CS00) | (1<<CS02);   /* set fck/1024 */
    OCR0A  = 3;
    TIMSK0 = (1<<OCIE0A);
}

void deleteLevel() {
    uint8_t i=31;
    while( i>7 ) {
        if( frameBuffer.floor[i] == 0xffff ) {
            for( uint8_t j=i; j>8; j-- ) {
                frameBuffer.floor[j] = frameBuffer.floor[j - 1];
            }
            frameBuffer.floor[8] = 0xc003;
            updatePoints();
            continue;
        }
        i--;
    }
}

uint8_t is_spaceDown() {
    uint8_t status = TRUE;

    if( frameBuffer.blocks[31] != 0 ) status = FALSE;

    for( uint8_t i=30; i>=7; i-- ) {
        if( (frameBuffer.blocks[i] & frameBuffer.floor[i + 1]) != 0 ) status = FALSE;
    }
    return status;
}

void moveBlockDown() {
    if( is_spaceDown() == TRUE ) {
        for( uint8_t i=31; i>7; i-- ) {
            frameBuffer.blocks[i] = frameBuffer.blocks[i - 1];
        }
        coords.y++;
        updateFramebuffer();
    } 
    else {
        for( uint8_t i=31; i>7; i-- ) {
            frameBuffer.floor[i] += frameBuffer.blocks[i];
        }
        deleteLevel();
        displayNewBlock();
        updateFramebuffer();
    }
}

uint8_t is_spaceLeft() {
    uint8_t status = TRUE;
    for( int8_t i=-2; i<=2; ++i ) {
        if( frameBuffer.blocks[coords.y + i] & frameBuffer.floor[coords.y + i]>>1 ) {
            status = FALSE;
        }
    }
    return status;
}

void moveBlockLeft() {
    if ( is_spaceLeft() == TRUE ) {
        for( int8_t y=-1; y<3; y++ ) {
            frameBuffer.blocks[coords.y + y] = frameBuffer.blocks[coords.y + y]<<1;
        }
        coords.x++;
        updateFramebuffer();
    }
}

uint8_t is_spaceRight() {
    uint8_t status = TRUE;
    for( int8_t i=-2; i<=2; ++i ) {
        if( frameBuffer.blocks[coords.y + i] & frameBuffer.floor[coords.y + i]<<1 ) {
            status = FALSE;
        }
    }
    return status;
}

void moveBlockRight() {
    if( is_spaceRight() == TRUE ) {
        for( int8_t y=-1; y<3; y++ ) {
            frameBuffer.blocks[coords.y + y] = frameBuffer.blocks[coords.y + y]>>1;
        }
        coords.x--;
        updateFramebuffer();
    }
}

void updateFramebuffer() {
    for( uint8_t i=0; i<32; i++ ) {
        frameBuffer.main[i] = frameBuffer.blocks[i] | frameBuffer.floor[i]; 
    }
    frameBuffer.main[3] = frameBuffer.nextBlock[0];
    frameBuffer.main[4] = frameBuffer.nextBlock[1];
    for( uint8_t i=1; i<6; i++ ) {
        frameBuffer.main[i] |= frameBuffer.points100[i - 1] | frameBuffer.points010[i - 1] | frameBuffer.points001[i - 1]; 
    }
}

void framebufferInit() {
    for( uint8_t i=0; i<8; ++i ) {
        frameBuffer.blocks[i] = 0;
        frameBuffer.floor[i] = 0;
    }

    for( uint8_t i=7; i<32; ++i ) {
        frameBuffer.blocks[i] = 0;
        frameBuffer.floor[i] = 0xc003;
    }
    frameBuffer.floor[7] = 0xffff;

    /* display "000" points */
    frameBuffer.points100[0] = 0xe000;
    frameBuffer.points100[1] = 0xa000;
    frameBuffer.points100[2] = 0xa000;
    frameBuffer.points100[3] = 0xa000;
    frameBuffer.points100[4] = 0xe000;

    frameBuffer.points010[0] = 0x0e00;
    frameBuffer.points010[1] = 0x0a00;
    frameBuffer.points010[2] = 0x0a00;
    frameBuffer.points010[3] = 0x0a00;
    frameBuffer.points010[4] = 0x0e00;

    frameBuffer.points001[0] = 0x00e0;
    frameBuffer.points001[1] = 0x00a0;
    frameBuffer.points001[2] = 0x00a0;
    frameBuffer.points001[3] = 0x00a0;
    frameBuffer.points001[4] = 0x00e0;

    nextBlock = timer_ms%7;
}

void buttonsInit() {
    DDRC  = 0;
    PORTC = 255;
}

void displayPLAY() {
    for( uint8_t i=0; i<32; ++i ) {
        frameBuffer.main[i] = 0;
        frameBuffer.blocks[i] = 0;
        frameBuffer.floor[i] = 0;
    }
    for( uint8_t i=0; i<5; ++i ) {
        frameBuffer.points100[i] = 0;
        frameBuffer.points010[i] = 0;
        frameBuffer.points001[i] = 0;
    }

    frameBuffer.floor[14] = 0xe8ea;
    frameBuffer.floor[15] = 0xa8aa;
    frameBuffer.floor[16] = 0xe8e4;
    frameBuffer.floor[17] = 0x88a4;
    frameBuffer.floor[18] = 0x8ea4;
    updateFramebuffer();
}

void GameOver() {
    timer_ms = 0;
    while( timer_ms < 500 )
        ;
    for( uint8_t j=0; j<5; j++ ) {
        for( uint8_t i=0; i<32; ++i ) {
            frameBuffer.main[i] = 0xffff;
        }
        timer_ms = 0;
        while( timer_ms < 500 )
            ;
        for( uint8_t i=0; i<32; ++i ) {
            frameBuffer.main[i] = 0x0000;
        }
        timer_ms = 0;
        while (timer_ms < 500)
            ;
    }
    pointsCounter--;
    for( uint8_t i=0; i<32; ++i ) {
        frameBuffer.floor[i] = 0x0000;
        frameBuffer.blocks[i] = 0x0000;
    }
    frameBuffer.nextBlock[0] = 0x0000;
    frameBuffer.nextBlock[1] = 0x0000;
    updatePoints();

    /* display points until reset */
    while(1)
        ;
}

void rotateBlockRight() { 
    if( currentBlock != I_BLOCK ) { /* if currentblock != BLOCK_I */
        int8_t crossArr[5] = { };
        int8_t diamondArr[4] = { };
        int8_t c = 0;
        int8_t d = 0;
        for( int8_t i=-1; i<2; i++ ) {
            for( int8_t j=-1; j<2; j++ ) {
                if( frameBuffer.blocks[coords.y + i] & (1<<(coords.x + j)) ) {
                    if( (i + j) % 2 ) {
                        diamondArr[c] = 1;
                        c++;
                    } 
                    else {
                        crossArr[d] = 1;
                        d++;
                    }
                } else {
                    if( (i + j) % 2 ) {
                        diamondArr[c] = 0;
                        c++;
                    } 
                    else {
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

        /* validation */
        c = 1;
        int8_t g;
        for( int8_t i=-1; i<2; i++ ) {
            g = frameBuffer.floor[coords.y + i];
            for( int16_t j=-1; j<2; j++ ) {
                if( newTable[i*3 + j + 4] ) {
                    if( g & (1<<(coords.x + j)) ) c = 0;  // validation == failed
                }
            }
        }

        /* if validation successful */
        if( c != 0 ) {
            for( int8_t i=-1; i<2; i++ ) {
                g = 0;
                for( int8_t j=-1; j<2; j++ ) {
                    if( newTable[i*3 + j + 4] ) g = g ^ (1<<(coords.x + j) );
                }
                frameBuffer.blocks[coords.y + i] = g;
            }
            updateFramebuffer();
        }
    } 
    else { /* case for BLOCK_I */
        /* check position */
        if( frameBuffer.blocks[coords.y - 1] == (1<<coords.x) ) { /* vertical */
            /* check if validation vertical -> horizontal == successful */
            if( !((15<<(coords.x - 2)) & frameBuffer.floor[coords.y]) ) {
                frameBuffer.blocks[coords.y - 1] = 0;
                frameBuffer.blocks[coords.y] = 15 << (coords.x - 2);
                frameBuffer.blocks[coords.y + 1] = 0;
                frameBuffer.blocks[coords.y + 2] = 0;
                updateFramebuffer();
            }
        } 
        else {  /* horizontal */
            int8_t c = 1;
            /* check if validation horizontal -> vertical == successful */
            for( int8_t i=-1; i<3; i++ ) {
                if( frameBuffer.floor[coords.y + i] & (1<<coords.x) ) c = 0;
            }
            if( c != 0 ) {
                for (int16_t i=-1; i<3; i++) {
                    frameBuffer.blocks[coords.y + i] = 1<<coords.x;
                }
                updateFramebuffer();
            }
        }
    }
}

void displayNewBlock() {
    /*delete previous block from block frame buffer*/
    for( int8_t i=-2; i<=2; ++i ) {
        frameBuffer.blocks[coords.y + i] = 0x0000;
    }

    /* assign coordinates of new core field of new block */
    coords.x = 8;
    coords.y = 8;
    currentBlock = nextBlock;
    nextBlock = timer_ms%7;

    switch (currentBlock) {
        case I_BLOCK:
            frameBuffer.blocks[8] = 0x03c0;
            frameBuffer.blocks[9] = 0x0000;
            break;
        case J_BLOCK:
            frameBuffer.blocks[8] = 0x0380;
            frameBuffer.blocks[9] = 0x0080;
            break;
        case L_BLOCK:
            frameBuffer.blocks[8] = 0x0380;
            frameBuffer.blocks[9] = 0x0200;
            break;
        case O_BLOCK:
            frameBuffer.blocks[8] = 0x0180;
            frameBuffer.blocks[9] = 0x0180;
            break;
        case S_BLOCK:
            frameBuffer.blocks[8] = 0x0180;
            frameBuffer.blocks[9] = 0x0300;
            break;
        case T_BLOCK:
            frameBuffer.blocks[8] = 0x0380;
            frameBuffer.blocks[9] = 0x0100;
            break;
        default:
            frameBuffer.blocks[8] = 0x0300;
            frameBuffer.blocks[9] = 0x0180;
            break;
    }

    displayNextBlock();
    updateFramebuffer();

    /* check if a new block has space to be spawned */
    if( frameBuffer.blocks[9] & frameBuffer.floor[9] ) GameOver();
    if( frameBuffer.blocks[8] & frameBuffer.floor[8] ) GameOver();
}

void displayNextBlock() {
    switch (nextBlock) {
        case I_BLOCK:
            frameBuffer.nextBlock[0] = 0x000f;
            frameBuffer.nextBlock[1] = 0x0000;
            break;
        case J_BLOCK:
            frameBuffer.nextBlock[0] = 0x000e;
            frameBuffer.nextBlock[1] = 0x0002;
            break;
        case L_BLOCK:
            frameBuffer.nextBlock[0] = 0x000e;
            frameBuffer.nextBlock[1] = 0x0008;
            break;
        case O_BLOCK:
            frameBuffer.nextBlock[0] = 0x0006;
            frameBuffer.nextBlock[1] = 0x0006;
            break;
        case S_BLOCK:
            frameBuffer.nextBlock[0] = 0x0006;
            frameBuffer.nextBlock[1] = 0x000c;
            break;
        case T_BLOCK:
            frameBuffer.nextBlock[0] = 0x000e;
            frameBuffer.nextBlock[1] = 0x0004;
            break;
        default:
            frameBuffer.nextBlock[0] = 0x000c;
            frameBuffer.nextBlock[1] = 0x0006;
            break;
    }
}

void updatePoints() {
    pointsCounter++;

    /* use lvl variable to scale the speed of blocks */
    if( pointsCounter < 100 ) lvl = pointsCounter * 3;

    /* calculate very digit of points value */
    uint8_t hundreds = pointsCounter/100;
    uint8_t tens = pointsCounter % 100;
    tens = tens/10;
    uint8_t unity = pointsCounter % 10;

    /* display hundreds digit */
    switch (hundreds) {
        case 0:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0xa000;
            frameBuffer.points100[2] = 0xa000;
            frameBuffer.points100[3] = 0xa000;
            frameBuffer.points100[4] = 0xe000;
            break;
    	case 1:
            frameBuffer.points100[0] = 0x4000;
            frameBuffer.points100[1] = 0xc000;
            frameBuffer.points100[2] = 0x4000;
            frameBuffer.points100[3] = 0x4000;
            frameBuffer.points100[4] = 0xe000;
           break;
        case 2:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0x2000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0x8000;
            frameBuffer.points100[4] = 0xe000;
            break;
        case 3:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0x2000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0x2000;
            frameBuffer.points100[4] = 0xe000;
            break;
        case 4:
            frameBuffer.points100[0] = 0xa000;
            frameBuffer.points100[1] = 0xa000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0x2000;
            frameBuffer.points100[4] = 0x2000;
            break;
        case 5:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0x8000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0x2000;
            frameBuffer.points100[4] = 0xe000;
            break;
        case 6:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0x8000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0xa000;
            frameBuffer.points100[4] = 0xe000;
            break;
        case 7:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0xa000;
            frameBuffer.points100[2] = 0x2000;
            frameBuffer.points100[3] = 0x2000;
            frameBuffer.points100[4] = 0x2000;
            break;
        case 8:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0xa000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0xa000;
            frameBuffer.points100[4] = 0xe000;
            break;
        case 9:
            frameBuffer.points100[0] = 0xe000;
            frameBuffer.points100[1] = 0xa000;
            frameBuffer.points100[2] = 0xe000;
            frameBuffer.points100[3] = 0x2000;
            frameBuffer.points100[4] = 0xe000;
            break;
    }

    /* display tens digit */
    switch (tens) {
        case 0:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0a00;
            frameBuffer.points010[2] = 0x0a00;
            frameBuffer.points010[3] = 0x0a00;
            frameBuffer.points010[4] = 0x0e00;
            break;
        case 1:
            frameBuffer.points010[0] = 0x0400;
            frameBuffer.points010[1] = 0x0c00;
            frameBuffer.points010[2] = 0x0400;
            frameBuffer.points010[3] = 0x0400;
            frameBuffer.points010[4] = 0x0e00;
    	    break;
        case 2:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0200;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0800;
            frameBuffer.points010[4] = 0x0e00;
            break;
        case 3:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0200;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0200;
            frameBuffer.points010[4] = 0x0e00;
            break;
        case 4:
            frameBuffer.points010[0] = 0x0a00;
            frameBuffer.points010[1] = 0x0a00;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0200;
            frameBuffer.points010[4] = 0x0200;
            break;
        case 5:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0800;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0200;
            frameBuffer.points010[4] = 0x0e00;
            break;
        case 6:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0800;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0a00;
            frameBuffer.points010[4] = 0x0e00;
            break;
        case 7:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0a00;
            frameBuffer.points010[2] = 0x0200;
            frameBuffer.points010[3] = 0x0200;
            frameBuffer.points010[4] = 0x0200;
            break;
        case 8:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0a00;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0a00;
            frameBuffer.points010[4] = 0x0e00;
            break;
        case 9:
            frameBuffer.points010[0] = 0x0e00;
            frameBuffer.points010[1] = 0x0a00;
            frameBuffer.points010[2] = 0x0e00;
            frameBuffer.points010[3] = 0x0200;
            frameBuffer.points010[4] = 0x0e00;
            break;
    }

    /* display unity digit */
    switch (unity) {
        case 0:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x00a0;
            frameBuffer.points001[2] = 0x00a0;
            frameBuffer.points001[3] = 0x00a0;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 1:
            frameBuffer.points001[0] = 0x0040;
            frameBuffer.points001[1] = 0x00c0;
            frameBuffer.points001[2] = 0x0040;
            frameBuffer.points001[3] = 0x0040;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 2:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x0020;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x0080;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 3:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x0020;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x0020;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 4:
            frameBuffer.points001[0] = 0x00a0;
            frameBuffer.points001[1] = 0x00a0;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x0020;
            frameBuffer.points001[4] = 0x0020;
            break;
        case 5:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x0080;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x0020;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 6:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x0080;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x00a0;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 7:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x00a0;
            frameBuffer.points001[2] = 0x0020;
            frameBuffer.points001[3] = 0x0020;
            frameBuffer.points001[4] = 0x0020;
            break;
        case 8:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x00a0;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x00a0;
            frameBuffer.points001[4] = 0x00e0;
            break;
        case 9:
            frameBuffer.points001[0] = 0x00e0;
            frameBuffer.points001[1] = 0x00a0;
            frameBuffer.points001[2] = 0x00e0;
            frameBuffer.points001[3] = 0x0020;
            frameBuffer.points001[4] = 0x00e0;
            break;
    }
    updateFramebuffer();
}
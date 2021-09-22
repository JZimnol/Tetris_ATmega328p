/*
 * @file Tetris.h
 * @author: JZimnol
 * @brief File containing macros, definitions, enums and structs for Tetris game
 */ 


#ifndef TETRIS_H_
#define TETRIS_H_

/*************************************************************************\
                                   MACROS
\*************************************************************************/
/*
 * @brief Macros used to latch SPI shift registers
 */
#define LT_ON  (PORTB |= (1<<PB2))       // latch on
#define LT_OFF (PORTB &= ~(1<<PB2))      // latch off
/*
 * @brief Buttons macros
 */
#define PC0_PUSHED !(PINC & (1<<PC0))    // rotate
#define PC1_PUSHED !(PINC & (1<<PC1))    // right
#define PC2_PUSHED !(PINC & (1<<PC2))    // down
#define PC3_PUSHED !(PINC & (1<<PC3))    // left

/*************************************************************************\
                                DEFINITIONS
\*************************************************************************/

#ifndef TRUE
    #define TRUE    (1)
#endif

#ifndef FALSE
    #define FALSE   (0)
#endif

/*************************************************************************\
                              ENUMS AND STRUCTS
\*************************************************************************/

/*
 * @brief Frame buffers data
 */
typedef struct {
    uint16_t main[32];
    uint16_t blocks[32];
    uint16_t floor[32];
    uint16_t nextBlock[2];

    uint16_t points100[5];
    uint16_t points010[5];
    uint16_t points001[5];
} FrameBuffer;
/*
 * @brief Coordinates of block's core pixel
 */
typedef struct {
    uint8_t x;
    uint8_t y;
} Coordinates;
/*
 * @brief Types of blocks
 */
typedef enum {
    I_BLOCK = (uint8_t)0,
    J_BLOCK = (uint8_t)1,
    L_BLOCK = (uint8_t)2,
    O_BLOCK = (uint8_t)3,
    S_BLOCK = (uint8_t)4,
    T_BLOCK = (uint8_t)5,
    Z_BLOCK = (uint8_t)6
} BlockType;

/*************************************************************************\
                            VARIABLE DECLARATIONS
\*************************************************************************/

uint16_t pointsCounter;         /* counter of displayed points */
volatile uint16_t timer_ms;     /* timer variable */
volatile uint8_t iteratorSPI;   /* iterator used for SPI communication */
volatile uint16_t debounce;     /* used to get the button debounce effect */
uint16_t lvl;                   /* level counter; it's used to calculate the speed 
                                   of falling of the block */
FrameBuffer frameBuffer;        /* frame buffer struct */
Coordinates coords;             /* core pixel coordinates struct */

/*************************************************************************\
                                 FUNCTIONS
\*************************************************************************/

/*
 * @brief initialize simple SPI communication
 */
void SPI_MasterInit();
/*
 * @brief transmit 16-bit data using SPI (frame buffer)
 * @param two data bytes to send
 */
void SPI_MasterTransmit_16bit(uint16_t data_bytes);
/*
 * @brief transmit 32-bit data using SPI (column number)
 * @param four data bytes to send
 */
void SPI_MasterTransmit_32bit(uint32_t data_bytes);
/*
 * @brief initialize TiM0 timer
 */
void TIM0_Init();
/*
 * @brief delete row full of blocks
 */ 
void deleteLevel();
/*
 * @brief check if there is space below the block
 * @return true or false
 */
uint8_t is_spaceDown();
/*
 * @brief move block one pixel down
 */
void moveBlockDown();
/*
 * @brief check if there is space on the left of the block
 * @return true or false
 */
uint8_t is_spaceLeft();
/*
 * @brief move block one pixel to the left
 */
void moveBlockLeft();
/*
 * @brief check if there is space on the right of the block
 * @return true or false
 */
uint8_t is_spaceRight();
/*
 * @brief move block one pixel to the right
 */
void moveBlockRight();
/*
 * @brief sum all framebuffers into main one
 */
void updateFramebuffer();
/*
 * @brief initialize frame buffer
 */
void framebufferInit();
/*
 * @brief initialize buttons
 */
void buttonsInit();
/*
 * @brief display PLAY at the beginning
 */
void displayPLAY();
/*
 * @brief end the game, display ending sequence
 */
void GameOver();
/*
 * @brief rotate block clockwise
 */ 
void rotateBlockRight();
/*
 * @brief display next block
 */
void displayNextBlock();
/*
 * @brief choose and display new block
 */
void displayNewBlock();
/*
 * @brief update points status
 */
void updatePoints();

#endif /* TETRIS_H_ */
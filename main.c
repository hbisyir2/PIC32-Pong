////////////////////////////////////////////////////////////////////////////////////
// ECE 2534:        Lab 03, Hakeem Bisyir
//
// File name:       main.c
//
// Description:     This programs runs the famous historic game pong using the
//                  joystick and the board's analog to digital converter.  The
//                  wire convention is as follows:
//                  port 1 (brown): L/R output
//                  port 2 (green): U/D output
//                  port 3 (blue): unused
//                  port 4 (yellow): high output Vcc
//                  port 5 (black): ground
//                  port 6 (red): high output Vcc 
//
// Efficiency:		The program works as described.
//
// My Experience:	Designing the menus of the game was fairly simple similar
//                  to the previous lab.  The challenging part of the project
//                  however was the game itself and designing a system to allow
//                  the smooth transition and movement of both the ball and paddle.
//                  Allowing both the ball and paddle to move independent of each other
//                  was the most challenging part.  The pixel drawing aspect of
//                  the project was fairly straight forward however there were a
//                  few challenged such as keeping the center dotted line untouched
//                  and not clearing pixels from the score counter.  Overall this
//                  project was much more challenging than the previous one, however
//                  with the tools we were given, it could be implemented with some
//                  thought and understanding of the material.
//
// Additional
// Implementations: Added a difficulty modifier that ranges from 0-2.
//
// Date:   10/27/2016

#include <stdio.h>                      // for sprintf()
#include <plib.h>                       // Peripheral Library
#include <stdbool.h>                    // for data type bool
#include "PmodOLED.h"
#include "OledChar.h"
#include "OledGrph.h"
#include "delay.h"

// Diligent board configuration
#pragma config ICESEL       = ICS_PGx1  // ICE/ICD Comm Channel Select
#pragma config DEBUG        = OFF       // Debugger Disabled for Starter Kit
#pragma config FNOSC        = PRIPLL	// Oscillator selection
#pragma config POSCMOD      = XT	    // Primary oscillator mode
#pragma config FPLLIDIV     = DIV_2	    // PLL input divider
#pragma config FPLLMUL      = MUL_20	// PLL multiplier
#pragma config FPLLODIV     = DIV_1	    // PLL output divider
#pragma config FPBDIV       = DIV_8	    // Peripheral bus clock divider
#pragma config FSOSCEN      = OFF	    // Secondary oscillator enable

// Global variable for timer interrupt
volatile unsigned int timer2_mseconds = 500;

// Global variables for ADC interrupt
volatile int ADC_Port1, ADC_Port2;

// Interrupt for timer 2
void __ISR(_TIMER_2_VECTOR, IPL4AUTO) _Timer2Handler(void) {
    timer2_mseconds--; // Increment the millisecond counter.
    INTClearFlag(INT_T2); // Clear the Timer2 interrupt flag.
}

// ADC interrupt
void __ISR(_ADC_VECTOR, IPL7SRS) _ADCHandler(void) {
    
    if (ReadActiveBufferADC10()) {
        ADC_Port1 = ReadADC10(0);
        ADC_Port2 = ReadADC10(1);
    }
    
    else if (!ReadActiveBufferADC10()) {
        ADC_Port1 = ReadADC10(8);
        ADC_Port2 = ReadADC10(9);
    }

    INTClearFlag(INT_AD1);
}

// ADC MUX Configuration
// Using both muxA and muxB, muxA takes AN2 as positive input, VREFL as negative input
// muxB takes AN3 as positive input, VREFL as negative input
#define AD_MUX_CONFIG ADC_CH0_POS_SAMPLEA_AN2 | ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEB_AN3 | ADC_CH0_NEG_SAMPLEB_NVREF

// ADC Config1 settings
// Data stored as 16 bit unsigned int
// Internal clock used to start conversion
// ADC auto sampling (sampling begins immediately following conversion)
#define AD_CONFIG1 ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON

// ADC Config2 settings
// Using internal (VDD and VSS) as reference voltages
// Do not scan inputs
// Two samples per interrupt
// Buffer mode is two 8-word buffer
// Alternate sample mode on
#define AD_CONFIG2 ADC_VREF_AVDD_AVSS | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_2 | ADC_ALT_BUF_ON | ADC_ALT_INPUT_ON

// ADC Config3 settings
// Autosample time in TAD = 8
// Prescaler for TAD:  the 20 here corresponds to a
// ADCS value of 0x27 or 39 decimal => (39 + 1) * 2 * TPB = 8.0us = TAD
// NB: Time for an AD conversion is thus, 8 TAD for aquisition +
//     12 TAD for conversion = (8+12)*TAD = 20*8.0us = 160us.
#define AD_CONFIG3 ADC_SAMPLE_TIME_8 | ADC_CONV_CLK_20Tcy

// ADC Port Configuration (PCFG)
// Not scanning, so nothing need be set here..
// Enable both AN2 and AN3
// sets the AD1CHS register (true, but not that obvious...)
#define AD_CONFIGPORT ENABLE_AN2_ANA | ENABLE_AN3_ANA

// ADC Input scan select (CSSL) -- skip scanning as not in scan mode
#define AD_CONFIGSCAN SKIP_SCAN_ALL

//Function Declarations
void Initialize();          //Initialize clock and OLED
bool joystickDown();        //Detects middle to down transition for joystick
bool joystickRight();       //Detects middle to right transition for joystick
bool joystickUp();          //Detects middle to up transition for joystick
bool joystickLeft();        //Detects middle to left transition for joystick
void Timer3InitMessage();   //Initial message timer
void Timer2InitStartGame(); //Timer for game countdown
void Timer3InitMenu();      //Debouncing timer
void Timer3InitPaddle();    //Debounding timer for joystick movement in game
void initADC();             //Initialize ADC
void startGame();           //Starts game
void drawGame(int paddleY, int height, int leftSide);   //Draws pong board
void drawCenterLine();                                  //Draws dotted center line
void drawBorder(int leftBound);                         //Draws solid line borders
void drawPaddle(int paddleY, int height);               //Draws paddle on board
void drawBall(int ballXPos, int ballYPos);              //Draws ball on board
void clearBall(int clearXPos, int clearYPos, int paddle, int height);   //Erases ball from board
int randomNumX();           //Creates random num for X velocity
int randomNumY();           //Creates random num for Y velocity
void paddleBounce(int* Xvel, int* Yvel, int oldPaddle, int newPaddle);  //Changes ball speed based on paddle movement


// Variable Declarations
int i = 0;                      //for various loops
int inputWait;                  //wait for a change from joystick in menu
int maxScore = 10;              //target score of game
int newMaxScore = 10;           //New target score to hold for confirmation
char maxScoreStr[3];            //string for target score
bool messageTime = true;        //bool for timed messages
const int paddleHeight = 5;     //height of paddle
int paddlePos = 13;             //bottom position of paddle
int oldPaddlePos = 13;          //old position of paddle for velocity
int score = 0;                  //actual score of game
char scoreStr[3];               //string of score
int endBoard = 1;               //left bound of the board
int difficulty = 0;             //Difficulty, ranges from 0-2
int newDifficulty = 0;          //New Difficulty to hold for confirmation
char difficultyStr[2];          //string of difficulty
int ballX = 64;                 //bottom left corner of ball X coordinate
int ballY = 16;                 //bottom left corner of ball Y coordinate
int ballSpeedX = 1;             //Speed of ball in X direction
int ballSpeedY = 1;             //Speed of ball in Y direction
bool bouncePaddle = false;      //Bool if ball bounces of the paddle


void Timer3InitMessage() 
{
    // The period of Timer 2 is (256 * 39062)/(10 MHz) = .1 s (freq = 10 Hz)
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_256 | T3_GATE_OFF, 39061);
    return;
}

void Timer2InitStartGame()
{
    // The period of Timer 2 is (256 * 39062)/(10 MHz) = .01 s (freq = 100 Hz)
    OpenTimer2(T2_ON | T2_IDLE_CON | T2_SOURCE_INT | T2_PS_1_16 | T2_GATE_OFF, 6249);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTClearFlag(INT_T2);
    INTEnable(INT_T2, INT_ENABLED);
    return;
}

void Timer3InitMenu() 
{
    // The period of Timer 2 is (256 * 15625)/(10 MHz) = .22 s (freq = 4.5 Hz)
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_256 | T3_GATE_OFF, 8624);
    return;
}

void Timer3InitPaddle() 
{
    // The period of Timer 2 is (256 * 15625)/(10 MHz) = .067 s (freq = 14.8 Hz)
    OpenTimer3(T3_ON | T3_IDLE_CON | T3_SOURCE_INT | T3_PS_1_256 | T3_GATE_OFF, 2624);
    return;
}

void Timer4Ball()
{
    // The period of Timer 2 is (16 * 625)/(10 MHz) = .2 ms (freq = 5 kHz)
    OpenTimer4(T4_ON | T4_IDLE_CON | T4_SOURCE_INT | T4_PS_1_16 | T4_GATE_OFF, 124);
    return;
}

// Initialize the ADC using my definitions
// Set up ADC interrupts
void initADC() {

    // Configure and enable the ADC HW
    SetChanADC10(AD_MUX_CONFIG);
    OpenADC10(AD_CONFIG1, AD_CONFIG2, AD_CONFIG3, AD_CONFIGPORT, AD_CONFIGSCAN);
    EnableADC10();

    // Set up, clear, and enable ADC interrupts
    INTSetVectorPriority(INT_ADC_VECTOR, INT_PRIORITY_LEVEL_7);
    INTClearFlag(INT_ADC_VECTOR);
    INTClearFlag(INT_AD1);
    INTEnable(INT_AD1, INT_ENABLED);
}


int main() {
    // Initialize timers and ADC
    Initialize();
    
    //Clear LEDs
    LATGCLR = (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12);
    
    //Configure system for interrupts
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();
    
    // Set brown and green ports for inputs
    // Set yellow port for output
    TRISBSET = 0x0C;
    TRISBCLR = 0x40;
    LATBCLR = 0x40;
    PORTB = PORTB | 0x40;
        
    enum States {initMessage, mainPlay, playGame, win, mainOptions, optionsScore, optionsChange10, optionsChange15, 
    optionsChange20, optionsDifficulty, difficulty0, difficulty1, difficulty2, confirmYes, confirmNo}; 
    // Possible states of state machine
    enum States systemState = initMessage; 	 // Initialize system state
    
    // Reset timers
	TMR2 = 0x0;
    TMR3 = 0x0;
    
    // main while loop
	while (1)
	{
        if (INTGetFlag(INT_T3)) {
            INTClearFlag(INT_T3);
            switch (systemState)
            {
            case initMessage:
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("ECE 2534, Lab 3");
                OledSetCursor(0, 1);
                OledPutString("by Hakeem Bisyir");
                OledSetCursor(0, 2);
                OledPutString("Pong!");
                OledUpdate();
                messageTime = true;
                i = 0;
                TMR3 = 0x0;
                Timer3InitMessage();
                while (messageTime) {
                    if (INTGetFlag(INT_T3)) {
                        INTClearFlag(INT_T3);
                        i++;
                    };
                    if (i >= 6) {
                        messageTime = false;
                    };
                };
                INTClearFlag(INT_T3);
                OledClearBuffer();
                OledUpdate();
                Timer3InitMenu();
                systemState = mainPlay;
                break;
            case mainPlay:
                inputWait = 1;
                OledClearBuffer();
                OledSetCursor(0, 0);
                OledPutString("PONG Main Menu");
                OledSetCursor(0, 1);
                OledPutString("-> Play Game");
                OledSetCursor(0, 2);
                OledPutString("   Options");
                OledUpdate();
                while (inputWait) {
                    if (joystickDown())
                    {
                        systemState = mainOptions;
                        inputWait = 0;
                    }
                    else if (joystickUp())
                    {
                        systemState = mainOptions;
                        inputWait = 0;
                    }
                    else if (joystickRight())
                    {
                        systemState = playGame;
                        inputWait = 0;
                    }
                }
                break;
                case playGame:
                    inputWait = 1;
                    OledClearBuffer();
                    startGame();
                    Timer4Ball();
                    ballSpeedX = randomNumX();
                    ballSpeedY = randomNumY();
                    ballX = 64;
                    ballY = 16;
                    score = 0;
                    paddlePos = 13;
                    bouncePaddle = false;
                    oldPaddlePos = paddlePos;
                    INTClearFlag(INT_T4);
                    Timer3InitPaddle();
                    OledSetCursor(9, 1);
                    sprintf(scoreStr, "%2d", score);
                    OledPutString(scoreStr);
                    while (inputWait) {
                        if (INTGetFlag(INT_T4))
                        {
                            INTClearFlag(INT_T4);
                            clearBall(ballX, ballY, paddlePos, paddleHeight);
                            if ((ballX + ballSpeedX) <= endBoard)
                            {
                                ballSpeedX = ballSpeedX * -1;
                            }
                            if (((ballY + ballSpeedY) >= 30) || ((ballY + ballSpeedY) <= 2))
                            {
                                ballSpeedY = ballSpeedY * -1;
                            }
                            if ((ballY+ballSpeedY <= paddlePos) && (ballY+ballSpeedY > (paddlePos - paddleHeight)) && (ballX+ballSpeedX >= 125)) {
                                ballSpeedX = ballSpeedX * -1;
                                score++;
                                sprintf(scoreStr, "%2d", score);
                                if (difficulty == 1) {
                                    if (endBoard < 50) {
                                        endBoard = endBoard + 5;
                                    }
                                    drawGame(paddlePos, paddleHeight, endBoard);
                                    clearBall(64, 16, paddlePos, paddleHeight);
                                }
                                else if (difficulty == 2) {
                                    if (endBoard < 60) {
                                        endBoard = endBoard + 5;
                                    }
                                    drawGame(paddlePos, paddleHeight, endBoard);
                                    clearBall(64, 16, paddlePos, paddleHeight);
                                }
                                bouncePaddle = true;
                            }
                            else if ((ballX + ballSpeedX) >= 127)
                            {
                                if (endBoard < 50) {
                                    ballX = 64;
                                    ballY = 16;
                                }
                                else {
                                    ballX = 96;
                                    ballY = 16;
                                }
                                ballSpeedX = randomNumX();
                                ballSpeedY = randomNumY();
                            }
                            ballX = ballX + ballSpeedX;
                            ballY = ballY + ballSpeedY;
                            drawBall(ballX, ballY);
                        }
                        
                        if (INTGetFlag(INT_T3)) {
                            INTClearFlag(INT_T3);
                            if (ADC_Port2 < 400)
                            {
                                paddlePos = paddlePos + 4;
                                if (paddlePos >= 31) {
                                    paddlePos = 30;
                                }
                                drawPaddle(paddlePos, paddleHeight);
                            }
                            else if (ADC_Port2 > 700)
                            {
                                paddlePos = paddlePos - 4;
                                if (paddlePos <= 5) {
                                    paddlePos = 6;
                                }
                                drawPaddle(paddlePos, paddleHeight);
                            }
                        }
                        if (score == maxScore) {
                            systemState = win;
                            inputWait = 0;
                        }
                        OledSetCursor(10, 1);
                        OledPutString(scoreStr);
                    if (bouncePaddle) {
                        paddleBounce(&ballSpeedX, &ballSpeedY, oldPaddlePos, paddlePos);
                        bouncePaddle = false;
                        }
                        if (ballSpeedX == 0) {
                            ballSpeedX++;
                        }
                    oldPaddlePos = paddlePos;
                    }
                    break;
                case win:
                    OledClearBuffer();
                    OledSetCursor(0,0);
                    OledPutString("Congrats!");
                    OledSetCursor(0,1);
                    OledPutString("You Win!");
                    TMR2 = 0x0;
                    i = 0;
                    messageTime = true;
                    Timer3InitMessage();
                    while (messageTime) {
                        if (INTGetFlag(INT_T3)) {
                            INTClearFlag(INT_T3);
                            i++;
                        };
                        if (i >= 6) {
                            messageTime = false;
                        };
                    };
                    INTClearFlag(INT_T3);
                    OledClearBuffer();
                    OledUpdate();
                    systemState = initMessage;
                    break;
                case mainOptions:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("PONG Main Menu");
                    OledSetCursor(0, 1);
                    OledPutString("   Play Game");
                    OledSetCursor(0, 2);
                    OledPutString("-> Options");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = mainPlay;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = mainPlay;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = optionsScore;
                            inputWait = 0;
                        }
                    }
                    break;
                case optionsScore:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Options Menu");
                    OledSetCursor(0, 1);
                    OledPutString("-> Score: ");
                    sprintf(maxScoreStr, "%2d", maxScore);
                    OledSetCursor(10, 1);
                    OledPutString(maxScoreStr);
                    OledSetCursor(0, 2);
                    OledPutString("   Difficulty:");
                    OledSetCursor(16, 2);
                    sprintf(difficultyStr, "%1d", difficulty);
                    OledPutString(difficultyStr);
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = optionsDifficulty;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = optionsDifficulty;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = optionsChange10;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = mainOptions;
                            inputWait = 0;
                        }
                    }
                    break;
                case optionsChange10:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Options Menu");
                    OledSetCursor(0, 2);
                    OledPutString("Score: ");
                    OledSetCursor(8, 1);
                    OledPutString("-> 10");
                    OledSetCursor(8, 2);
                    OledPutString("   15");
                    OledSetCursor(8, 3);
                    OledPutString("   20");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = optionsChange15;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = optionsChange20;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = confirmYes;
                            newMaxScore = 10;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsScore;
                            inputWait = 0;
                        }
                    }
                    break;
                case optionsChange15:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Options Menu");
                    OledSetCursor(0, 2);
                    OledPutString("Score: ");
                    OledSetCursor(8, 1);
                    OledPutString("   10");
                    OledSetCursor(8, 2);
                    OledPutString("-> 15");
                    OledSetCursor(8, 3);
                    OledPutString("   20");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = optionsChange20;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = optionsChange10;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = confirmYes;
                            newMaxScore = 15;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsScore;
                            inputWait = 0;
                        }
                    }
                    break;
                case optionsChange20:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Options Menu");
                    OledSetCursor(0, 2);
                    OledPutString("Score: ");
                    OledSetCursor(8, 1);
                    OledPutString("   10");
                    OledSetCursor(8, 2);
                    OledPutString("   15");
                    OledSetCursor(8, 3);
                    OledPutString("-> 20");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = optionsChange10;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = optionsChange15;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = confirmYes;
                            newMaxScore = 20;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsScore;
                            inputWait = 0;
                        }
                    }
                    break;
                case optionsDifficulty:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Options Menu");
                    OledSetCursor(0, 1);
                    OledPutString("   Score: ");
                    sprintf(maxScoreStr, "%2d", maxScore);
                    OledSetCursor(10, 1);
                    OledPutString(maxScoreStr);
                    OledSetCursor(0, 2);
                    OledPutString("-> Difficulty:");
                    OledSetCursor(16, 2);
                    sprintf(difficultyStr, "%1d", difficulty);
                    OledPutString(difficultyStr);
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = optionsScore;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = optionsScore;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = difficulty0;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = mainOptions;
                            inputWait = 0;
                        }
                    }
                    break;
                case difficulty0:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Difficulty Menu");
                    OledSetCursor(0, 2);
                    OledPutString("Difficulty:");
                    OledSetCursor(12, 1);
                    OledPutString("-> 0");
                    OledSetCursor(12, 2);
                    OledPutString("   1");
                    OledSetCursor(12, 3);
                    OledPutString("   2");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = difficulty1;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = difficulty2;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = confirmYes;
                            newDifficulty = 0;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsDifficulty;
                            inputWait = 0;
                        }
                    }
                    break;
                case difficulty1:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Difficulty Menu");
                    OledSetCursor(0, 2);
                    OledPutString("Difficulty:");
                    OledSetCursor(12, 1);
                    OledPutString("   0");
                    OledSetCursor(12, 2);
                    OledPutString("-> 1");
                    OledSetCursor(12, 3);
                    OledPutString("   2");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = difficulty2;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = difficulty0;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = confirmYes;
                            newDifficulty = 1;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsDifficulty;
                            inputWait = 0;
                        }
                    }
                    break;
                    case difficulty2:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Difficulty Menu");
                    OledSetCursor(0, 2);
                    OledPutString("Difficulty:");
                    OledSetCursor(12, 1);
                    OledPutString("   0");
                    OledSetCursor(12, 2);
                    OledPutString("   1");
                    OledSetCursor(12, 3);
                    OledPutString("-> 2");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = difficulty0;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = difficulty1;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = confirmYes;
                            newDifficulty = 2;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsDifficulty;
                            inputWait = 0;
                        }
                    }
                    break;
                case confirmYes:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Are you sure?");
                    OledSetCursor(0, 2);
                    OledPutString("-> Yes");
                    OledSetCursor(0, 3);
                    OledPutString("   No");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = confirmNo;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = confirmNo;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = optionsScore;
                            difficulty = newDifficulty;
                            maxScore = newMaxScore;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsScore;
                            newDifficulty = difficulty;
                            newMaxScore = maxScore;
                            inputWait = 0;
                        }
                    }
                    break;
                case confirmNo:
                    inputWait = 1;
                    OledClearBuffer();
                    OledSetCursor(0, 0);
                    OledPutString("Are you sure?");
                    OledSetCursor(0, 2);
                    OledPutString("   Yes");
                    OledSetCursor(0, 3);
                    OledPutString("-> No");
                    OledUpdate();
                    while (inputWait) {
                        if (joystickDown())
                        {
                            systemState = confirmYes;
                            inputWait = 0;
                        }
                        else if (joystickUp())
                        {
                            systemState = confirmYes;
                            inputWait = 0;
                        }
                        else if (joystickRight())
                        {
                            systemState = optionsScore;
                            newDifficulty = difficulty;
                            newMaxScore = maxScore;
                            inputWait = 0;
                        }
                        else if (joystickLeft())
                        {
                            systemState = optionsScore;
                            newDifficulty = difficulty;
                            newMaxScore = maxScore;
                            inputWait = 0;
                        }
                    }
                    break;
            } // end switch-case
        } // end debouncing if
    } // end while(1)
} // end main







/////////////////////////////////////////////////////////////////
// Function:    joystickUp
// Description: Perform a nonblocking check to see if joystick has been moved
// Inputs:      None
// Returns:     TRUE if middle to up transition is detected;
//                otherwise return FALSE

bool joystickUp()
{
    enum joystick {middle, up}; // Possible states of BTN1
    
    static enum joystick joystickCurrentPosition = middle;  // BTN1 current state
    static enum joystick joystickPreviousPosition = middle; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    joystickPreviousPosition = joystickCurrentPosition;

    // Read BTN1
    if(ADC_Port2 > 700)                                
    {
        joystickCurrentPosition = up;
    } 
	else
    {
        joystickCurrentPosition = middle;
    } 
    
    if((joystickCurrentPosition == up) && (joystickPreviousPosition == middle))
    {
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}


/////////////////////////////////////////////////////////////////
// Function:    joystickDown
// Description: Perform a nonblocking check to see if joystick has been moved
// Inputs:      None
// Returns:     TRUE if middle to down transition is detected;
//                otherwise return FALSE

bool joystickDown()
{
    enum joystick {middle, down}; // Possible states of BTN1
    
    static enum joystick joystickCurrentPosition = middle;  // BTN1 current state
    static enum joystick joystickPreviousPosition = middle; // BTN1 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    joystickPreviousPosition = joystickCurrentPosition;

    // Read BTN1
    if(ADC_Port2 < 400)                                
    {
        joystickCurrentPosition = down;
    } 
	else
    {
        joystickCurrentPosition = middle;
    } 
    
    if((joystickCurrentPosition == down) && (joystickPreviousPosition == middle))
    {
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}



/////////////////////////////////////////////////////////////////
// Function:    joystickRight
// Description: Perform a nonblocking check to see if joystick has been moved
// Inputs:      None
// Returns:     TRUE if middle to right transition is detected;
//                otherwise return FALSE

bool joystickRight()
{
    enum joystick {middle, right}; // Possible states of BTN2
    
    static enum joystick joystickCurrentPosition = middle;  // BTN2 current state
    static enum joystick joystickPreviousPosition = middle; // BTN2 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    joystickPreviousPosition = joystickCurrentPosition;

    // Read BTN2
    if(ADC_Port1 > 700)                                
    {
        joystickCurrentPosition = right;
    } 
	else
    {
        joystickCurrentPosition = middle;
    } 
    
    if((joystickCurrentPosition == right) && (joystickPreviousPosition == middle))
    {
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}


/////////////////////////////////////////////////////////////////
// Function:    joystickLeft
// Description: Perform a nonblocking check to see if joystick has been moved
// Inputs:      None
// Returns:     TRUE if middle to left transition is detected;
//                otherwise return FALSE

bool joystickLeft()
{
    enum joystick {middle, left}; // Possible states of BTN2
    
    static enum joystick joystickCurrentPosition = middle;  // BTN2 current state
    static enum joystick joystickPreviousPosition = middle; // BTN2 previous state
    // Reminder - "static" variables retain their values from one call to the next.
    
    joystickPreviousPosition = joystickCurrentPosition;

    // Read BTN2
    if(ADC_Port1 < 400)                                
    {
        joystickCurrentPosition = left;
    }
	else
    {
        joystickCurrentPosition = middle;
    } 
    
    if((joystickCurrentPosition == left) && (joystickPreviousPosition == middle))
    {
        return TRUE; // 0-to-1 transition has been detected
    }
    return FALSE;    // 0-to-1 transition not detected
}



/////////////////////////////////////////////////////////////////
// Function:     Initialize
// Description:  Initialize the system
// Inputs:       None
// Return value: None

void Initialize()
{
   // Initialize GPIO for all LEDs
   TRISGSET = 0xc0;     // For BTN1 & BTN2: configure PortG bit for input
   TRISGCLR = 0xf000;   // For LEDs 1-4: configure PortG pins for output
   ODCGCLR  = 0xf000;   // For LEDs 1-4: configure as normal output (not open drain)

   // Initialize Timer1, Timer2, and OLED
   DelayInit();
   Timer3InitMenu();
   OledInit();
   initADC();
   
   return;
}

/////////////////////////////////////////////////////////////////
// Function:     startGame
// Description:  draws the pong board and runs the countdown
// Inputs:       None
// Return value: None

void startGame()
{
    char countdownStr[6];
    Timer2InitStartGame();
    drawGame(13, 5, 1);
    timer2_mseconds = 500;
    Timer3InitPaddle();
    while (timer2_mseconds > 10) {
        if (INTGetFlag(INT_T3)) {
            INTClearFlag(INT_T3);
            if (ADC_Port2 < 400)
            {
                paddlePos = paddlePos + 4;
                if (paddlePos >= 31) {
                    paddlePos = 30;
                }
                drawPaddle(paddlePos, paddleHeight);
            }
            else if (ADC_Port2 > 700)
            {
                paddlePos = paddlePos - 4;
                if (paddlePos <= 5) {
                    paddlePos = 6;
                }
                drawPaddle(paddlePos, paddleHeight);
            }
        }
        sprintf(countdownStr, "%2d.%2d", timer2_mseconds/100, timer2_mseconds%100);
        OledSetCursor(9, 1);
        OledPutString(countdownStr);
        OledUpdate();
    }
    OledSetCursor(9, 1);
    OledPutString("     ");
    timer2_mseconds = 500;
    return;
}

/////////////////////////////////////////////////////////////////
// Function:     drawGame
// Description:  draws the pong board
// Inputs:       paddleY - bottom (largest) pixel value of the paddle
//               height - height of the paddle
//               leftSide - X column of left side bound of the board
// Return value: None

void drawGame(int paddleY, int height, int leftSide)
{
    OledClearBuffer();
    drawCenterLine();
    drawBorder(leftSide);
    drawPaddle(paddleY, height);
    drawBall(64, 16);
    return;
}

/////////////////////////////////////////////////////////////////
// Function:     drawCenterLine
// Description:  draws the dotted line in the middle of the board
// Inputs:       None
// Return value: None

void drawCenterLine()
{
    int yCenter = 0;
    for (yCenter = 0; yCenter < 32; yCenter++)
        if (yCenter%3 != 0) {
            OledMoveTo(64, yCenter);
            OledDrawPixel();
        }
    OledUpdate();
    return;
}

/////////////////////////////////////////////////////////////////
// Function:     drawBorder
// Description:  draws the top, bottom, and left side border of the board
// Inputs:       leftBound - X column of left side border of board
// Return value: None

void drawBorder(int leftBound)
{
    int topX;
    // Draw top bar
    for (topX = leftBound; topX < 128; topX++) {
        OledMoveTo(topX, 0);
        OledDrawPixel();
        OledMoveTo(topX, 1);
        OledDrawPixel();
    }
    int leftY;
    // Draw left bar
    for (leftY = 0; leftY < 32; leftY++) {
        OledMoveTo(leftBound, leftY);
        OledDrawPixel();
        OledMoveTo(leftBound-1, leftY);
        OledDrawPixel();
    }
    int bottomX;
    // Draw bottom bar
    for (bottomX = leftBound; bottomX < 128; bottomX++) {
        OledMoveTo(bottomX, 31);
        OledDrawPixel();
        OledMoveTo(bottomX, 30);
        OledDrawPixel();
    }
    OledUpdate();
    return;
}

/////////////////////////////////////////////////////////////////
// Function:     drawPaddle
// Description:  draws the paddle on the board
// Inputs:       paddleY - bottom (largest) pixel value of the paddle
//               height - height of the paddle
// Return value: None

void drawPaddle(int paddleY, int height)
{
    int drawHeight;
    int i;
    // Clear right column of the board
    OledSetDrawColor(0);
    for (i = 2; i <= 29; i++) {
        OledMoveTo(127, i);
        OledDrawPixel();
        OledMoveTo(126, i);
        OledDrawPixel();
    }
    
    // Draws paddle after clearing right column
    OledSetDrawColor(1);
    if ((paddleY-height) < 2) {
        paddleY = 6;
    }
    else if (paddleY > 29) {
        paddleY = 30;
    }
    for (drawHeight = 0; drawHeight <= height; drawHeight++) {
        OledMoveTo(127, paddleY);
        OledDrawPixel();
        OledMoveTo(126, paddleY);
        OledDrawPixel();
        paddleY--;
    }
    OledUpdate();
    return;
}

/////////////////////////////////////////////////////////////////
// Function:     drawBall
// Description:  draws the pong ball on the board
// Inputs:       ballXPos - X position of ball's bottom left corner
//               ballYPos - Y position of ball's bottom left corner
// Return value: None

void drawBall(int ballXPos, int ballYPos)
{
    if (ballXPos != 127 && ballXPos != 126) {
        OledMoveTo(ballXPos, ballYPos);
        OledDrawPixel();
        OledMoveTo(ballXPos, ballYPos-1);
        OledDrawPixel();
    }
    if (ballXPos+1 != 127 && ballXPos != 126) {
        OledMoveTo(ballXPos+1, ballYPos);
        OledDrawPixel();
        OledMoveTo(ballXPos+1, ballYPos-1);
        OledDrawPixel();
    }
    OledUpdate();
}

/////////////////////////////////////////////////////////////////
// Function:     clearBall
// Description:  erases ball from the board
// Inputs:       clearXPos - X position of ball's bottom left corner
//               clearYPos - Y position of ball's bottom left corner
//               paddle - current Paddle position
//               height - paddle height
// Return value: None

void clearBall(int clearXPos, int clearYPos, int paddle, int height)
{
    OledSetDrawColor(0);
    //Ensures it does not clear center line or paddle
    if (clearXPos != 64 && clearXPos != 126) {
        OledMoveTo(clearXPos, clearYPos);
        OledDrawPixel();
        OledMoveTo(clearXPos, clearYPos-1);
        OledDrawPixel();
    }
    else if (clearXPos == 64) {
        if (clearYPos%3 == 0) {
            OledMoveTo(clearXPos, clearYPos);
            OledDrawPixel();
        }
        else if ((clearYPos-1)%3 == 0) {
            OledMoveTo(clearXPos, clearYPos-1);
            OledDrawPixel();
        }
    }
    else if (clearXPos == 126) {
        if (clearYPos > paddle && clearYPos < paddle-height) {
            OledMoveTo(clearXPos, clearYPos);
            OledDrawPixel();
        }
        else if (clearYPos-1 > paddle && clearYPos-1 < paddle-height) {
            OledMoveTo(clearXPos, clearYPos-1);
            OledDrawPixel();
        }
    }
    if (clearXPos+1 != 64 && clearXPos+1 != 126) {
        OledMoveTo(clearXPos+1, clearYPos);
        OledDrawPixel();
        OledMoveTo(clearXPos+1, clearYPos-1);
        OledDrawPixel();
    }
    else if (clearXPos+1 == 64) {
        if (clearYPos%3 == 0) {
            OledMoveTo(clearXPos+1, clearYPos);
            OledDrawPixel();
        }
        else if ((clearYPos-1)%3 == 0) {
            OledMoveTo(clearXPos+1, clearYPos-1);
            OledDrawPixel();
        }
    }
    else if (clearXPos+1 == 126) {
        if (clearYPos > paddle && clearYPos < paddle-height) {
            OledMoveTo(clearXPos+1, clearYPos);
            OledDrawPixel();
        }
        else if (clearYPos-1 > paddle && clearYPos-1 < paddle-height) {
            OledMoveTo(clearXPos+1, clearYPos-1);
            OledDrawPixel();
        }
    }
    OledUpdate();
    OledSetDrawColor(1);
}

/////////////////////////////////////////////////////////////////
// Function:     randomNumX
// Description:  uses ADC to give a random number to use as ball's X velocity
// Inputs:       None
// Return value: random num between 1 and 5 and the corresponding negatives

int randomNumX()
{
    int a;
    a = (ADC_Port1 + ADC_Port2)%5 + 1;
    if (a%2 == 0) {
        a = a * -1;
    }
    return a;
}

/////////////////////////////////////////////////////////////////
// Function:     randomNumY
// Description:  uses ADC to give a random number to use as ball's Y velocity
// Inputs:       None
// Return value: random num between 1 and 3 and the corresponding negatives

int randomNumY()
{
    int b;
    b = (ADC_Port1 + ADC_Port2)%3 + 1;
    if (b%2 == 0) {
        b = b * -1;
    }
    return b;
}

/////////////////////////////////////////////////////////////////
// Function:     paddleBounce
// Description:  Changes ball speed based on paddle movement
// Inputs:       *Xvel - X velocity of ball to change
//               *Yvel - Y velocity of ball to change
//               oldPaddle - old paddle position
//               newPaddle - new paddle position
// Return value: None

void paddleBounce(int* Xvel, int* Yvel, int oldPaddle, int newPaddle) 
{
    int changePaddle;
    changePaddle = newPaddle - oldPaddle;
    changePaddle = changePaddle/2;
    if (changePaddle == 0) {
        return;
    }
    *Yvel = *Yvel + changePaddle;
    *Xvel++;
    if (*Xvel >= 0) {
        *Xvel = -1;   
    }
    return;
}
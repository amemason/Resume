//-------------------------------------------------------------------------------
//      The code plays the first 20 notes of the Pokemon Red and Blue Title 
//      song, pauses, and then repeats the song. If the port 1 pushbutton is 
//      pushed, the song speeds up, and slows down if it is pushed again.      
//
//
//       Target: TI LaunchPad development board with MSP430G2553 device
//
//       Date:           Dec 5, 2016
//       Last Revision:  1.0
//       Written by:     Amelia Mason
//       Adapted from:   "DemoSoundGeneratorV1"
//                       Dr. Michael Helm, ECE dept, Texas Tech University
//       Assembler/IDE:  IAR Embedded Workbench 5.5
//
//       HW I/O assignments:
//       P1.0    LED1    (Active HIGH)RED
//       P1.1    not used
//       P1.2    not used
//       P1.3    PushButton (Active LOW) (internal Pullup Enabled)
//       P1.4    not used
//       P1.5    not used
//       P1.6    LED2    (Active HIGH)GREEN
//       P1.7    not used
//
//       P2.0    not used
//       P2.1    not used
//       P2.2    not used
//       P2.3    not used
//       P2.4    not used
//       P2.5    not used
//       P2.6    not used
//       P2.7    not used
//
//

#include  <msp430g2553.h>

//----------------------------------------------------------------------
// CONSTANT DEFINITIONS

#define LONG_DELAY    (0x7FFF)   
#define SHORT_DELAY   (0x1FFF)

#define SPEAKER_OUT (0x08u) // (%00001000)

#define LED1RED    (0x01u) //(%00000001)  //Port pin position P1.0
#define LED2GRN   (0x40u)//(%01000000)  //Port pin position P1.6

#define PUSHBUTTON  (0x08u) //(%00001000)  //Port pin position P1.3

#define SPEAKER_PORT P2OUT

#define SONG_LENGTH 20 // song is 20 notes long

//----------------------------------------------------------------------
// GLOBAL VARIABLE definition
int Count = 0;   // count of PushButton presses
int delay = 0;  // delay for 
int duration = 0;  // duration note is played
char note_counter = 0; // counter for how many notes have been played
char fast = 0; // bool flag for if song is in fast mode (1)

 
int NOTE_ARRAY[20] = {196, 257, 294, 370, 392, 392, 392, 392, 392, 392, 392,
349, 349, 349, 349, 349, 370, 392, 494, 587}; // frequency of notes

int DURATION_ARRAY[20] = {160, 160, 160, 160, 300, 300, 120, 120, 300, 300, 300, 
104, 104, 104, 104, 104, 104, 320, 160, 640}; // duration

int NOTE_DELAY[20] = {2550, 1956, 1701, 1351, 1276, 1276, 1276, 1276, 1276, 
1276, 1276, 1445, 1445, 1445, 1445, 1445, 1351, 1276, 1012, 852}; 
// calculated delay

//----------------------------------------------------------------------
// FUNCTIONs
//----------------------------------------------------------------------
// delay_1 - delays by an amount proportional to the argument passed in
//  returns- nothing
//----------------------------------------------------------------------
void delay_1(int DelayTime) 
{
  for(int i = 0;i < DelayTime;i++);
};  // end delay_1 function

void PLAY_NOTE (int delay, int duration, char note_counter) // plays note
{ 
  delay = NOTE_DELAY[SONG_LENGTH-note_counter]/2; // get correct delay
  duration = DURATION_ARRAY[SONG_LENGTH - note_counter]/4; // get duration
  
  if (fast > 0) // if fast(1) then speed up
  {
    duration = duration/2;
  }
  
  while (duration > 0) // play note by generating a squarewave
  {
      SPEAKER_PORT |= SPEAKER_OUT; // on for half cycle
    
      while (delay > 0)
      {
        delay--;
      }
    
      SPEAKER_PORT = (SPEAKER_PORT & ~SPEAKER_OUT); //off for half cycle
  
      delay = NOTE_DELAY[SONG_LENGTH-note_counter];
  
      while (delay > 0)
      {
        delay--;
      }
    
    duration--;
    
  }
};

//----------------------------------------------------------------------
// MAIN PROGRAM 
void main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  
  //P1DIR = 0x41;  // both LEDs               // P1.0 output, else input
  //P1OUT =  0x08; // both LEDs ON, pullup for PB  // P1.4 set, else reset
  //P1REN |= 0x08; // PB position            // P1.4 pullup
  //P1IE |= 0x08;                             // P1.4 interrupt enabled
  //P1IES |= 0x08;                            // P1.4 Hi/lo edge
  //P1IFG &= ~0xFF;                           // P1.4 IFG cleared

  //better coding style to use defined CONSTANTs for I/O assignments:

// set up Port 1  
  P2DIR |= SPEAKER_OUT;
  
  P1REN = PUSHBUTTON;
  P1OUT = PUSHBUTTON;
  
  P1IFG &= ~0xFF;  // clear ALL the Int Flags on Port 1
 
  Count = 0;   // initialize Count to zero

  _BIS_SR(GIE);                 // enable general interrupts
  
   while(1)  // forever
   {
    note_counter = SONG_LENGTH; // set note counter to song length
    
    while (note_counter > 0) // play until the song has reached its end
    {
      PLAY_NOTE(delay, duration, note_counter); // play note
      delay_1(SHORT_DELAY); // pause between notes
      note_counter--; // dec counter
    }
    
    delay_1(LONG_DELAY); // delay between repeats
    
   }
  //return 0;
}  // end of MAIN

// Port 1 interrupt service routine (this will detect pushbutton press on P1)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
  fast = ~fast;                  // LED1RED = toggle
  Count++;   // keep a count of how many times the PB is pressed
  P1IFG = 0;  // clear ALL the Int Flag bits on Port 1
}


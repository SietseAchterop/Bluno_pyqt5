/*
*	Test program for a Bluno m0 board
*            (Cortex-M0, 32 bit)
*   It responds to lines (ending with a carriage return (CR) send over bluetooth 
*   This to test the communication, especially the notifications
*/

#include <EEPROM.h>
#include <TimerOne.h>

// adresses in EEPROM
#define Pval1adr  0
#define Ival1adr  4
#define Dval1adr  8
#define Pval2adr 12
#define Ival2adr 16
#define Dval2adr 20
#define pos1adr  24
#define pos2adr  28
#define freeadr  32


// Encoders
//   we only use 1 interrupt per encoder, resolution probably high enough
// bluno M0 gebruikt D0 en D1 voor uart, D2 en D3 voor i2c, daarom D7 en D8 interrupts gebruiken)
#define  m_ena1 7    // Int. was 2
#define  m_ena2 9
#define  m_enb1 8    // Int. was 3
#define  m_enb2 10

#define  ledje 13

//
byte speed1, speed2    = 0;    // intended motor speed
byte received          = 0;    // # of characters received
volatile int encoder1, oldenc1, encoder2, oldenc2 = 0;
int setpoint1, setpoint2 = 0;
volatile int corr1, corr2;

int  timer1_counter;
byte mot, seconds, control  = 0;

// init from EEPROM/FSM
unsigned int pval1, ival1, dval1, pval2, ival2, dval2;

//
byte comavail  = 0;   // a command is read
char command[10];     // command buffer
byte comind    = 0;   // index into buffer
byte comm      = 0;   // command to be executed
char printstr[22] ;    // string for printing. Langer dan 20 over ble lijkt onbetrouwbaar
// debugging
byte debug       = 1;
byte counter     = 0;


void setup()
{
  // Bluetooth init
  Serial1.begin(115200);
  Serial2.begin(115200);

  pinMode(ledje, OUTPUT);

  retrieve_from_eeprom();
  setpoint1 = encoder1;
  setpoint2 = encoder2;
  

  /*

  noInterrupts();
  
  interrupts();             // enable all interrupts
  */

  //  Timer1.initialize(10000);  // 0.01 second period
  //  Timer1.attachInterrupt(timer1_int);

}

void loop()
{
  // Default response
  sprintf(printstr, "ack\r");

  // read a line
  if(Serial1.available() > 0) 
    {
      received = Serial1.read();
      Serial2.write('='); Serial2.print(received,HEX); Serial2.write('='); Serial2.write('\r');
      switch(received) {
      case '\r' :
	if(comind > 0) {
	  comavail = 1;

	  // verbinding betrouwbaar maken!
	  //   add checksum, ack, retries
	}
	
	break;
      default:
	command[comind] = received;
	comind++;
	if(comind > 9)  // throw away commands that are too long.
	  comind = 0;
	break;
      }
    }

  // parse command, clears buffer
  if(comavail) {

    /* Command structure
       First char: command type.
       Optional second char: boolean,
                             motor number 1,2
       Optional last values: int value

       We assume all commands are correct.
     */

    mot = command[1] - 'l';     // 'l' (motorL) else (motorR)

    switch (command[0]) {
    case 'i':                   // return info
      sprintf(printstr, "XXInfo: %d, %d\r", setpoint1, setpoint2);
      //digitalWrite(ledje, !digitalRead(ledje));
      break;
    case 'X':                   //  save/retrieve data to EEPROM
      if(command[1] == '0')
	save_to_eeprom();
      else
	retrieve_from_eeprom();
      break;

    // next cases have motor number a second char, use mot variable
    case 'e':               // return encoder values
      sprintf(printstr, "Enc: %d, %d\r", encoder1, encoder2);
      break;
    case 'p':               //  set setpoint
      if (!mot)
	setpoint1 = atoi(command+2);
      else
	setpoint2 = atoi(command+2);
      break;

    case 'c':               // control 0: off, else on
      control = !(command[1] == '0');
      sprintf(printstr, "Control: %d\r", control);
      break;

    // lowpower aan/uit (stop timer)

    // initialisatie procedure, door motoren kort vast de laten lopen.
    case 'q':
      //   beide motoren 1 seconde met snelheid x laten lopen, met counter:  qx
      control = 0;     // stop controller
      speed1 = atoi(command+1);
      comm = 1;
      break;

      // set positions to zero in EEPROM after initialisation
      //  als controller uit staat, zet setpoints en encoder waardon op nul en write to eeprom
      
    default:
      // comm = 0;
      break;
    }
    Serial1.print(printstr);
    Serial2.print(printstr);
    // prepare for next command
    comavail = 0;
    comind = 0;
  }

  // execute 'q' command
  switch (comm) {
  case 0:
    break;
  case 1:
    break;
  default:
    comm = 0;
    break;
  }

  digitalWrite(ledje, 0);
}


void save_to_eeprom() {

  EEPROM.put(Pval1adr, pval1);
  EEPROM.put(Ival1adr, ival1);
  EEPROM.put(Dval1adr, dval1);
  EEPROM.put(Pval2adr, pval2);
  EEPROM.put(Ival2adr, ival2);
  EEPROM.put(Dval2adr, dval2);

  EEPROM.put(pos1adr, encoder1);
  EEPROM.put(pos2adr, encoder2);

}

void retrieve_from_eeprom() {

  EEPROM.get(Pval1adr, pval1);
  EEPROM.get(Ival1adr, ival1);
  EEPROM.get(Dval1adr, dval1);
  EEPROM.get(Pval2adr, pval2);
  EEPROM.get(Ival2adr, ival2);
  EEPROM.get(Dval2adr, dval2);

  EEPROM.get(pos1adr, encoder1);
  EEPROM.get(pos2adr, encoder2);
  // ook doorgeven aan controller!  (via i command)

}

// encoder event interrupts
void encoder1Event() {
  if (digitalRead(m_ena1) == HIGH) {
    if (digitalRead(m_ena2) == LOW) {
      encoder1++;
    } else {
      encoder1--;
    }
  } else {
    if (digitalRead(m_ena2) == LOW) {
      encoder1--;
    } else {
      encoder1++;
    }
  }
}

void encoder2Event() {
  if (digitalRead(m_enb1) == HIGH) {
    if (digitalRead(m_enb2) == LOW) {
      encoder2++;
    } else {
      encoder2--;
    }
  } else {
    if (digitalRead(m_enb2) == LOW) {
      encoder2--;
    } else {
      encoder2++;
    }
  }
}

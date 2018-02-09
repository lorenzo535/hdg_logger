

// RTC defs
#include <Time.h>
#include <DS1302RTC.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>

#include <SD.h>


//#include <Ethernet.h>


#define RTC_CE  6
#define RTC_IO  7
#define RTC_CLK  5

#define SD_CS 10

// OLED
#define sclk 13
#define mosi 11
#define cs   A0
#define rst  9
#define dc   8

// Color definitions
#define  BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#define DISLPLAY_OFFSET M_PI/2.0
#include <Fonts/FreeSerif9pt7b.h>


// Set pins:  CE, IO,CLK

// end RTC defs

typedef struct
{
  char header1;
  char header2;
  char header3;
  char msgID;
  byte header4;

  unsigned int  roll;
  unsigned int  pitch;
  unsigned int  heading;

  unsigned int  accel_right;
  unsigned int  accel_fwd;
  unsigned int  accel_up;
  unsigned int  right_magn;
  unsigned int  fwd_magn;
  unsigned int  up_magn;
  char trailer;

} HMRData;


char goodbuffer[24];



// Lorenzo on Arduino Uno: Wiring
/* RTC
 *  CE -->6
 *  IO -->7
 *  CLK-->5
 *
 *  OLED
 *  GND --> GND
 *  VCC --> 5V
 *  SCL --> 13
 *  SDA --> 11
 *  RES --> 9
 *  DC --> 8
 *  CS --> A0
 */



Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
SoftwareSerial mySerial (2, 3);
DS1302RTC RTC(RTC_CE, RTC_IO, RTC_CLK);


float old_endline_x, old_endline_y, y = 0.0, old_y = 0.0;
float endline_x, endline_y;
float heading, pitch, roll;
String filename;
unsigned int logged_lines;
unsigned char readbuffer[128];
int read_chars;
unsigned long millis_since_last_log;
char swapbuffer[2];

char HMR_reset_msg[] = { 0x0d, 0x0A, 0x7E, 0x42, 0x00, 0xD7};

HMRData hmr_data_frame;



void testRTC();
int LogDataLine( float _heading, float _pitch, float _roll);
void ResetBuffer();
void LogAndDisplayData();


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  mySerial.begin (9600);
  mySerial.println ("TEST TEST");

  read_chars = 0;
  memset (readbuffer, 0, 128);
  logged_lines = 0;

  //Oled
  display.begin();

  display.setCursor(5, 8);
  display.setTextColor(WHITE, BLACK);
  display.setTextSize(1);
  display.fillScreen(BLACK);
  old_endline_x = 60;
  old_endline_y = 60;

  lcdTestPattern();
  delay (1000);

  display.setFont(&FreeSerif9pt7b);
  display.print("hello!");

  //RTC
  if (RTC.haltRTC()) {
    mySerial.println("The DS1302 is stopped.  Please run the SetTime");
    mySerial.println("example to initialize the time and begin running.");
    mySerial.println();
  }
  if (!RTC.writeEN()) {
    mySerial.println("The DS1302 is write protected. This normal.");
    mySerial.println();
  }

  // Setup time library
  mySerial.print("RTC Sync   ");
  setSyncProvider(RTC.get);          // the function to get the time from the RTC
  if (timeStatus() == timeSet)
    mySerial.println(" Ok!");
  else
    mySerial.println(" FAIL!");

  delay ( 1000 );

  mySerial.println(" First call");
  testRTC();
  mySerial.println(" end");
  tmElements_t set_tm;
  set_tm.Second = 32;
  set_tm.Minute = 12;
  set_tm.Hour = 8;
  set_tm.Wday = 2;
  set_tm.Day = 1;
  set_tm.Month = 1;
  set_tm.Year = 48; //offset from 1970

  //RTC.write (set_tm);

  testRTC();
/*
  //SD
  // SD Card Initialization
  if (SD.begin(SD_CS))
  {
    mySerial.println("SD card is ready to use.");
  } else
  {
    mySerial.println("SD card initialization failed");
   
  }
*/
  Serial.write (HMR_reset_msg, 6);
  delay(500);
  Serial.write (HMR_reset_msg, 6);
  delay(500);
  

  ResetBuffer();
  //Reset interrupts
  cli();

  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  //activate interrupts
  sei();
  millis_since_last_log = millis();
}

void ResetBuffer()
{
  read_chars = 0;
  memset (&hmr_data_frame, 0, sizeof(hmr_data_frame));
  memset (readbuffer, 0, 128);
  //mySerial.println("resetbuffer");
}


// Interrupt is called once a millisecond, looks for any new HMR3500 data, and stores it
SIGNAL(TIMER0_COMPA_vect)
{ 
  unsigned char c;
/*
//if (Serial.available())
    c = Serial.read();
   if (c) 
   {
    mySerial.write(c);

    if (read_chars > 127)
    ResetBuffer();
    
    readbuffer[read_chars] = c;
    read_chars ++;
    
   }
 */
}



void loop()
{

char c;
   while (Serial.available())
   {
    c = Serial.read();   
   //mySerial.print(c);

    if (read_chars > 127)
    ResetBuffer();
    
    readbuffer[read_chars] = c;
    read_chars ++;
   }
 
 
 if (read_chars >= 24)
 {
  TryToDecode();        
  }

  LogAndDisplayData();
  
  delay (10);  
 
}


void LogAndDisplayData()
{
  if (millis() - millis_since_last_log < 1000)
  return;

  millis_since_last_log= millis();
    
  LogDataLine (heading, pitch, roll);
  
}



void TryToDecode()
{
  int i,j;
  
  for (i = 0; i < read_chars; i++)
  {
    if (readbuffer[i] == 0x70)
    {
    if ((i >3) && ( readbuffer[i-3] == 0x0D) && ( readbuffer[i-2] == 0x0A)&& ( readbuffer[i-1] == 0x7E))
    {
    //found message frame
    //mySerial.println ("found  message start");
        if ( (i + sizeof(hmr_data_frame)+1) <= read_chars)
        {
              
          for (j = 0; j< 24; j++)
          {
          goodbuffer[j] = readbuffer[j+i-3];
          //mySerial.write (goodbuffer[j]);
          }
        
         memcpy (&hmr_data_frame,  goodbuffer, sizeof(hmr_data_frame));
         //mySerial.println ("copied full message");
        
         heading = (float)hmr_data_frame.heading;
         heading = heading /65535 * 360.0;
         
         mySerial.print (" heading : ");
         mySerial.println (heading);
         
         pitch = (float)hmr_data_frame.pitch;
         pitch = pitch /65535 * 360.0;
         
         mySerial.print (" pitch : ");
         mySerial.println (pitch);


         roll = (float)hmr_data_frame.roll;
         roll = roll /65535 * 360.0;
         
         mySerial.print (" roll : ");
         mySerial.println (roll);
                
        ResetBuffer();
        return;      
        } 
        else return;    
      }  
    }
  }

}
 

/*
  if (Serial.available())
    mySerial.write (Serial.read());
  else
    mySerial.println ("nada");
  delay (2000);

  heading += 0.1;
  pitch += 0.2;
  roll += 0.15;

  logged_lines += LogDataLine(heading, pitch, roll);
  display.setCursor(5, 8);
  display.print ("lines:");
  display.print (logged_lines);
  */






void displayOnOled(float yaw)
{
  //yaw is radiant

  // OLED
  display.setCursor(5, 12);
  display.setTextColor(BLACK);
  display.println(old_y, 1);
  display.setCursor(5, 12);
  display.setTextColor(WHITE);
  display.println (yaw, 1);
  old_y = yaw;
  endline_x = 40 + 20 * cos(yaw + DISLPLAY_OFFSET);
  endline_y = 40 + 20 * sin(yaw + DISLPLAY_OFFSET);
  display.drawLine (40, 40, old_endline_x, old_endline_y, BLACK);
  display.drawLine (40, 40, endline_x, endline_y, BLUE);
  old_endline_x = endline_x;
  old_endline_y = endline_y;
}


void testRTC()
{
  return;
  tmElements_t tm;

  Serial.print("UNIX Time: ");
  Serial.print(RTC.get());

  if (! RTC.read(tm)) {
    Serial.print("  Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(", DoW = ");
    Serial.print(tm.Wday);
    Serial.println();
  } else {
    Serial.println("DS1302 read error!  Please check the circuitry.");
    Serial.println();
    delay(9000);
  }

  // Wait one second before repeating :)
  delay (1000);
}


void CreateFilename()
{
  RTC.get();
  tmElements_t tm;

  if (! RTC.read(tm)) {
    filename = String (tm.Month) + String (tm.Day) + String (tm.Hour) + String (tm.Minute) + ".csv";
  }
  else
    filename = "default.txt";

  display.setCursor(5, 9);
  display.print (filename);
}


void print2digits(int number)
{
  if (number >= 0 && number < 10)
    Serial.write('0');
  Serial.print(number);
}


void lcdTestPattern(void)
{
  uint32_t i, j;
  display.goTo(0, 0);

  for (i = 0; i < 64; i++)
  {
    for (j = 0; j < 96; j++)
    {
      if (i > 55) {
        display.writeData(WHITE >> 8);
        display.writeData(WHITE);
      }
      else if (i > 47) {
        display.writeData(BLUE >> 8);
        display.writeData(BLUE);
      }
      else if (i > 39) {
        display.writeData(GREEN >> 8);
        display.writeData(GREEN);
      }
      else if (i > 31) {
        display.writeData(CYAN >> 8);
        display.writeData(CYAN);
      }
      else if (i > 23) {
        display.writeData(RED >> 8);
        display.writeData(RED);
      }
      else if (i > 15) {
        display.writeData(MAGENTA >> 8);
        display.writeData(MAGENTA);
      }
      else if (i > 7) {
        display.writeData(YELLOW >> 8);
        display.writeData(YELLOW);
      }
      else {
        display.writeData(BLACK >> 8);
        display.writeData(BLACK);
      }
    }
  }
}


int LogDataLine( float _heading, float _pitch, float _roll)
{
  File logfile;
  tmElements_t tm;

  logfile = SD.open(filename, FILE_WRITE);
  if (logfile)
  {
    RTC.get();
    RTC.read(tm);
    logfile.print("  Time = ");
    logfile.print(tm.Hour);
    logfile.print(':');
    logfile.print(tm.Minute);
    logfile.print(':');
    logfile.print(tm.Second);
    logfile.print(',');
    logfile.print("h,");
    logfile.print(String (_heading, 1));
    logfile.print(",p,");
    logfile.print(String (_pitch, 1));
    logfile.print(",r,");
    logfile.println(String (_roll, 1));

    logfile.close(); // close the file
    return 1;
  }
  // if the file didn't open, print an error:
  else {
    mySerial.println("error opening logfile");
    return 0;
  }
}



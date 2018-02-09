/*
   Uses Web Server by Tom Igoe
 */

// RTC defs
#include <Time.h>
#include <DS1302RTC.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial (2,3);

#define RTC_CE  6
#define RTC_IO  7
#define RTC_CLK  5

// Set pins:  CE, IO,CLK
DS1302RTC RTC(RTC_CE, RTC_IO, RTC_CLK);
uint8_t ramBuffer[31];


typedef struct 
{
  byte header1;
  byte header2;
  byte msgID;
  byte header4;
  
  unsigned int  heading;
  unsigned int  pitch;
  unsigned int  roll;

  unsigned int  accel_right;
  unsigned int  accel_fwd;
  unsigned int  accel_up;
  unsigned int  right_magn;
  unsigned int  fwd_magn;
  unsigned int  up_magn;
  byte trailer;
  
} HMRData;


// end RTC defs

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

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>

#include <SD.h>

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
float old_endline_x, old_endline_y, y = 0.0, old_y = 0.0;
float endline_x, endline_y;
float heading, pitch, roll;
String filename;
unsigned int logged_lines;

#define DISLPLAY_OFFSET M_PI/2.0
#include <Fonts/FreeSerif9pt7b.h>

#include <SPI.h>
//#include <Ethernet.h>

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

//byte TCM_get_data[] = { 0x00, 0x05, 0x04, 0xBF, 0x71};
byte TCM_get_data[] = { 0x00, 0x05, 0x01, 0xEF, 0xd4};

//IPAddress ip(192, 168, 1, 177);


//EthernetServer server(80);

void testRTC();
int LogDataLine( float _heading, float _pitch, float _roll);


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(38400);

  mySerial.begin (4800);
  mySerial.println ("TEST TEST");


  logged_lines = 0;
  /*
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
*/
  //Oled
  display.begin();

  display.setCursor(5,8);
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
  if(timeStatus() == timeSet)
    mySerial.println(" Ok!");
  else
    mySerial.println(" FAIL!");
  
  delay ( 2000 );

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
    
//SD
// SD Card Initialization
  if (SD.begin(SD_CS))
  {
    mySerial.println("SD card is ready to use.");
  } else
  {
    mySerial.println("SD card initialization failed");
    return;
  }

}

void loop()
{
 
 Serial.write (TCM_get_data, 5);
 delay (100);
 if (Serial.available())
 mySerial.write (Serial.read());
 else  
 mySerial.println ("nada");
 delay (2000); 

 heading += 0.1;
 pitch += 0.2;
 roll +=0.15;
 
 logged_lines += LogDataLine(heading, pitch, roll);  
 display.setCursor(5,8);
 display.print ("lines:");
 display.print (logged_lines);
 }

/*
void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 1; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
*/

void displayOnOled(float yaw)
{
   //yaw is radiant
   
   // OLED
    display.setCursor(5,12);
    display.setTextColor(BLACK);
    display.println(old_y,1);
    display.setCursor(5,12);
    display.setTextColor(WHITE);
    display.println (yaw, 1);   
    old_y = yaw;         
    endline_x = 40+20*cos(yaw + DISLPLAY_OFFSET);
    endline_y = 40+20*sin(yaw + DISLPLAY_OFFSET);
    display.drawLine (40,40, old_endline_x, old_endline_y, BLACK);
    display.drawLine (40,40, endline_x, endline_y, BLUE);
    old_endline_x = endline_x;
    old_endline_y = endline_y;     
}


void testRTC()
{
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
    filename = String (tm.Month) +String (tm.Day) + String (tm.Hour) + String (tm.Minute) + ".csv";
  }
  else
  filename = "default.txt";

  display.setCursor(5,9);
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
  uint32_t i,j;
  display.goTo(0, 0);
  
  for(i=0;i<64;i++)
  {
    for(j=0;j<96;j++)
    {
      if(i>55){display.writeData(WHITE>>8);display.writeData(WHITE);}
      else if(i>47){display.writeData(BLUE>>8);display.writeData(BLUE);}
      else if(i>39){display.writeData(GREEN>>8);display.writeData(GREEN);}
      else if(i>31){display.writeData(CYAN>>8);display.writeData(CYAN);}
      else if(i>23){display.writeData(RED>>8);display.writeData(RED);}
      else if(i>15){display.writeData(MAGENTA>>8);display.writeData(MAGENTA);}
      else if(i>7){display.writeData(YELLOW>>8);display.writeData(YELLOW);}
      else {display.writeData(BLACK>>8);display.writeData(BLACK);}
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
    logfile.print(String (_heading,1));
    logfile.print(",p,");
    logfile.print(String (_pitch,1));
    logfile.print(",r,");
    logfile.println(String (_roll,1));

    logfile.close(); // close the file  
    return 1;
  }
  // if the file didn't open, print an error:
  else {
    mySerial.println("error opening logfile");
    return 0;
  }
}



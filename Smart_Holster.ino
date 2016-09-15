// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code shows how to listen to the GPS module in an interrupt
// which allows the program to have more 'freedom' - just parse
// when a new NMEA sentence is available! Then access data when
// desired.
//
// Tested and works great with the Adafruit Ultimate GPS module
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/746
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
// include the SD library:
#include <SPI.h>
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card card;
//SdVolume volume;
//SdFile root;
// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 10;
File file;
char* filename = "GPS_DATA.txt";


// If you're using a GPS module:
// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// If using software serial (sketch example default):
//   Connect the GPS TX (transmit) pin to Digital 3
//   Connect the GPS RX (receive) pin to Digital 2
// If using hardware serial (e.g. Arduino Mega):
//   Connect the GPS TX (transmit) pin to Arduino RX1, RX2 or RX3
//   Connect the GPS RX (receive) pin to matching TX1, TX2 or TX3

// If you're using the Adafruit GPS shield, change 
// SoftwareSerial mySerial(3, 2); -> SoftwareSerial mySerial(8, 7);
// and make sure the switch is set to SoftSerial

// If using software serial, keep this line enabled
// (you can change the pin numbers to match your wiring):
SoftwareSerial mySerial(8, 7);

// If using hardware serial (e.g. Arduino Mega), comment out the
// above SoftwareSerial line, and enable this line instead
// (you can change the Serial number to match your wiring):

//HardwareSerial mySerial = Serial1;


Adafruit_GPS GPS(&mySerial);


// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences. 
#define GPSECHO  false

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

const int button1 = 5;
const int button2 = 6;
int button1State = 0;
int button2State = 0;
const int level2LED = 9;
const int level3LED = 10;
String normalized;

long scaledLat;
long scaledLong;

void setup()  
{
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(level2LED, OUTPUT);
  pinMode(level3LED, OUTPUT);
  
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  //Serial.begin(9600);
  
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
  pinMode(chipSelect, OUTPUT);
  
  if (!SD.begin(chipSelect)) {
    //Serial.println(F("initialization failed!"));
    return;
  }
  //if file currently exists, delete so we dont start appending to the end
  if (SD.exists(filename)) SD.remove(filename);
  file = SD.open(filename, FILE_WRITE);

  delay(1000);
  // Ask for firmware version
  mySerial.println(PMTK_Q_RELEASE);
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO)
    if (c) UDR0 = c;  
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
#endif
}


void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}


uint32_t timer = millis();

void loop()                     // run over and over again
{
  String catVal = "";
  //File file;
  button1State = digitalRead(button1);
  button2State = digitalRead(button2);
  if (button1State == HIGH) {
   digitalWrite(level2LED, HIGH);
 } else {
   digitalWrite(level2LED, LOW);
 }
 if (button2State == HIGH) {
   digitalWrite(level3LED, HIGH);
 } else {
   digitalWrite(level3LED, LOW);
 }
 
  // in case you are not using the interrupt above, you'll
  // need to 'hand query' the GPS, not suggested :(
 if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
  char c = GPS.read();
    // if you want to debug, this is a good time to do it!
  //if (GPSECHO)
    //if (c) Serial.print(c);
}

  // if a sentence is received, we can check the checksum, parse it...
if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();

  // approximately every 2 seconds, print out the current stats
  if (millis() - timer > 2000) { 
    timer = millis(); // reset the timer
    file = SD.open(filename, FILE_WRITE);
    
    if (file) {
      if (GPS.fix == 1) {   
      //Serial.println(F("has fix and file"));

        if(GPS.hour < 10) file.print("0");      //all of these '< 10' checks are to prepend a '0' to single digits for db entry
        file.print(GPS.hour); file.print(":");

        if(GPS.minute < 10) file.print("0");
        file.print(GPS.minute, DEC); file.print(":");
        
        if(GPS.seconds < 10) file.print("0");
        file.print(GPS.seconds, DEC); file.print(",");
        
      file.print("20"); file.print(GPS.year, DEC); file.print("-");     //normalized with a '20' in front for YYYY
      
      if (GPS.month < 10) file.print("0");
      file.print(GPS.month, DEC); file.print("-");
      
      if (GPS.day < 10) file.print("0");
      file.print(GPS.day, DEC); file.print(",");

      file.print(GPS.fix);
      file.print(",");
      file.print(scale(GPS.latitude), 4);
      //file.print(GPS.latitude, 4);
      //scaledLat = scale(GPS.latitude);
      file.print(",");
      //scaledLong = scale(GPS.longitude);
      //file.print(-GPS.longitude, 4);    //hacky negative since we're always in the western hemisphere. can scale in the future by parsing the W or E out of the full nmea within the adafrui_gps library
      file.print(-(scale(GPS.longitude)), 4);
      file.print(",");
      file.print(button1State);
      file.print(",");
      file.println(button2State);
      file.close();
    } else {
     // Serial.println(F("fix failed")); 

    }  
  } else {
     // Serial.println(F("file failed"));
    }
  }
}
     




//Verbose printing of all data recieved. Kept only to reference the fields available within the GPS object
/*
    if (GPS.fix) {
      Serial.print("Location: ");\]`\
      
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", "); 
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Location (in degrees, works with Google Maps): ");
      Serial.print(GPS.latitudeDegrees, 4);
      Serial.print(", "); 
      Serial.println(GPS.longitudeDegrees, 4);
      
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
    }
    Serial.print("time=");
    Serial.print(GPS.hour, DEC); Serial.print(":");
    Serial.print(GPS.minute, DEC); Serial.print(":");
    Serial.print(GPS.seconds, DEC); Serial.print(",");
    Serial.print("date=");
    Serial.print(GPS.year, DEC); Serial.print("-");
    Serial.print(GPS.month, DEC); Serial.print("-");
    Serial.print(GPS.day, DEC); Serial.print(",");
    Serial.print("fix=");
    Serial.print(GPS.fix);
    Serial.print(",lat=");
    Serial.print(GPS.latitudeDegrees);
    Serial.print(",long=");
    Serial.print(GPS.longitudeDegrees);
    Serial.print(",levelTwo=");
    Serial.print(button1State);
    Serial.print(",levelThree=");
    Serial.println(button2State);
    */

float scale(float preciseNum) {
  int deg = preciseNum / 100;
  int add = 0;
  //Serial.println(deg);
  long scaledNum = preciseNum * 10000;
  String str = "";
  str += scaledNum;
  String begStr;
  String endStr;
  if (str.charAt(0) != '-') {
  //Serial.println(str);
  begStr = str.substring(2, 4);
  endStr = str.substring(4);
  add = 1;
  } else {
  begStr = str.substring(3, 5);
  endStr = str.substring(5);  
  //Serial.println("dsaf;sld");  
  add = 0;
  }
  str = begStr + "." + endStr;
  //Serial.println(str);
  float mins = str.toFloat();
  mins /= 60;
  //Serial.println(mins, 4);
  if (add) return deg + mins;
  else return deg - mins;
}


/*
########################
#        OVERVIEW      #
########################

 Example D: Checks all addresses for active sensors, and logs data for each sensor every minute.

 This is a simple demonstration of the SDI-12 library for Arduino.

 It discovers the address of all sensors active on a single bus and takes measurements from them.
 Every SDI-12 device is different in the time it takes to take a measurement, and the amount of data it returns.
 This sketch will not serve every sensor type, but it will likely be helpful in getting you started.
 Each sensor should have a unique address already - if not, multiple sensors may respond simultaenously
 to the same request and the output will not be readable by the Arduino.

 To address a sensor, please see Example B: b_address_change.ino

#########################
#      THE CIRCUIT      #
#########################

 You  may use one or more pre-adressed sensors.

 See:
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_usb_multiple_sensors.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb_multiple_sensors.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_usb.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb.png

###########################
#      COMPATIBILITY      #
###########################

 This library requires the use of pin change interrupts (PCINT).
 Not all Arduino boards have the same pin capabilities.
 The known compatibile pins for common variants are shown below.

 Arduino Uno: 	All pins.

 Arduino Mega or Mega 2560:
 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62),
 A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).

 Arduino Leonardo:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)

#########################
#      RESOURCES        #
#########################

 Written by Kevin M. Smith in 2013.
 Contact: SDI12@ethosengineering.org

 The SDI-12 specification is available at: http://www.sdi-12.org/
 The library is available at: https://github.com/EnviroDIY/Arduino-SDI-12
*/


#include <SDI12.h>

#define SERIAL_BAUD 115200  // The baud rate for the output serial port
#define DATA_PIN 10         // The pin of the SDI-12 data bus
#define POWER_PIN -1       // The sensor power pin (or -1 if not switching power)

// Define the SDI-12 bus
SDI12 mySDI12(DATA_PIN);

// keeps track of active addresses
// each bit represents an address:
// 1 is active (taken), 0 is inactive (available)
// setTaken('A') will set the proper bit for sensor 'A'
byte addressRegister[8] = {
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000
};

uint8_t numSensors = 0;


// converts allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
// to a decimal number between 0 and 61 (inclusive) to cover the 62 possible addresses
byte charToDec(char i){
  if((i >= '0') && (i <= '9')) return i - '0';
  if((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if((i >= 'A') && (i <= 'Z')) return i - 'A' + 37;
  else return i;
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
// maps a decimal number between 0 and 61 (inclusive) to
// allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
char decToChar(byte i){
  if((i >= 0) && (i <= 9)) return i + '0';
  if((i >= 10) && (i <= 36)) return i + 'a' - 10;
  if((i >= 37) && (i <= 62)) return i + 'A' - 37;
  else return i;
}

void printBufferToScreen(){
  String buffer = "";
  //mySDI12.read(); // consume address
  while(mySDI12.available()){
    char c = mySDI12.read();
    if(c == '+'){
      buffer += ',';
    }
    else if ((c != '\n') && (c != '\r')) {
      buffer += c;
    }
    delay(50);
  }
  Serial.println("Buffer:");
 Serial.println(buffer);
 Serial.println();
}

// gets identification information from a sensor, and prints it to the serial port
// expects a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
void printInfo(char i){
  String command = "";
  command += (char) i;
  command += "I!";
  mySDI12.sendCommand(command);
  // Serial.print(">>>");
  // Serial.println(command);
  delay(30);

  printBufferToScreen();
}

void takeMeasurement(char i){
  String command = "";
  command += i;
  command += "M!"; // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command);
  delay(30);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of measurments available, 0-9]
  String sdiResponse = "";
  delay(30);
  while (mySDI12.available())  // build response string
  {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r'))
    {
      sdiResponse += c;
      delay(5);
    }
  }
  mySDI12.clearBuffer();

  // find out how long we have to wait (in seconds).
  uint8_t wait = 0;
  wait = sdiResponse.substring(1,4).toInt();

  // Set up the number of results to expect
   int numMeasurements =  sdiResponse.substring(4,5).toInt();

Serial.println(numMeasurements);
/*
  unsigned long timerStart = millis();
  while((millis() - timerStart) < (1000 * wait)){
    if(mySDI12.available())  // sensor can interrupt us to let us know it is done early
    {
      mySDI12.clearBuffer();
      break;
    }
  }
  // Wait for anything else and clear it out
  */
  delay(2000);
  
  delay(30);
  mySDI12.clearBuffer();

  // in this example we will only take the 'DO' measurement
  command = "";
  command += i;
  command += "D0!"; // SDI-12 command to get data [address][D][dataOption][!]
  mySDI12.sendCommand(command);
  while(!mySDI12.available()>1); // wait for acknowlegement
 
  delay(300); // let the data transfer

  
  printBufferToScreen();
  mySDI12.clearBuffer();

  
}

void DMeasurement(char i){   

  float junk;
  int CTDdepth;
  float CTDtemp,CTDcond;
  
  String command = ""; 
  command += i;
  command += "M!"; // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command); 
  delay(2000); // wait a while
  mySDI12.flush(); // we don't care about what it sends back
  mySDI12.clearBuffer();
  
  command = "";
  command += i;
  command += "D0!"; // SDI-12 command to get data [address][D][dataOption][!]
  mySDI12.sendCommand(command);

  while(!mySDI12.available()>1); // wait for acknowlegement
  delay(300); // let the data transfer
  
  //printBufferToScreen();
  //mySDI12.clearBuffer();

     if(mySDI12.available() > 0){
        int add = mySDI12.parseInt();
        int va = mySDI12.parseInt();
        float vb = mySDI12.parseFloat();
        float vc = mySDI12.parseFloat();

       // Serial.print("address=");
        Serial.print(add);
        Serial.print(", ");
        Serial.print(va);
        Serial.print(", ");
        Serial.print(vb);
        Serial.print(", ");
        Serial.print(vc);
        Serial.println();
        //Serial.println(add);
       // Serial.println(va);
        //Serial.println(vb);
       // Serial.println(vc);
     }


  mySDI12.flush(); 
  mySDI12.clearBuffer();

  
 }     

 
// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i){

  String myCommand = "";
  myCommand = "";
  myCommand += (char) i;                 // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for(int j = 0; j < 3; j++){            // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    delay(30);
    if(mySDI12.available()) {  // If we here anything, assume we have an active sensor
      printBufferToScreen();
      mySDI12.clearBuffer();
      return true;
    }
  }
  mySDI12.clearBuffer();
  return false;
}

// this quickly checks if the address has already been taken by an active sensor
boolean isTaken(byte i){
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  return addressRegister[j] & (1<<k); // return bit status
}

// this sets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setTaken(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] |= (1 << k);
  return !initStatus; // return false if already taken
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
// this unsets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setVacant(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] &= ~(1 << k);
  return initStatus; // return false if already vacant
}


void setup(){
  Serial.begin(SERIAL_BAUD);
  while(!Serial);

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500); // allow things to settle

  // Power the sensors;
  if(POWER_PIN > 0){
    Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(200);
  }

  Serial.println("Scanning all addresses, please wait...");
  /*
      Quickly Scan the Address Space
   */

  for(byte i = '0'; i <= '9'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space 0-9

 for(byte i = 'a'; i <= 'z'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space a-z

  for(byte i = 'A'; i <= 'Z'; i++) if(checkActive(i)) {numSensors++; setTaken(i);}   // scan address space A-Z

  /*
      See if there are any active sensors.
   */
  boolean found = false;

  for(byte i = 0; i < 62; i++){
    if(isTaken(i)){
      found = true;
      Serial.print("First address found:  ");
      Serial.println(decToChar(i));
      Serial.print("Total number of sensors found:  ");
      Serial.println(numSensors);
      break;
    }
  }

  if(!found) {
    Serial.println("No sensors found, please check connections and restart the Arduino.");
    while(true);
  } // stop here

  //Serial.println();
  //Serial.println("Time Elapsed (s), Sensor Address and ID, Measurement 1, Measurement 2, ... etc.");

  Serial.println("-------------------------------------------------------------------------------");
  
       Serial.println("address,va,vb,vc");
  Serial.println("-------------------------------------------------------------------------------");
}

void loop(){

  // scan address space 0-9
  for(char i = '0'; i <= '9'; i++) if(isTaken(i)){
    //Serial.print(millis()/1000);
    //Serial.print(",\t");
   // printInfo(i);
   // Serial.print(",\t");
   
    //takeMeasurement(i);
    DMeasurement(i);
    
    //Serial.println();
  }


  // scan address space a-z
  for(char i = 'a'; i <= 'z'; i++) if(isTaken(i)){
    Serial.print(millis()/1000);
    Serial.print(",\t");
    printInfo(i);
    Serial.print(",\t");
     //takeMeasurement(i);
         DMeasurement(i);

    Serial.println();
  }

  // scan address space A-Z
  for(char i = 'A'; i <= 'Z'; i++) if(isTaken(i)){
    Serial.print(millis()/1000);
    Serial.print(",\t");
    printInfo(i);
    Serial.print(",\t");
     //takeMeasurement(i);
         DMeasurement(i);

    Serial.println();
  };

  delay(10000); // wait ten seconds between measurement attempts.

Serial.println("------------------------");
}

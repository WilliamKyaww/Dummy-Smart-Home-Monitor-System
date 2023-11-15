#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
bool rep = true;
unsigned long lastChecked = 0;

#define BUTTON_UP 8
#define BUTTON_DOWN 4
#define DEBOUNCE_DELAY 1000

struct Device {
  String id;
  char type;
  String location;
  String state;
  int power = 0;
};

const int MAXDEVICES = 10; //max number of devices allowed
Device devices[MAXDEVICES]; 
int numDevices = 0; //number for devices out of max
int currentDevice = 0; //index for up/down display

//------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lastChecked = millis();
  sync();
}

//------------------------------------------------------------------------------------------------------------

void sync(){
  lcd.setBacklight(5); //purple
  while (rep == true) {
    Serial.print("Q"); 
    delay(1000); 

    if (Serial.available()) {
      char c = Serial.read();
      while (Serial.available()){
        Serial.read(); //clearing out old data
      }

      if (c == 'X') {
        rep = false;
      }
    }
  }
  Serial.println("BASIC \n");
  lcd.setBacklight(7); //white or light up
}

//------------------------------------------------------------------------------------------------------------

void input(){
  ButtonsPress();
  Serial.println("Input:");
  while(Serial.available() == 0){}
  String inputString = Serial.readStringUntil('\n');

  Serial.println(inputString);

while (numDevices == 0) {
  if (inputString.charAt(0) != 'A'){
    Serial.println("ERROR: Need to add a device first");

    Serial.println("---------------------------");
    Serial.println("There are currently " + String(numDevices) + "/" + MAXDEVICES + " devices");
    Serial.println("Input:");
    while(Serial.available() == 0){}
    inputString = Serial.readStringUntil('\n');

    Serial.println(inputString);
  }
  else{
    break;
  }
}

switch(inputString.charAt(0)){
  case 'A':
    addDevice(inputString);
    break;
  case 'S':
    setState(inputString);
    break;
  case 'P':
    setPower(inputString);
    break;
  case 'R':
    removeDevice(inputString);
    break;
  default:
    Serial.println("Invalid input!");
  }
}

//------------------------------------------------------------------------------------------------------------

void addDevice(String inputString){
    char option = inputString.charAt(0);  
    String id = inputString.substring(2, 5); 
    id.trim();
    char type = inputString.charAt(6); 
    String location = inputString.substring(8); 
    location.trim();

    // Device UPDATE - Check if device already exists
    for (int i = 0; i < numDevices; i++) {
        if (devices[i].id == id) {
            devices[i].type = type;
            devices[i].location = location;
            Serial.println("Device updated!");
            Serial.println("Device index: " + String(i));
            Serial.println("Device ID: " + devices[i].id);
            Serial.println("Device type: " + String(devices[i].type));
            Serial.println("Device location: " + devices[i].location);
            Serial.println("Device state: " + devices[i].state);
            Serial.println("Device power: " + String(devices[i].power));
            currentDevice = i;
            displayDevice(devices[currentDevice]);
            return;
        }
    }

    // Device ADD - Add new device
    if (numDevices < MAXDEVICES) {
        Device newDevice;
        newDevice.id = id;
        newDevice.type = type;
        newDevice.location = location;
        newDevice.state = "OFF";
        newDevice.power = 0;
        devices[numDevices] = newDevice;
        currentDevice = numDevices;
        displayDevice(devices[currentDevice]); //numDevice refers to the current device
        numDevices++;
        
      
      // Verify device fields are stored
      Serial.println();
      Serial.println("Device added!");
      Serial.println("Device index: " + String(numDevices - 1));
      Serial.println("Device ID: " + devices[numDevices - 1].id);
      Serial.println("Device type: " + String(devices[numDevices - 1].type));
      Serial.println("Device location: " + devices[numDevices - 1].location);
      Serial.println("Device state: " + devices[numDevices - 1].state);
      Serial.println("Device power: " + String(devices[numDevices - 1].power));
    } else {
        Serial.println("ERROR: Maximum number of devices reached");

        //Printing out all the devices on the serial monitor for testing purposes
        Serial.println();
        Serial.println("---------------------------");
          for (int i = 0; i < MAXDEVICES; i++) {
            Serial.print("Device index: ");
            Serial.println(i);
            Serial.print("Device ID: ");
            Serial.print(devices[i].id);
            Serial.print(", Device type: ");
            Serial.print(String(devices[i].type));
            Serial.print(", Device location: ");
            Serial.print(devices[i].location);
            Serial.print(", Device state: ");
            Serial.print(devices[i].state);
            Serial.print(", Device power: ");
            Serial.println(String(devices[i].power));
            Serial.println();
          } 
      }

}

//------------------------------------------------------------------------------------------------------------

void setState(String inputString) {
  char option = inputString.charAt(0);  
  String id = inputString.substring(2, 5); 
  id.trim();
  String state = inputString.substring(6); 
  state.trim();

  if (state != "ON" && state != "OFF") {
    Serial.println("ERROR: state!");
  } else {
    if (state == "ON"){
      state += " ";
    }

    // Find the device with the matching ID
    bool deviceFound = false;
    for (int i = 0; i < numDevices; i++) {
      if (devices[i].id == id) {
        // Update the state of the device
        devices[i].state = state;
        Serial.println("State of device " + devices[i].id + " set to " + devices[i].state + ".");
        currentDevice = i;
        displayDevice(devices[currentDevice]);
        deviceFound = true;
        break;
      }
    }

    if (!deviceFound) {
      Serial.println("ERROR: Device not found!");
    }
  }
}

//------------------------------------------------------------------------------------------------------------

void setPower(String inputString) {
  char option = inputString.charAt(0);  
  String id = inputString.substring(2, 5); 
  id.trim();
  String powerStr = inputString.substring(6); 
  powerStr.trim();
  int power = powerStr.toInt(); // convert power value to integer

  // Find the device with the matching ID
  bool deviceFound = false;
  for (int i = 0; i < numDevices; i++) {
    if (devices[i].id == id) {
      // Check the power range based on device type
      if (devices[i].type == 'S' && (power < 0 || power > 100)) {
        Serial.println("ERROR: Speaker volume should be between 0 and 100 inclusive");
        deviceFound = true;
      }
      else if (devices[i].type == 'L' && (power < 0 || power > 100)) {
        Serial.println("ERROR: Light intensity should be between 0 and 100 inclusive");
        deviceFound = true;
      }
      else if (devices[i].type == 'T' && (power < 9 || power > 32)) {
        Serial.println("Invalid. Thermostat temperature should be between 9 and 32 inclusive");
        deviceFound = true;
      }
      else if (devices[i].type == 'O' || devices[i].type == 'C') {
        deviceFound = true;
      }
      else {
        // Update the power of the device
        devices[i].power = power;
        Serial.println("Power of device " + devices[i].id + " set to " + String(devices[i].power) + ".");
        
        currentDevice = i;
        displayDevice(devices[currentDevice]);
        deviceFound = true;
      }
      break;
    }
  }
  if (!deviceFound) {
    Serial.println("Device not found!");
  }
}

//------------------------------------------------------------------------------------------------------------

void removeDevice(String inputString) {
  char option = inputString.charAt(0);
  String id = inputString.substring(2, 5);
  id.trim();

  // Find the device with the matching ID
  bool deviceFound = false;

  for (int i = 0; i < numDevices; i++) {
    if (devices[i].id == id) {
      // Shift all devices after the deleted device back by one
      for (int j = i; j < numDevices - 1; j++) {
        devices[j] = devices[j+1];
      }
      numDevices--;
      Serial.println("Device " + id + " removed.");
      deviceFound = true;
      break;
    }
  }

  if (!deviceFound) {
    Serial.println("Device not found!");
  }
}

//------------------------------------------------------------------------------------------------------------

void displayDevice(Device device) {
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("^" + devices[currentDevice].id + " " + String(devices[currentDevice].location));

  if (devices[currentDevice].type == 'S' || devices[currentDevice].type == 'L'){
    lcd.setCursor(0,1);
    lcd.print("v" + String(devices[currentDevice].type) + " " + devices[currentDevice].state + " " + devices[currentDevice].power + "%");
  }
  else if (devices[currentDevice].type == 'T'){
    lcd.setCursor(0,1);
    lcd.print("v" + String(devices[currentDevice].type) + " " + devices[currentDevice].state + " " + devices[currentDevice].power + char(223) + "C");
  }
  else if (devices[currentDevice].type == 'O' || devices[currentDevice].type == 'C'){
    lcd.setCursor(0,1);
    lcd.print("v" + String(devices[currentDevice].type) + " " + devices[currentDevice].state);
  }

  if (devices[currentDevice].state == "ON "){
    lcd.setBacklight(2); //green backlight
  }
  else {
    lcd.setBacklight(3); //yellow backlight

  }

}

//------------------------------------------------------------------------------------------------------------

void displayF228295(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("F228295");
  lcd.setBacklight(5); //purple
}

//------------------------------------------------------------------------------------------------------------

void ButtonsPress(){

  uint8_t pressedButtons = lcd.readButtons();
  //Serial.println("A button is pressed");
  
  if ((lastChecked + 1000) < millis()) {
    lastChecked = millis();
    
    if (BUTTON_UP & pressedButtons) {
      Serial.println("Up button pressed");
      currentDevice ++;
      if (currentDevice >= MAXDEVICES) {
        currentDevice = 0;  // wrap around to the first device
      }
      displayDevice(devices[currentDevice]);
      delay(2000);
    }

    else if (BUTTON_DOWN & pressedButtons) {
      Serial.println("Down button pressed");
      currentDevice --;
      if (currentDevice < 0) {
        currentDevice = MAXDEVICES - 1;  // wrap around to the last device
      }
      displayDevice(devices[currentDevice]);
      delay(2000);
    }

    else if (BUTTON_SELECT & pressedButtons) {
      Serial.println("Select button pressed");
      displayF228295();
      delay(2000);
    }
    else{

    }
  }
}


//------------------------------------------------------------------------------------------------------------

void loop() {
  ButtonsPress();
  
  Serial.println("There are currently " + String(numDevices) + "/" + MAXDEVICES + " devices");
  delay(2000);
  input();
  delay(2000);
  Serial.println("---------------------------");
}





//SEPARATE DUMMY SKETCH THAT WORKS (BUTTONS)
/*
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
unsigned long lastChecked;

struct Device {
  String id;
  char type;
  String location;
  String state;
  int power = 0;
};

const int MAXDEVICES = 4;
Device devices[MAXDEVICES]; 
int currentDevice = 0;

void setup() {  
  Serial.begin(9600);
  lcd.begin(16, 2);
  lastChecked = millis();

  devices[0].id= "AAA";
  devices[0].type= 'S';
  devices[0].location= "LivingRoom";
  devices[0].state= "ON";
  devices[0].power= 10;

  devices[1].id= "BBB";
  devices[1].type= 'L';
  devices[1].location= "Kitchen";
  devices[1].state= "OFF";
  devices[1].power= 20;

  devices[2].id= "CCC";
  devices[2].type= 'O';
  devices[2].location= "Garden";
  devices[2].state= "ON";
  devices[2].power= 30;

  devices[3].id= "DDD";
  devices[3].type= 'T';
  devices[3].location= "Toilet";
  devices[3].state= "OFF";
  devices[3].power= 40;
}

void loop() {
    ButtonsPress();
}

void ButtonsPress(){
    uint8_t pressedButtons = lcd.readButtons();
  
  if ((lastChecked + 1000) < millis()) {
    lastChecked = millis();
    lcd.clear();
    
    if (BUTTON_UP & pressedButtons) {
      currentDevice ++;
      if (currentDevice >= MAXDEVICES) {
        currentDevice = 0;  // wrap around to the first device
      }
      displayDevice();
      delay(2000);
    }

    if (BUTTON_DOWN & pressedButtons) {
      currentDevice --;
      if (currentDevice < 0) {
        currentDevice = MAXDEVICES - 1;  // wrap around to the last device
      }
      displayDevice();
      delay(2000);
    }

    if (BUTTON_SELECT & pressedButtons) {
      displayF228295();
      delay(2000);
    }
  }
}

void displayDevice() {
  lcd.setCursor(0,0);
  lcd.print("^" + devices[currentDevice].id + " " + devices[currentDevice].location);
  lcd.setCursor(0,1);
  lcd.print("v" + String(devices[currentDevice].type) + " " + devices[currentDevice].state + " " + String(devices[currentDevice].power) + "%");
}

void displayF228295(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("F228295");
  lcd.setBacklight(5); //purple
}



*/

















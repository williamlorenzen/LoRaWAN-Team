#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#include "GPS_Air530.h"
#include "GPS_Air530Z.h"  
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"

// note that the select button is being used to turn on/off BLE communication
// note that the toggle button is being used to toggle the display

//************************************************************************* Variables and Defines ************************************************************************************

#define BUTTON_POWER GPIO7
#define EMERGENCY_BUTTON GPIO6
#define SELECT_BUTTON GPIO5
#define TOGGLE_BUTTON USER_KEY
#define LED GPIO9

// defines for the BLE module
#define FACTORYRESET_ENABLE         0
#define NEW_BLE_NAME               "LIAM DEVICE" // change this to whatever you want the name of the BLE module to beacon as
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "DISABLE"

// initializes BLE module
Adafruit_BluefruitLE_UART ble(Serial1, BLUEFRUIT_UART_MODE_PIN); //serial1 for tx2 and rx2

// initializes the OLED display
SSD1306Wire  display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr , freq , SDA, SCL, resolution , rst

// initializes GPS
Air530ZClass GPS;

// array of strings containing the messages to be displayed on the OLED upon a particular downlink message being received from TTN server
static const char* downlink_mess[] = {
  "",
  "Return within Bounds",
  "Return to Base",  
  "Move to Field A", 
  "Move to Field B",
  "Rain Expected",
  "High Heat Alert",
  "Return Device to Base"
};

// array of strings containing the messages to be displayed on the OLED upon a particular string being received on the BLE module
// these strings correspond to the message code that will be sent to the server in an uplink
static const char* uplink_mess[] = {
  "",
  "Task complete", //complete
  "Starting Task", //start
  "Need a hand", //hand
  "Repair Needed", //repair
  "Pest Sighting", //pest
  "Equipment Malfunction", //malf
  "Deliveries Arrived" //deliv
};

//================================================= Arrays used to send fake GPS data to server for testing and demonstration purposes ==================================================================
// this mock GPS data will only be sent to server if "debug = 1"

//Device 1
/*
int lat_debug_iterable[] = {542124, 542105, 542577, 543149, 543639, 543873, 544345, 545019, 545861, 545805, 545777, 545750, 546249, 547163, 547801, 548291};
int long_debug_iterable[] = {586105, 587607, 588347, 588004, 586373, 587232, 588237, 586316, 586424, 587561, 588752, 590780, 591810, 590704, 588548, 586391};
int debug_array_size = 16;
*/

//Device 2
/*
int lat_debug_iterable[] = {542109, 542257, 542377, 543061, 543440, 543597, 543699, 544050, 544909, 545574, 545787, 545796, 545556, 545353, 545020, 544576};
int long_debug_iterable[] = {594707, 595565, 596671, 597250, 597647, 598612, 599900, 600715, 600715, 600093, 599256, 598387, 597293, 596456, 595887, 595308};
int debug_array_size = 16;
*/

//Device 3
///*
int lat_debug_iterable[] = {549455, 549973, 550675, 551312, 551765, 551894, 551950, 551922, 551913, 551922, 551996, 552033, 551663, 551294, 551118, 550776, 550481, 550585, 550356, 550088, 550015};
int long_debug_iterable[] = {600577, 600609, 600469, 600287, 600062, 599364, 598603, 597723, 597133, 596467, 595824, 595201, 595159, 595094, 595094, 595094, 595631, 596278, 596913, 597782, 598447};
int debug_array_size = 21;
//*/

//========================================================================================================================================================================================================

int debug = 0; // controls whether real GPS data is sent to server, or mock data (0 for real, 1 for mock)
int debug_iterator = 0;

uint32_t downlink_data = 0;

uint8_t switch_flag = 0;
uint8_t emergency_flag = 0;
uint8_t emergency_button_counter = 0;
bool emergency_button_ready = false;
uint16_t select_flag = 0;
uint8_t toggle_flag = 0;
uint8_t up_message_flag = 0;
uint8_t down_message_flag = 0;

uint32_t starting;
uint8_t run_once = 0;
uint8_t ble_run_once = 0;
uint8_t ble_disp_run_once = 0;

uint32_t emergency_timer;
uint32_t timer2;

int batteryVoltage = 0;
uint16_t battery_Voltage = 0;

int32_t lng_int_part = 0;
int32_t lng_tmp;
uint32_t lng_frac_part = 0;

int32_t lat_int_part = 0;
int32_t lat_tmp;
uint32_t lat_frac_part = 0;

extern TimerEvent_t TxNextPacketTimer; // had to add this so that the timer that controls LoRaWAN wake up from low power mode can be turned off when the program enters the DEVICE_STATE_IDLE switch case
// turning off this timer ensures that the program will stay in DEVICE_STATE_IDLE until the toggle button is pressed (rather than exiting this switch case upon the timer overflowing)

//======================================= The OTAA parameters that have been specified for each of the three devices in TheThingsNetwork ===============================================================
// swapping OTAA parameters makes TTN think that data is being sent from a different device

/* OTAA para for Liam*/
///*
uint8_t devEui[] = { 0x70, 0xb3, 0xd5, 0x7e, 0xd0, 0x06, 0x53, 0x89 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0xcc, 0xb4, 0x5e, 0xba, 0xfb, 0xd7, 0x60, 0x07, 0x9c, 0x0d, 0xf0, 0x51, 0x17, 0x30, 0x29, 0x4e };
//*/

/* OTAA para for Justin*/
/*
uint8_t devEui[] = { 0x70, 0xb3, 0xd5, 0x7e, 0xd0, 0x06, 0x55, 0xE1 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x3f, 0xc8, 0x0d, 0x40, 0x41, 0xdb, 0xe9, 0xb2, 0x5b, 0x9c, 0xc7, 0xaf, 0xd1, 0xd4, 0x66, 0xd6 };
*/

/* OTAA para for Maria*/
/*
uint8_t devEui[] = { 0x70, 0xb3, 0xd5, 0x7e, 0xd0, 0x06, 0x54, 0xCB };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0xF5, 0x56, 0x03, 0xED, 0x90, 0x74, 0x5A, 0x3C, 0xB2, 0xFA, 0xA4, 0x36, 0x1B, 0x5F, 0xFA, 0x4C };
*/

//========================================================================================================================================================================================================

// ABP para -> these are irrelevant because we use OTAA as it is more secure
uint8_t nwkSKey[] = { 0x6E, 0xD6, 0xC2, 0xBC, 0x0B, 0xAE, 0x0B, 0x03, 0x0C, 0x9B, 0x19, 0xBC, 0xF5, 0x96, 0xD3,0x24 };
uint8_t appSKey[] = { 0x5E, 0x3B, 0x4F, 0xB3, 0x33, 0xB7, 0x6F, 0xF3, 0x57, 0x11, 0x0D, 0x6C, 0x3F, 0x9A, 0x0F,0x93 };
uint32_t devAddr =  ( uint32_t )0x260c83F6;

/*LoraWan channelsmask, default channels 0-7*/ 
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;

/*LoraWan Class, Class A and Class C are supported*/
DeviceClass_t  loraWanClass = LORAWAN_CLASS;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 15000; // right now it is set to 15s, meaning that every ~15s, an uplink message will be sent to the server

/*OTAA or ABP*/
bool overTheAirActivation = LORAWAN_NETMODE;

/*ADR enable*/
bool loraWanAdr = LORAWAN_ADR;

/* set LORAWAN_Net_Reserve ON, the node could save the network info to flash, when node reset not need to join again */
bool keepNet = LORAWAN_NET_RESERVE;

/* Indicates if the node is sending confirmed or unconfirmed messages */
bool isTxConfirmed = LORAWAN_UPLINKMODE;

/* Application port */
uint8_t appPort = 2;
/*!
* Note, that if NbTrials is set to 1 or 2, the MAC will not decrease
* the datarate, in case the LoRaMAC layer did not receive an acknowledgment
*/
uint8_t confirmedNbTrials = 4;

//**************************************************************************************************************************************************************************************************

//*********************************************************************************** Functions *************************************************************************************************

// A small helper for BLE code
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function used for converting GPS data
int32_t fracPart(double val, int n) 
{
  return (uint32_t)abs((val - (int32_t)(val))*pow(10,n));
}

// turns OLED display ON
void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

// turns OLED display off
void VextOFF(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

// interrupt service routine for when emergency button (red button) is held down
void EMERGENCYButtonPress(void) {
  // had to turn off interrupts and make a large debouncing delay on this button's ISR because for some reason this GPIO reads a falling edge whenever the board is booting up
  // which incorrectly triggers the ISR --> this makes it so that the Emergency button has to be held down to activate the ISR, which protects against an emergency signal accidentally being sent
  noInterrupts();
  delay(800);
  if (emergency_button_ready == true) {

      // if button has been held down once, displays "Emergency Signal Sent" on OLED and the emergency flag is changed, which will then be sent in the next uplink message
      if (emergency_button_counter == 1 && digitalRead(EMERGENCY_BUTTON) == 0) {
          Serial.print("Emergency button has been pressed twice");
          Serial.println();
          emergency_button_counter = 2;
          emergency_flag = 0xFF;
          Display_Emergency_Press(emergency_button_counter);
      }
      // if button has been held down once, displays "Send Emergency Signal" on OLED
      if (digitalRead(EMERGENCY_BUTTON) == 0 && emergency_button_counter == 0) {
        Serial.print("Emergency button has been pressed once");
        Serial.println();
        emergency_button_counter = 1;
        Display_Emergency_Press(emergency_button_counter);
        }

        // variable initialized with current time, which is used to determine how much time has elapsed since the the emergency button has been pressed
        emergency_timer = millis();
  }

    delay(1000);
    interrupts();
}

// interrupt service routine for when select button (blue button) is pressed
void SELECTButtonPress(void) {
  delay(20);
  // if select button is pressed, the BLE is turned on/off and corresponding message on OLED is displayed
  if (digitalRead(SELECT_BUTTON) == 0) {
    Serial.print("Select button has been pressed");
    Serial.println();
    select_flag = select_flag + 1;
    if (select_flag%2 == 1) {
      Serial.print("Turning on BLE");
      Serial.println();
      Display_BLE_Status(select_flag);
      deviceState = DEVICE_STATE_IDLE;
    }
    else {
      Serial.print("Turning off BLE");
      Serial.println();
      Display_BLE_Status(select_flag);
      delay(1000);
      GPS.begin(9600); // GPS is turned on after BLE communication is stopped because the GPS has to be turned off when
      // BLE communication starts, as the BLE and GPS share Serial1 (RX2 and TX2) on the Heltec
      ble_run_once = 0;
      deviceState = DEVICE_STATE_INIT;
    }
}

}

// interrupt service routine for when toggle button (green button) is pressed
void TOGGLEButtonPress(void) {
  delay(20);
  // if toggle button is pressed, the OLED is turned on/off
  // when OLED is turned on, the current longitude, latitude, and battery voltage (mV) of the end node are displayed
  if (digitalRead(TOGGLE_BUTTON) == 0) {
    Serial.print("Toggle button has been pressed");
    Serial.println();
    toggle_flag = toggle_flag + 1;
    if (toggle_flag%2 == 1) {
      Serial.print("Turning on display");
      Serial.println();
      Display_Crit_Info();
    }
    else {
      Serial.print("Turning off display");
      Serial.println();
      display.clear();
      display.display();

      VextOFF();
    }
}
}

// displays on OLED whether the BLE is being turned on or off based on the value of input argument
void Display_BLE_Status(uint16_t select_flag) {

  VextON(); // turns on OLED 
  delay(100);

  display.clear(); // clears OLED display
  display.display(); // makes the OLED screen empty

  // formatting on OLED
  display.setTextAlignment(TEXT_ALIGN_CENTER); 
  display.setFont(ArialMT_Plain_10);

  if (select_flag%2 == 1) {
      char str[30];
      sprintf(str,"Turning");
      display.drawString(64, 14, str);
      sprintf(str,"ON");
      display.drawString(64, 28, str);
      sprintf(str,"BluetoothLE");
      display.drawString(64, 42, str);
  }

  if (select_flag%2 == 0) {
      char str[30];
      sprintf(str,"Turning");
      display.drawString(64, 14, str);
      sprintf(str,"OFF");
      display.drawString(64, 28, str);
      sprintf(str,"BluetoothLE");
      display.drawString(64, 42, str);
  }
   display.display(); // displays on OLED what was input to .drawString()

   delay(2000);

   display.clear();
   display.display();

   VextOFF();
}

// displays on OLED information about emergency message based on the value of input argument
void Display_Emergency_Press(uint8_t emergency_button_counter) {

  VextON();
  delay(100);

  display.clear();
  display.display();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);

  if (emergency_button_counter == 1) {
      char str[30];
      sprintf(str,"Send");
      display.drawString(64, 14, str);
      sprintf(str,"Emergency");
      display.drawString(64, 28, str);
      sprintf(str,"Signal?");
      display.drawString(64, 42, str);
  }

  if (emergency_button_counter == 2) {
      char str[30];
      sprintf(str,"Emergency");
      display.drawString(64, 14, str);
      sprintf(str,"Signal");
      display.drawString(64, 28, str);
      sprintf(str,"Sent");
      display.drawString(64, 42, str);
  }
  
  display.display();
}

// displays on OLED the current latitude, longitude, and battery voltage (mV) of the end node
void Display_Crit_Info() {

  VextON();
  delay(100);

  display.clear();
  display.display();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);

  char str[30];
  sprintf(str,"lat : %d.%d",lat_int_part,lat_frac_part);
  display.drawString(64, 14, str);
  sprintf(str,"lon: -%d.%d",lng_int_part,lng_frac_part);
  display.drawString(64, 28, str);
  sprintf(str,"volt: %d mV",battery_Voltage);
  display.drawString(64, 42, str);

  display.display();

}

// initializes all of the Heltec's GPIOs and configures all interrupts and their corresponding ISRs
void GPIOInits(void) {

  pinMode(EMERGENCY_BUTTON,INPUT);
  attachInterrupt(EMERGENCY_BUTTON,EMERGENCYButtonPress,FALLING);

  pinMode(SELECT_BUTTON,INPUT);
  attachInterrupt(SELECT_BUTTON,SELECTButtonPress,FALLING);

  pinMode(TOGGLE_BUTTON,INPUT);
  attachInterrupt(TOGGLE_BUTTON,TOGGLEButtonPress,FALLING);

  pinMode(BUTTON_POWER,OUTPUT);
  digitalWrite(BUTTON_POWER, HIGH);

  pinMode(ADC2, INPUT);

  pinMode(LED,OUTPUT);
  digitalWrite(LED, HIGH);
}

// 
uint16_t getBatVolt(void) {

  // setting GPIO8 to an output and setting it low so that it acts as GND for ADC (see schematic)
  pinMode(GPIO8,OUTPUT);
  digitalWrite(GPIO8, LOW);

  int temp = 0;

  for(int i=0;i<50;i++) { //read 50 times and get average
        temp+=analogReadmV(ADC2);
        }
	batteryVoltage = (2.25)*(temp / 50); // 2.25 corresponds to the resistors connected to ADC ((18k total)/(8k)), to map voltage read across 8k resistor to the voltage of the battery

  // setting GPIO8 to an input so that is high impedance and there is less power disspated across resistors connected to ADC2
  pinMode(GPIO8,INPUT);

  return batteryVoltage;
}

// initializes the BLE module
void InitializeBLE(void) {

  Serial.println(F("Adafruit Bluefruit Command Mode Example"));
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!
  
  Serial.println(F("Change BLE name to " NEW_BLE_NAME));
  ble.sendCommandCheckOK("AT+GAPDEVNAME=" NEW_BLE_NAME);

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("******************************"));
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    Serial.println(F("******************************"));
  }

}

// displays on the OLED the time it took for the GPS to set
void Display_GPS_Set(uint32_t time_elapsed, int user_delay) {

  VextON();
  delay(100);

  display.clear();
  display.display();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);

  char str[30];
  sprintf(str,"GPS HAS");
  display.drawString(64, 6, str);
  sprintf(str,"SET IN");
  display.drawString(64, 20, str);
  sprintf(str, "%u", time_elapsed);
  display.drawString(64, 34, str);
  sprintf(str,"SECONDS");
  display.drawString(64, 48, str);

  display.display();

  delay(user_delay);

  display.clear();
  display.display();

  VextOFF();

}

// displays a message on the OLED based on the code that is received from a downlink from the TTN server
void Display_Downlink_Message(uint32_t downlink_data_code, int user_delay) {
  VextON();
  delay(100);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);

  char str[30];
  sprintf(str, downlink_mess[downlink_data_code]);
  display.drawString(64, 34, str);

  display.display();

  delay(user_delay);

  display.clear();
  display.display();

  VextOFF();
}

// displays a message on the OLED based on the string that is read by the BLE module and that has been sent by the BluefruitConnect app
void Display_Uplink_Message(uint32_t uplink_data_code) {

  display.clear();
  display.display();

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);

  char str[30];
  sprintf(str, "Going to send:");
  display.drawString(64, 20, str);
  sprintf(str, uplink_mess[uplink_data_code]);
  display.drawString(64, 34, str);

  display.display();

  ble_disp_run_once = 1;

}

/* Prepares the payload of the frame using data collected from end node */
static void prepareTxFrame( uint8_t port )
{
  if (debug == 0) { // reads the true GPS data of the end node
    lng_int_part = (int32_t)GPS.location.lng();
    lng_tmp=lng_int_part;
    lng_int_part=abs(lng_int_part);
    lng_frac_part = fracPart(GPS.location.lng(),6);
    
    lat_int_part = (int32_t)GPS.location.lat();
    lat_tmp=lat_int_part;
    lat_int_part = abs(lat_int_part);
    lat_frac_part = fracPart(GPS.location.lat(),6);
  }

  if (debug == 1) { // reads mock GPS data from preconfigured arrays
    lng_int_part = -84;
    lng_tmp=lng_int_part;
    lng_int_part=abs(lng_int_part);
    lng_frac_part=long_debug_iterable[debug_iterator];
    
    lat_int_part = 30;
    lat_tmp=lat_int_part;
    lat_int_part = abs(lat_int_part);
    lat_frac_part=lat_debug_iterable[debug_iterator];

    if (debug_iterator<debug_array_size-1){
      debug_iterator++;
    }

  }
  
    battery_Voltage = getBatVolt(); // getting battery voltage

    appDataSize = 14; // 14 bytes are sent in each uplink message
    appData[0] = (uint8_t)(lng_frac_part & 0xFF); // first byte scheduled is first byte of fraction part of longitude
    appData[1] = (uint8_t)((lng_frac_part>>8) & 0xFF); // second byte scheduled is second byte of fraction part of longitude
    appData[2] = (uint8_t)((lng_frac_part>>16) & 0xFF); // third byte scheduled is third byte of fraction part of longitude
    appData[3] = (uint8_t)(lng_int_part & 0xFF); // fourth byte scheduled is integer part of longitude
    // fifth byte scheduled is a flag that indicates if the longitude is negative
    if (lng_tmp < 0) {
      appData[4] = 0xFF; 
    }
    else {
      appData[4] = 0x00;
    }
   
    appData[5] = (uint8_t)(lat_frac_part & 0xFF); // sixth byte scheduled is first byte of fraction part of latitude
    appData[6] = (uint8_t)((lat_frac_part>>8) & 0xFF); // seventh byte scheduled is second byte of fraction part of latitude
    appData[7] = (uint8_t)((lat_frac_part>>16) & 0xFF); // eighth byte scheduled is third byte of fraction part of latitude
    appData[8] = (uint8_t)(lat_int_part & 0xFF); // ninth byte scheduled is integer part of latitude
    // tenth byte scheduled is a flag that indicates if the latitude is negative
    if (lat_tmp < 0) {
      appData[9] = 0xFF;
    }
    else {
      appData[9] = 0x00;
    }

    // eleventh byte scheduled is emergency flag
    appData[10] = emergency_flag;

    appData[11]  = (uint8_t)(battery_Voltage & 0xFF); // twelfth byte scheduled is first byte of battery voltage (mV)
    appData[12] = (uint8_t)((battery_Voltage>>8) & 0xFF); // thirteenth byte scheduled is second byte of battery voltage (mV)
    appData[13] = up_message_flag; // fourteenth byte is message code corresponding to the string sent from BluefruitConnect app and read by BLE module

}

// handles downlinks (messages sent from TTN to the end node)
void downLinkDataHandle(McpsIndication_t *mcpsIndication) // refer to http://community.heltec.cn/t/how-to-call-function-that-processes-downlink-message/11009/8 for reference
{
  Serial.printf("\n\t downLinkDataHandle: ACK Data received\n");
  Serial.printf("\n\t+REV DATA:%s,\n\tRXSIZE %d,\n\tPORT %d, RSSI: %d,\n\t SNR: %d,\n\t DATA_RATE:%d,\r\n",
           mcpsIndication->RxSlot ? "RXWIN2" : "RXWIN1", mcpsIndication->BufferSize, mcpsIndication->Port,
           mcpsIndication->Rssi, (int)mcpsIndication->Snr, (int)mcpsIndication->RxDoneDatarate);

  if (mcpsIndication->RxData == true) {
    
    Serial.print("+REV DATA:");
    for(uint8_t i=0;i<mcpsIndication->BufferSize;i++) {

      Serial.printf("%02X",mcpsIndication->Buffer[i]);
      }
    Serial.println();

    downlink_data = mcpsIndication->Buffer[0] << 8 | mcpsIndication->Buffer[1] << 0;
    Serial.print("Final downlink data is:");
    Serial.print(downlink_data);
    Serial.println();

    if (downlink_data == 4657) { //corresponds to 0x1231
      down_message_flag = 1; // if 0x1231 received, downlink message flag is updated to ensure that "Return within Bounds" is displayed on OLED
    }
    if (downlink_data == 4658) { //corresponds to 0x1232
      down_message_flag = 2; // if 0x1232 received, downlink message flag is updated to ensure that "Return to Base" is displayed on OLED
    }
    if (downlink_data == 4659) { //corresponds to 0x1233
      down_message_flag = 3; // if 0x1233 received, downlink message flag is updated to ensure that "Move to Field A" is displayed on OLED
    }
    if (downlink_data == 4660) { //corresponds to 0x1234
      down_message_flag = 4; // if 0x1234 received, downlink message flag is updated to ensure that "Move to Field B" is displayed on OLED
    }
    if (downlink_data == 4661) { //corresponds to 0x1235
      down_message_flag = 5; // if 0x1235 received, downlink message flag is updated to ensure that "Rain Expected" is displayed on OLED
    }
    if (downlink_data == 4662) { //corresponds to 0x1236
      down_message_flag = 6; // if 0x1236 received, downlink message flag is updated to ensure that "High Heat Alert" is displayed on OLED
    }
    if (downlink_data == 4663) { //corresponds to 0x1237
      down_message_flag = 7; // if 0x1237 received, downlink message flag is updated to ensure that "Return Device to Base" is displayed on OLED
    }
  }
}

//**********************************************************************************************************************************************************************************

//******************************************************************************* Setup *******************************************************************************************

void setup() {

	Serial.begin(115200);

  GPIOInits(); // initializing all GPIOs and any interrupts and ISRs

  display.init(); // initializing OLED display

  InitializeBLE(); // initializing BLE module
  delay(100);
  GPS.begin(9600); // turning on GPS and initializing it; it has 9600 baud rate

  timer2 = millis(); // timer used to track how much time elapses between the program starting and the GPS setting

  emergency_button_ready = true; // enables the Emergency Button ISR's operation (protection against false falling edge reads upon device bootup)

#if(AT_SUPPORT)
	enableAt();
#endif
  // transitioning program to switch block that initializes LoRaWAN
	deviceState = DEVICE_STATE_INIT; 
}

//**********************************************************************************************************************************************************************************

//**************************************************************************** Main Loop ******************************************************************************************

void loop()
{
  
  if (emergency_button_counter != 0) {
    if ((uint32_t)round((millis()-emergency_timer)/1000) > 10) { // if more than 10 seconds has elapsed since the Emegency button has been held down
    // then the OLED display is cleared so that the emergency status messages disappear; emergency_button_counter is set to 0 after 10 seconds as well
    // meaning that if the emergency button was held down once and not held down again within the 10 second window, then the emergency button will
    // once again have to be held down twice in order to send an emergency signal
      Serial.print("Emergency timed out, loser");
      Serial.println();
      emergency_button_counter = 0;
      display.clear();
      display.display();

      VextOFF();
    }
  }

	switch( deviceState )
	{
      case DEVICE_STATE_IDLE:  // switch block for BLE communication only
      {

        if (ble_run_once == 0) {
          TimerStop( &TxNextPacketTimer ); // stop the LoRaWAN timer so that this switch block is not exited prematurely

          VextON();
          delay(100);

          GPS.end(); // turning off GPS because the BLE and GPS share Serial1 
          delay(1000);

          InitializeBLE(); // initializing BLE once again
          ble_run_once = 1;
        }
        // Check for incoming characters from Bluefruit
          ble.println("AT+BLEUARTRX");
          ble.readline();

          if (ble.buffer[0]=='c' && ble.buffer[1]=='o' && ble.buffer[2]=='m' && ble.buffer[3]=='p' && ble.buffer[4]=='l' && ble.buffer[5]=='e' && ble.buffer[6]=='t' && ble.buffer[7]=='e') {
            Serial.print("Task complete"); // if "complete" is received by BLE, uplink message flag is sent to 1, corresponding to the message "Task complete"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 1;
          }
          if (ble.buffer[0]=='s' && ble.buffer[1]=='t' && ble.buffer[2]=='a' && ble.buffer[3]=='r' && ble.buffer[4]=='t') {
            Serial.print("Starting Task"); // if "start" is received by BLE, uplink message flag is sent to 2, corresponding to the message "Starting Task"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 2;
          }
          if (ble.buffer[0]=='h' && ble.buffer[1]=='a' && ble.buffer[2]=='n' && ble.buffer[3]=='d') {
            Serial.print("Need a hand"); // if "hand" is received by BLE, uplink message flag is sent to 3, corresponding to the message "Need a hand"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 3;
          }
          if (ble.buffer[0]=='r' && ble.buffer[1]=='e' && ble.buffer[2]=='p' && ble.buffer[3]=='a' && ble.buffer[4]=='i' && ble.buffer[5]=='r') {
            Serial.print("Repair Needed"); // if "repair" is received by BLE, uplink message flag is sent to 4, corresponding to the message "Repair Needed"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 4;
          }
          if (ble.buffer[0]=='p' && ble.buffer[1]=='e' && ble.buffer[2]=='s' && ble.buffer[3]=='t') {
            Serial.print("Pest Sighting"); // if "pest" is received by BLE, uplink message flag is sent to 5, corresponding to the message "Pest Sighting"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 5;
          }
          if (ble.buffer[0]=='m' && ble.buffer[1]=='a' && ble.buffer[2]=='l' && ble.buffer[3]=='f') {
            Serial.print("Equipment Malfunction"); // if "malf" is received by BLE, uplink message flag is sent to 6, corresponding to the message "Equipment Malfunction"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 6;
          }
          if (ble.buffer[0]=='d' && ble.buffer[1]=='e' && ble.buffer[2]=='l' && ble.buffer[3]=='i' && ble.buffer[4]=='v') {
            Serial.print("Deliveries Arrived"); // if "deliv" is received by BLE, uplink message flag is sent to 7, corresponding to the message "Deliveries Arrived"
            Serial.println();
            ble_disp_run_once = 0;
            up_message_flag = 7;
          }

          if (ble_disp_run_once == 0) {
            Display_Uplink_Message(up_message_flag); // displays message corresponding to uplink message flag on the OLED
          }

          if (ble.buffer[0]=='s' && ble.buffer[1]=='e' && ble.buffer[2]=='n' && ble.buffer[3]=='d') { // if "send" is received by BLE, the OLED display is turned off, the GPS is turned 
          // on and reinitialized, and this BLE switch block is exited so that LoRaWAN communication can once again be established
            select_flag = select_flag + 1; 
            Serial.print("Turning off BLE");
            Serial.println();

            display.clear();
            display.display();

            VextOFF();

            delay(1000);
            GPS.begin(9600);
            ble_run_once = 0;

            deviceState = DEVICE_STATE_INIT;
          }
            break;

    }
		case DEVICE_STATE_INIT: // initializes the LoRaWAN module using init(), which sets LoRaWAN parameters, region, abd the LoRaWAN timer ISR
		{
#if(LORAWAN_DEVEUI_AUTO)
			LoRaWAN.generateDeveuiByChipID();
#endif
#if(AT_SUPPORT)
			getDevParam();
#endif
			printDevParam();
			LoRaWAN.init(loraWanClass,loraWanRegion);
			deviceState = DEVICE_STATE_JOIN;
			break;
		}
		case DEVICE_STATE_JOIN: // joins the server and chooses appropriate next switch block based on parameters
		{
			LoRaWAN.join();
			break;
		}
		case DEVICE_STATE_SEND: // performs intermediate processing, prepares payload of frame, and sends uplink message to server
		{
      // reads data from GPS and decodes it
      starting = millis();
      while( (millis()-starting) < 1000 ) {
         while (GPS.available() > 0) {
          GPS.encode(GPS.read());
          }
          }

      if(( (lng_frac_part != 0) && run_once == 0)){ // when the GPS sets, the time that it took for the GPS to set is displayed on the OLED

        uint32_t time_elapsed = (uint32_t)round((millis()-timer2)/1000);

        Display_GPS_Set(time_elapsed, 10000); 

        run_once = 1;
      }

      if(down_message_flag != 0){ // if a downlink message has been received from TTN and it matches one of the preconfigured downlink codes, the message corresponding
      // to that downlink code is displayed on the OLED for 10 seconds
        Display_Downlink_Message(down_message_flag, 10000);
        down_message_flag = 0;
      }

			prepareTxFrame( appPort ); // prepares frame to be transmitted in uplink message
			LoRaWAN.send(); // sends the frame to the server
			deviceState = DEVICE_STATE_CYCLE; // moves to switch block that schedules next packet transmission
			break;
		}
		case DEVICE_STATE_CYCLE: // sets the value for the LoRaWAN timer and starts the timer, which will ensure that the Heltec exits low power mode to send another uplink message
    // after "appTxDutyCycle" seconds
		{
			// Schedule next packet transmission
			txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND ); // timer set to overflow after "appTxDutyCycle + small random delay" seconds 
			LoRaWAN.cycle(txDutyCycleTime);
			deviceState = DEVICE_STATE_SLEEP; // movs to switch block that places Heltec in low power mode
			break;
		}
		case DEVICE_STATE_SLEEP:
		{
			LoRaWAN.sleep(); // places Heltec in low power mode
			break;
		}
		default:
		{
			deviceState = DEVICE_STATE_INIT;
			break;
		}
	}
}

//****************************************************************************************************************************************************************************************

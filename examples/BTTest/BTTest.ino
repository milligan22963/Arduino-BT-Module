 /*
  * This sketch is designed to establish a connection over Bluetooth to transfer
  * data between a host system and a target microcontroller.
  */
  
#include <stdio.h>            // sprintf
#include <EEPROM.h>           // storage of connection data
#include <SoftwareSerial.h>   // communication between BT module and arduino
#include <btmodule.h>         // arduino library for communicating with the SeeedStudio BT module

// Status pin for onboard led
#define BT_CONNECTED_STATUS_PIN  13

// The BT shield can use two digital pins for the serial communications from D0 - D7
#define BT_COMM_SPEED 38400
#define BT_PIN_CODE 1234
#define BT_RX_PIN 6
#define BT_TX_PIN 7

#define BT_CONNECTED_PIN 1 // analog pin

// EEPROM SIGNATURE
const long signature = 0xACED; // some non-arbritary value

#define MAX_BT_ADDRESS 32

/* Structures used in this sketch */
typedef struct
{
  long signature;                  // data is only valid when 0xACED is stored here.
  int valid;                       // 0 if not valid, non-zero if valid
  byte bt_address[MAX_BT_ADDRESS]; // bluetooth address of previously discovered device(s)
} EE_Structure;

// State machine for bluetooth communications
enum StateMachine
{
  Initialize = 0, // if we store the device addr, we can jump to connecting
  RestoreDeviceAddress,
  DiscoveringDevice,
  DeviceDiscovered, // once discovered, we can reconnect without all of the setup
  ConnectingToDevice,
  ActiveCommunications,
  ErrorState,
  EndStates
};

/*
 * Method declarations
 */
int ReadInteger(int offset);
void WriteInteger(int offset, int value);
long ReadLong(int offset);
void WriteLong(int offset, long value);
void ReadBytes(int offset, byte *data);
void WriteBytes(int offset, byte *data);

/*
 * ReadInteger - a helper function to read in an integer from the nonvolatile storage
 *
 * Params - offset - the offset into the EEPROM where data is stored
 *
 * Returns - int - the value read from the specified location
 */
int ReadInteger(int offset)
{
  int returnValue = 0;
  
  // Read in EEPROM
  for (int count = 0; count < sizeof(int); count++)
  {
    returnValue |= EEPROM.read(offset + count) << (count * 8); // read in a byte from eeprom and shift it x to build up an integer of data
  }
  
  return returnValue;
}

/*
 * WriteInteger - a helper function to write in an integer to the nonvolatile storage
 *
 * Params - offset - the offset into the EEPROM where data is to be stored
 *        - value - the value to be stored
 *
 * Returns - none
 */
void WriteInteger(int offset, int value)
{
  byte dataValue = 0;
  byte intSize = sizeof(int);
  
  for (int count = 0; count < intSize; count++)
  {
    dataValue = value >> ((intSize - 1 - count) * 8); // We need to shift from left to right so upper first then on down
    EEPROM.write(offset + count, dataValue);
  }
}

/*
 * ReadLong - a helper function to read in a long from the nonvolatile storage
 *
 * Params - offset - the offset into the EEPROM where data is stored
 *
 * Returns - long - the value read from the specified location
 */
long ReadLong(int offset)
{
  long value = 0;
  
  int storedValue = ReadInteger(offset);
  
  value = (storedValue << 16);
  
  storedValue = ReadInteger(offset + 2);
  
  value += storedValue;
  
  return value;
}

/*
 * WriteInteger - a helper function to write in a long to the nonvolatile storage
 *
 * Params - offset - the offset into the EEPROM where data is to be stored
 *        - value - the value to be stored
 *
 * Returns - none
 */
void WriteLong(int offset, long value)
{
  int upperValue = (value >> 16);
  int lowerValue = value && 0xFFFF;
  
  WriteInteger(offset, upperValue);
  WriteInteger(offset + 2, lowerValue);
}

/*
 * ReadBytes - a helper function to read in a number of bytes from the nonvolatile storage
 *
 * Params - offset - the offset into the EEPROM where data is stored
 *        - data - the array to hold the data being read in
 *        - maxLength - the maximum length to read
 *
 * Returns - none
 */
void ReadBytes(int offset, byte *data, int maxLength)
{
  if (data != NULL)
  {
    for (int index = 0; index < maxLength; index++)
    {
      data[index] = EEPROM.read(offset + index);
    }
  }
}

/*
 * WriteBytes - a helper function to write out a number of bytes to the nonvolatile storage
 *
 * Params - offset - the offset into the EEPROM where data is to be stored
 *        - data - the array of bytes to be stored
 *        - length - the length of the data to write
 * Returns - none
 */
void WriteBytes(int offset, byte *data, int length)
{
  if (data != NULL)
  {
    for (int index = 0; index < length; index++)
    {
      EEPROM.write(offset + index, data[index]);
    }
  }
}

/*
 * Variables used by this sketch
 */
volatile EE_Structure StoredData;
BTModule *g_pBTModule = NULL; // Our interface to the Bluetooth module
StateMachine g_connectionState = Initialize;

/*
 * Setup - main configuration point of our sketch to configure
 *         the Arduino for setting up the bluetooth module
 *         and restoring the connection data if present
 *
 * Params - none
 *
 * Returns - nothing
 */
void setup()
{
  Serial.begin(9600); // for monitoring

  Serial.println("Starting setup...\n");
  
  g_pBTModule = BTModule::GetInstance(BT_RX_PIN, BT_TX_PIN);
  
  pinMode(BT_CONNECTED_STATUS_PIN, OUTPUT);
  
  StoredData.signature = ReadLong(0); 
    
  // Have we been here before?
  if (StoredData.signature == signature)
  {
    StoredData.valid = ReadInteger((char *)&StoredData.valid - (char *)&StoredData.signature);
    
    if (StoredData.valid != 0)
    {
      ReadBytes((char *)&StoredData.bt_address[0] - (char *)&StoredData.signature, (byte *)&StoredData.bt_address[0], MAX_BT_ADDRESS);
    }
  }
  else
  {
    // Store signature and default valid as 0 to indicate the address isn't valid
    WriteLong(0, signature);
    WriteInteger((char *)&StoredData.valid - (char *)&StoredData.signature, 0); // default to 0
  }

  // If we are still connected then jump right in
  if (analogRead(BT_CONNECTED_PIN) != 0)
  {
    g_connectionState = ActiveCommunications;
  }

  Serial.println("Setup Completed.");
}

void loop()
{
  char *pRemoteAddress = NULL;
  
  switch (g_connectionState)
  {
    case Initialize:
    {
      Serial.println("Initializing device...");
      digitalWrite(BT_CONNECTED_STATUS_PIN, LOW); // turn off when not connected
  
      g_pBTModule->begin(BT_COMM_SPEED, true, "ArduinoBT", BT_PIN_CODE); // do we need to block on this?
      
      g_connectionState = RestoreDeviceAddress;
    }
    break;
    
    case RestoreDeviceAddress:
    {
      if (StoredData.valid != 0)
      {
        pRemoteAddress = (char *)&StoredData.bt_address[0];
        g_connectionState = DeviceDiscovered;
      }
      else
      {
        pRemoteAddress = NULL;
        g_connectionState = DiscoveringDevice;
      }
    }
    break;
    
    case DiscoveringDevice:
    {
      Serial.println("Discovering devices...");
 
      // discover our device, do not block, we will come back to try again
      if (g_pBTModule->discoverDevice(false) == true)
      {
        g_connectionState = DeviceDiscovered;
      }
    }
    break;
    
    case DeviceDiscovered:
    {
      Serial.println("Connecting to device...");
      if (g_pBTModule->connectToDevice(pRemoteAddress, true) == true)
      {
        g_connectionState = ActiveCommunications;
        digitalWrite(BT_CONNECTED_STATUS_PIN, HIGH); // we have a connection

        // Update eeprom data
        if (StoredData.valid == 0)
        {
            WriteInteger((char *)&StoredData.valid - (char *)&StoredData.signature, 1); // it is now valid
            g_pBTModule->getRemoteAddress((char *)&StoredData.bt_address[0]);
            WriteBytes((char *)&StoredData.bt_address[0] - (char *)&StoredData.signature, (byte *)&StoredData.bt_address[0], MAX_BT_ADDRESS);
        }  
      }
    }
    break;
            
    case ActiveCommunications:
    {
      char dataValue = 0;
      
      if (Serial.available())
      {
        g_pBTModule->sendByte(Serial.read());
      }
      
      if (g_pBTModule->readByte(&dataValue) == true)
      {
        Serial.print(dataValue);
      }
    }
    break;
    
    case ErrorState:
    {
      // Reset and come back up
      g_connectionState = DeviceDiscovered;
    }
    break;
  }
}


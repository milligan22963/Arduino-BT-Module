
#include <stdio.h>
#include <Arduino.h>
#include "btmodule.h"

// BT commands
char *ENABLE_BT_MASTER = "\r\n+STWMOD=1\r\n";
char *ENABLE_BT_SLAVE = "\r\n+STWMOD=0\r\n";
char *SET_BT_BAUD_RATE_9600 = "\r\n+STBD=9600\r\n";
char *SET_BT_BAUD_RATE_19200 = "\r\n+STBD=19200\r\n";
char *SET_BT_BAUD_RATE_38400 = "\r\n+STBD=38400\r\n";
char *SET_BT_BAUD_RATE_57600 = "\r\n+STBD=57600\r\n";
char *SET_BT_BAUD_RATE_115200 = "\r\n+STBD=115200\r\n";
char *SET_BT_BAUD_RATE_230400 = "\r\n+STBD=230400\r\n";
char *SET_BT_BAUD_RATE_460800 = "\r\n+STBD=460800\r\n";
char *SET_BT_NAME = "\r\n+STNA=%s\r\n"; // use sprintf to setname
char *SET_BT_AUTO_CONNECT_OFF = "\r\n+STAUTO=0\r\n";
char *SET_BT_AUTO_CONNECT_ON = "\r\n+STAUTO=1\r\n";
char *SET_PERMIT_PAIRED_CONNECT_OFF = "\r\n+STOAUT=0\r\n";
char *SET_PERMIT_PAIRED_CONNECT_ON = "\r\n+STOAUT=1\r\n";
char *SET_BT_PIN_CODE = "\r\n+STPIN=%04d\r\n"; // use sprintf to set pin number
char *CLEAR_BT_PIN_CODE = "\r\n+DLPIN\r\n";
char *READ_BT_LOCAL_ADDRESS = "\r\n+RTADDR\r\n";
char *SET_PERMIT_RECONNECT_OFF = "r\n+LOSSRECONN=0\r\n";
char *SET_PERMIT_RECONNECT_ON = "r\n+LOSSRECONN=1\r\n";
char *SET_BT_STOP_INQUIRE = "\r\n+INQ=0\r\n";
char *SET_BT_START_INQUIRE = "\r\n+INQ=1\r\n";
char *CONNECT_BT_DEVICE = "\r\n+CONN=%s\r\n"; // in the format aa,bb,cc,dd,ee,ff
char *INQUIRE_BT_RESPONSE = "\r\n+RTINQ="; //%02x,%02x,%02x,%02x,%02x,%02x;%s\r\n";
char *CLIENT_BT_REQUEST_PINCODE = "\r\n+INPIN\r\n";
char *ANSWER_CLIENT_BT_PINCODE_REQUEST = "\r\n+RTPIN=%04d\r\n"; // sprintf to set pin number
char *BT_END_TOKEN = "\r\n";
char *BT_ECHO_ON = "\r\n+STECHO=1\r\n";
char *BT_ECHO_OFF = "\r\n+STECHO=0\r\n";

char *BT_CONNECT_RESPONSE = "CONNECT:"; // followed by OK or fail
char *BT_CONNECT_RESPONSE_OK = "CONNECT:OK"; // followed by OK or fail
char *BT_CONNECT_RESPONSE_FAIL = "CONNECT:FAIL"; // followed by OK or fail

// 0 - Initializing
// 1 - Ready
// 2 - Inquiring
// 3 - Connecting
// 4 - Connected
char *BT_MODULE_STATUS_RESPONSE = "+BTSTATE:"; // response to last command

BTModule *BTModule::sm_pInstance = NULL;

BTModule *BTModule::GetInstance(int rxPin, int txPin)
{
  if (sm_pInstance == NULL)
  {
      sm_pInstance = (BTModule *)malloc(sizeof(BTModule));
      sm_pInstance->Initialize(rxPin, txPin);
  }
  
  return sm_pInstance;
}

void BTModule::ReleaseInstance()
{
	if (sm_pInstance != NULL)
	{
		free(sm_pInstance);
		sm_pInstance = NULL;
	}
}
	
void BTModule::Initialize(int rxPin, int txPin)
{
    memset(m_buffer, 0, sizeof(m_buffer));
    memset(m_remoteDeviceAddress, 0, sizeof(m_remoteDeviceAddress));
    m_bufferIndex = 0;
    m_isMaster = false;

	m_pBTModule = new SoftwareSerial(rxPin, txPin);
	
	// configure communication pins
	pinMode(rxPin, INPUT);
	pinMode(txPin, OUTPUT);
}

// http://www.seeedstudio.com/wiki/Serial_port_bluetooth_module_(Master/Slave)#Commands_to_change_default_settings
void BTModule::begin(long baudRate, bool isMaster, char *pName, int pinCode)
{
    char *pCommands[2];
    char buffer[64];
		
	memset(pCommands, 0, sizeof(pCommands)); // clear everything
		
	m_pBTModule->begin(baudRate);

	m_isMaster = isMaster;

    if (isMaster == true)
    {
    	pCommands[0] = ENABLE_BT_MASTER;
    }
    else
    {
    	pCommands[0] = ENABLE_BT_SLAVE;
    }
    SendBTCommands(pCommands);
    
    sprintf(buffer, SET_BT_NAME, pName);
    pCommands[0] = buffer; // set friendly name  
    SendBTCommands(pCommands);

    sprintf(buffer, SET_BT_PIN_CODE, pinCode);  
    pCommands[0] = buffer;
    SendBTCommands(pCommands);
}

void BTModule::autoConnect(bool allowed)
{
    char *pCommands[2];

	memset(pCommands, 0, sizeof(pCommands)); // clear everything
	
	if (allowed == true)
	{
		pCommands[0] = SET_BT_AUTO_CONNECT_ON;
	}
	else
	{
		pCommands[0] = SET_BT_AUTO_CONNECT_OFF;
	}
    SendBTCommands(pCommands);
}

void BTModule::pairedConnect(bool allowed)
{
    char *pCommands[2];

	memset(pCommands, 0, sizeof(pCommands)); // clear everything
	
	if (allowed == true)
	{
		pCommands[0] = SET_PERMIT_PAIRED_CONNECT_ON;
	}
	else
	{
		pCommands[0] = SET_PERMIT_PAIRED_CONNECT_OFF;
	}
    SendBTCommands(pCommands);
}

void BTModule::reconnect(bool allowed)
{
    char *pCommands[2];

	memset(pCommands, 0, sizeof(pCommands)); // clear everything
	
	if (allowed == true)
	{
		pCommands[0] = SET_PERMIT_RECONNECT_ON;
	}
	else
	{
		pCommands[0] = SET_PERMIT_RECONNECT_OFF;
	}
    SendBTCommands(pCommands);
}

bool BTModule::discoverDevice(bool block)
{
  bool foundDevice = false;
  char *pCommands[2];

  memset(pCommands, 0, sizeof(pCommands)); // clear everything

  // start inquire
  pCommands[0] = SET_BT_START_INQUIRE;
  
  SendBTCommands(pCommands);
  
  int attempts = 0;
  m_bufferIndex = 0;
  while (attempts < 100)
  {
    if (m_pBTModule->available())
    {
      m_buffer[m_bufferIndex++] = m_pBTModule->read();
      m_buffer[m_bufferIndex] = 0; // terminate what we have

      // Inquiry response?
      char *pResponseStart = strstr(m_buffer, INQUIRE_BT_RESPONSE);
      if (pResponseStart != NULL)
      {
        // look for the end token
        if (strstr(pResponseStart, BT_END_TOKEN) != NULL)
        {
          char *pSemiColon = strchr(pResponseStart, ';');
          
          if (pSemiColon != NULL)
          {
          	pResponseStart += strlen(INQUIRE_BT_RESPONSE);  // move past the response
          	
            int addressSize = pSemiColon - pResponseStart;
            strncpy(m_remoteDeviceAddress, pResponseStart, addressSize);
            m_remoteDeviceAddress[addressSize] = 0;

			Serial.print("Remote device addr: ");
			Serial.println(m_remoteDeviceAddress);            
            foundDevice = true;

            /* We can stop inquring now */
            pCommands[0] = SET_BT_STOP_INQUIRE;
            
            SendBTCommands(pCommands);              
            break;
          }
          else
          {
          	// shouldnt happen but lets not blow our buffer(s)
            if (m_bufferIndex > 127)
            {
              m_bufferIndex = 0;
            }
          }
        }
      }
    }
    else
    {
      if (block == false)
      {
      	attempts++;
      }
      delay(500); // wait a little
    }
  }
  
  return foundDevice;
}

bool BTModule::connectToDevice(char *pDeviceName, bool block) // if no device name, it will use m_remoteDeviceAddress
{
  	bool success = false;
  	char localBuffer[64];
	
	if (pDeviceName == NULL)
	{
		pDeviceName = m_remoteDeviceAddress;
	}
	else
	{
		memcpy(m_remoteDeviceAddress, pDeviceName, sizeof(m_remoteDeviceAddress));
	}
	
	sprintf(localBuffer, CONNECT_BT_DEVICE, pDeviceName);

	m_pBTModule->print(localBuffer);

	m_bufferIndex = 0;
	int attempts = 0;
	while ((success == false) && (attempts < 50))
	{
    	if (m_pBTModule->available())
		{
			m_buffer[m_bufferIndex++] = m_pBTModule->read();
			m_buffer[m_bufferIndex] = 0; // terminate what we have

			if (strstr(m_buffer, BT_CONNECT_RESPONSE_OK) != NULL)
			{
	        	success = true;
	        	break;
			}
			else if (strstr(m_buffer, BT_CONNECT_RESPONSE_FAIL) != NULL)
			{
	        	m_pBTModule->print(localBuffer);
				m_bufferIndex = 0;
			}
	    }
	    else
	    {
	    	if (block == false)
	    	{
		    	attempts++;
	    	}
			delay(250); // wait a little
	    }    
	}
	m_pBTModule->flush();
  
	return success;
}

bool BTModule::getRemoteAddress(char *pAddress)
{
	bool success = false;
	
    if ((m_remoteDeviceAddress[0] != 0) && (pAddress != NULL))
    {
    	memcpy(pAddress, m_remoteDeviceAddress, sizeof(m_remoteDeviceAddress));
    }
    
    return success;
}

void BTModule::setRemoteAddress(char *pAddress)
{
	if (pAddress != NULL)
	{
		memcpy(m_remoteDeviceAddress, pAddress, sizeof(m_remoteDeviceAddress));
	}
}

bool BTModule::moduleReady(int statusWanted, bool block)
{
  bool success = false;
  
  int attempts = 0;
  m_bufferIndex = 0;
  
  while (attempts < 50)
  {
    if (m_pBTModule->available())
    {
      m_buffer[m_bufferIndex++] = m_pBTModule->read();
      m_buffer[m_bufferIndex] = 0; // terminate what we have
      
      char *pResponse = strstr(m_buffer, BT_MODULE_STATUS_RESPONSE);
      if (pResponse != NULL)
      {
        int statusValue = atoi(&pResponse[strlen(BT_MODULE_STATUS_RESPONSE)]);
        
        if (statusValue == statusWanted)
        {
        	Serial.println("Good status.");
          success = true;
          break;
        }
        else
        {
          Serial.println("Bad status.");
          m_bufferIndex = 0;
          break;
        }
      }
    }
    else
    {
    	if (block == false)
    	{
			attempts++;
    	}
		delay(250);
    }
  }

  return success;
}

bool BTModule::sendSerialData(char *pData)
{
	bool success = false;
	
	if (pData != NULL)
	{
		Serial.print("Sending command: ");
		Serial.println(pData);
		m_pBTModule->flush();
		m_pBTModule->print(pData);
		success = true;
	}
	
	return success;
}

bool BTModule::getSerialData(char *pData, char termCharacter)
{
	bool success = false;

	if (pData != NULL)
	{
		int bufferIndex = 0;
		
		while (success == false)
		{
			if (m_pBTModule->available())
			{
				char nextChar = m_pBTModule->read();
			    pData[bufferIndex++] = nextChar;
			    pData[bufferIndex] = 0; // terminate what we have
			    
			    if (nextChar == termCharacter)
			    {
			    	success = true;
			    	break;
			    }
			    if ((strstr(pData, "ERROR") != NULL) ||
			        (strstr(pData, "LINK LOSS") != NULL))
			    {
			        break;
			    }
			}
			else
			{
				delay(50);
			}
		}
	}
	return success;
}

void BTModule::ReadBuffer(bool printOut)
{
	m_bufferIndex = 0;
	while (m_pBTModule->available())
	{
		m_buffer[m_bufferIndex++] = m_pBTModule->read();
		m_buffer[m_bufferIndex] = 0;
	}
	
	if ((m_bufferIndex > 0) && (printOut == true))
	{
		Serial.println(m_buffer);
	}
}

void BTModule::SendBTCommands(char **pCommands)
{
  if (pCommands != NULL)
  {
    int commandIndex = 0;
    char *pCommand = pCommands[commandIndex];
    while (pCommand != NULL)
    {
      m_pBTModule->print(pCommand);
      
      // Move to the next command if any
      pCommand = pCommands[++commandIndex];
    }
	m_pBTModule->flush();
    delay(BT_DELAY);
  }
}

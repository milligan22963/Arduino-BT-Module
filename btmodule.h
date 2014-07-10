/*
 *  BT Module to interface with the seeedstudio Bluetooth product
 *  http://www.seeedstudio.com/depot/Bluetooth-Shield-p-866.html?cPath=19_21
 *
 */
 
#ifndef _H_BT_MODULE
#define _H_BT_MODULE

#include <SoftwareSerial.h>

#define BT_DELAY 2000

 // BT commands
extern char *ENABLE_BT_MASTER;
extern char *ENABLE_BT_SLAVE;
extern char *SET_BT_BAUD_RATE_9600;
extern char *SET_BT_BAUD_RATE_19200;
extern char *SET_BT_BAUD_RATE_38400;
extern char *SET_BT_BAUD_RATE_57600;
extern char *SET_BT_BAUD_RATE_115200;
extern char *SET_BT_BAUD_RATE_230400;
extern char *SET_BT_BAUD_RATE_460800;
extern char *SET_BT_NAME; // use sprintf to setname
extern char *SET_BT_AUTO_CONNECT_OFF;
extern char *SET_BT_AUTO_CONNECT_ON;
extern char *SET_PERMIT_PAIRED_CONNECT_OFF;
extern char *SET_PERMIT_PAIRED_CONNECT_ON;
extern char *SET_BT_PIN_CODE; // use sprintf to set pin number
extern char *CLEAR_BT_PIN_CODE;
extern char *READ_BT_LOCAL_ADDRESS;
extern char *SET_PERMIT_RECONNECT_OFF;
extern char *SET_PERMIT_RECONNECT_ON;
extern char *SET_BT_STOP_INQUIRE;
extern char *SET_BT_START_INQUIRE;
extern char *CONNECT_BT_DEVICE; // in the format aa,bb,cc,dd,ee,ff
extern char *INQUIRE_BT_RESPONSE; //%02x,%02x,%02x,%02x,%02x,%02x;%s\r\n";
extern char *CLIENT_BT_REQUEST_PINCODE;
extern char *ANSWER_CLIENT_BT_PINCODE_REQUEST; // sprintf to set pin number
extern char *BT_END_TOKEN;
extern char *BT_ECHO_ON;
extern char *BT_ECHO_OFF;

extern char *BT_CONNECT_RESPONSE; // followed by OK or fail
extern char *BT_CONNECT_RESPONSE_OK; // followed by OK or fail
extern char *BT_CONNECT_RESPONSE_FAIL; // followed by OK or fail

// 0 - Initializing
// 1 - Ready
// 2 - Inquiring
// 3 - Connecting
// 4 - Connected
extern char *BT_MODULE_STATUS_RESPONSE; // response to last command

class BTModule
{
	public:
		static BTModule *GetInstance(int rxPin, int txPin);
		static void ReleaseInstance();
	    
	    void begin(long baudRate, bool isMaster, char *pName, int pinCode);
	    void autoConnect(bool allowed);
	    void pairedConnect(bool allowed);
	    void reconnect(bool allowed);
	    
	    bool discoverDevice(bool block);
	    bool connectToDevice(char *pDeviceName = NULL, bool block = true); // if no device name, it will use m_remoteDeviceAddress
		bool sendByte(char dataValue);
		bool readByte(char *dataValue);

	    bool sendSerialData(char *pData);
	    bool getSerialData(char *pData, char termCharacter);
	    
	    bool getRemoteAddress(char *pAddress);
	    void setRemoteAddress(char *pAddress);
		bool moduleReady(int statusWanted, bool block);
	    
	private:
	    void Initialize(int rxPin, int txPin);
	    void SendBTCommands(char **pCommands);
	    void ReadBuffer(bool printOut);
	    
	private:
	    static BTModule *sm_pInstance;
	    char m_buffer[128];
		char m_remoteDeviceAddress[64]; // address of remote device
	    int  m_bufferIndex;
	    bool m_isMaster;
	    SoftwareSerial *m_pBTModule;
};

#endif // _H_BT_MODULE
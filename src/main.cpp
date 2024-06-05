#include <Arduino.h>
#include <mcp_can.h>
#include <SPI.h>

#include <LittleFS.h>
#include <FS.h>

#define ARDUINO_LOOP_STACK_SIZE (16 * 1024)
#define CAN0_INT 4 // Set INT to pin 4

byte byData = 0;

File file;

MCP_CAN CAN0(5);

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[16];
char msgString[128];
String *arr = new String[9];
bool bWaitAck = false;

unsigned int wCurrentID = 0;
byte byCurrentLen = 0;
byte byCurrentData[8];

void splitString(String str, char separator)
{

  int iStrStart = 0;
  for (int i = 0; i < 9; i++)
  {
    int separatorIndex = str.indexOf(separator, iStrStart);
    arr[i] = str.substring(iStrStart, separatorIndex);
    iStrStart = separatorIndex + 1;
  }
  wCurrentID = strtoul(arr[4].c_str(), NULL, 16);
  arr[7].replace("0x", "");
  byCurrentLen = (byte)arr[7].toInt();
  arr[8].replace("0x", "");
  arr[8].replace(" ", "");
  // Serial.println(str);

  for (unsigned int i = 0; i < arr[8].length(); i += 2)
  {
    byCurrentData[i / 2] = strtoul(arr[8].substring(i, i + 2).c_str(), NULL, 16);
  }
}
byte *hexToBytes(String str)
{
  str.replace("0x", "");
  str.replace(" ", "");
  // Serial.println(str);
  byte *data = new byte[str.length() / 2];
  for (unsigned int i = 0; i < str.length(); i += 2)
  {
    data[i / 2] = strtoul(str.substring(i, i + 2).c_str(), NULL, 16);
    // Serial.println(str.substring(i, i + 2).c_str());
  }
  return data;
}

String str;

void setup()
{
  Serial.begin(115200);
  pinMode(CAN0_INT, INPUT);
  // delay(1000);
  Serial.println("GM MC1 2024");
  if (!LittleFS.begin())
  {
    Serial.println("LITTLEFS Mount Failed");
    return;
  }
  // file = LittleFS.open("/canblue.txt", "r");

  if (CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");
  CAN0.setMode(MCP_NORMAL);
}

void loop()
{
  if (Serial.available() > 0)
  {
    byData = Serial.read();
    Serial.println(byData);
    if (byData == 98)
    {
      file.close();
      file = LittleFS.open("/canblue.txt", "r");
    }
    if (byData == 103)
    {
      file.close();
      file = LittleFS.open("/cangreen.txt", "r");
    }
    if (byData == 114)
    {
      file.close();
      file = LittleFS.open("/canred.txt", "r");
    }
  }

  if(file.available() == 0){
    Serial.print("=") ;
  }
  while (file.available())
  {
  
    str = file.readStringUntil(0x0a);
    if (str.indexOf("Send") != -1 && str.indexOf("Success") != -1)
    {
      if (bWaitAck == false)
      {
        str.replace("\t\t", "\t");

        splitString(str, 0x09);

        unsigned int wID = wCurrentID;

        byte byLen = byCurrentLen;

        byte sndStat = CAN0.sendMsgBuf(wID, 0, byLen, byCurrentData);
        // Serial.printf("Sending Message ID:%d Len:%d data:%s, ", wID, byLen, data);
        if (sndStat == CAN_OK)
        {
          // Serial.println("Message Sent Successfully!");
          Serial.print(".");
        }
        else
        {
          // Serial.println("Error Sending Message...");
          Serial.print("-");
        }
        if (wID == 0x46A)
        {
          bWaitAck = true;
          // Serial.println("Wait for Acknowledge");
          Serial.print("?");
        }
        // delay(1); // send data per 100ms
      }
    }
    if (bWaitAck == true)
    {
      if (!digitalRead(CAN0_INT))
      { // If CAN0_INT pin is low, read receive buffer

        CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
        bWaitAck = false;
        /*

                if ((rxId & 0x80000000) == 0x80000000) // Determine if ID is standard (11 bits) or extended (29 bits)
                  sprintf(msgString, "Extended ID: 0x%.8lX  DLC: %1d  Data:", (rxId & 0x1FFFFFFF), len);
                else
                  sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);

                //  Serial.print(msgString);

                if ((rxId & 0x40000000) == 0x40000000)
                { // Determine if message is a remote request frame.
                  sprintf(msgString, " REMOTE REQUEST FRAME");
                  Serial.print(msgString);
                }
                else
                {
                  for (byte i = 0; i < len; i++)
                  {
                    sprintf(msgString, " 0x%.2X", rxBuf[i]);
                    //  Serial.print(msgString);
                    Serial.print("!");
                  }
                  if (rxId == 0x2cc)
                  {
                    bWaitAck = false;
                  }
                }

                Serial.println();*/
      }
    }
  }
  // file.close();
}

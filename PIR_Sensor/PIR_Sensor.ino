//#define DEBUG
//#define DEBUG_WIFI

#define AP_SSID "iotemb1"
#define AP_PASS "iotemb10"
#define SERVER_NAME "192.168.1.78"
#define SERVER_PORT 5000
#define LOGID "PSW_WIFI"
#define PASSWD "PASSWD"

#define WIFITX 7  //7:TX -->ESP8266 RX
#define WIFIRX 6 //6:RX-->ESP8266 TX
#define LEDB 3
#define LEDG 11
#define LEDR 5
#define SENSOR_PIN 2
#define CDS_PIN A0
#define CMD_SIZE 60
#define ARR_CNT 5

#include "WiFiEsp.h"
#include "SoftwareSerial.h"
#include <TimerOne.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

char sendBuf[CMD_SIZE];

bool timerIsrFlag = false;
unsigned int secCount;
char sendId[10] = "PSW_SMP";
char recvId[10] = "PSW_BTM";
char SQL[10] = "PSW_SQL";

SoftwareSerial wifiSerial(WIFIRX, WIFITX);
WiFiEspClient client;

char lcdLine1[17] = "Smart IoT By KSH";
char lcdLine2[17] = "WiFi Connecting!";
boolean sensorState = LOW;
boolean lastsensorState = LOW;
boolean ledState = true;
boolean sensorFlag = true;
boolean colorFlag = true;
boolean AlarmFlag = false;
bool cdsFlag = true;
bool FirstcdsFlag = false;
bool randomFlag = true;
int n;
int R;
int G;
int B;
int R1;
int G1;
int BB1;
int AlarmH, AlarmM;
int TimeH = 20, TimeM = 43;
int cds = 0;

void setup() {
  // put your setup code here, to run once:
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);
  Serial.begin(115200); //DEBUG
  Serial.print("sendid :");
  Serial.print(sendId);
  wifi_Setup();

  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerIsr); // timerIsr to run every 1 seconds
  pinMode(LEDR,OUTPUT);
  pinMode(LEDG,OUTPUT);
  pinMode(LEDB,OUTPUT);
}

void loop() {
  if (client.available()) {
    socketEvent();
  }
  if (timerIsrFlag)
  {
    timerIsrFlag = false;
    if(secCount == 60)
    {
        secCount = 0;
        TimeM++;
        if(TimeM == 60)
        {
            TimeM = 0;
            TimeH++;
            if(TimeH == 24)
            {
                TimeH = 0;  
            }  
        }
    }
  
//cds
    cds = map(analogRead(CDS_PIN), 0, 1023, 0, 100);

    if(FirstcdsFlag)
    {    
        
        if ((cds >= 50) && cdsFlag)
        {
          cdsFlag = false;
          ledState = true;
          sprintf(sendBuf, "[%s]Good@%s\n", recvId, cdsFlag ? "Night" : "Morning"); 
          client.write(sendBuf, strlen(sendBuf));
        }
        else if ((cds < 50) && !cdsFlag)
         {
          cdsFlag = true;
          ledState = false;
          sprintf(sendBuf, "[%s]Good@%s\n", recvId, cdsFlag ? "Night" : "Morning");
          client.write(sendBuf, strlen(sendBuf));
        }

        if(!(secCount % 5 ))
        {
            sprintf(sendBuf, "[%s]SENSOR@%d\n", SQL, cds);
            client.write(sendBuf, strlen(sendBuf));  
        }
    }

      if(!colorFlag)
    {
      if(randomFlag)
      {
        R = random(1,255);
        G = random(1,255);
        B = random(1,255);
      }
      else
      {
         R = R1;
         G = G1;
         B = BB1;  
      }
    }
    else
    {
      R = 255;
      G = 255;
      B = 255;  
    }

    
    if (!(secCount % 5))
    {
      if (!client.connected()) {
        //lcdDisplay(0, 1, "Server Down");
        server_Connect();
      }
    }
    //sensorState
    if (sensorFlag)
    {
      sensorState = digitalRead(SENSOR_PIN);

      if (sensorState != lastsensorState) {
        if (sensorState == HIGH) {
          ledState = true;
          lastsensorState = !lastsensorState;
        }
        else if (sensorState == LOW)
        {
          n++;
          if (!(n % 10))
          {
            ledState = false;
            n = 0;
            lastsensorState = !lastsensorState;

          }
        }
      }
    }
    if(ledState)
  {
     sprintf(lcdLine2, "LED ON\0 ");
     RGBcolor(R,G,B);  
  } 
  else
  {
      sprintf(lcdLine2, "LED OFF\0 ");
      RGBcolor(0,0,0);  
  }
    if(AlarmFlag)
    {
      sprintf(lcdLine2, "Set time %0d : %0d \0", AlarmH, AlarmM);
    if(TimeH == AlarmH && TimeM == AlarmM) 
    {
        ledState = true;
        if(!(secCount))
        {
          AlarmFlag = false;
          sprintf(lcdLine2, "LED ON\0 ");
        }  
    }
     
  }
 }

  
  
  
 lcdDisplay(0, 0, lcdLine1);
 lcdDisplay(0, 1, lcdLine2);
 
}

void socketEvent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  sendBuf[0] = '\0';
  len = client.readBytesUntil('\n', recvBuf, CMD_SIZE);
  client.flush();

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  
  //LED ON,OFF
  if (!strcmp(pArray[1], "LED")) {
    if (!strcmp(pArray[2], "ON")) {
      ledState = true;
    }
    else if (!strcmp(pArray[2], "OFF")) {
      ledState = false;
    }
    sprintf(sendBuf, "[%s]%s@%s\n", pArray[0], pArray[1], pArray[2]);
  }

  //Sensor ON, OFF
  else if (!strcmp(pArray[1], "SENSOR")) {
    if (!strcmp(pArray[2], "OFF"))
    {
      sensorFlag = false;
      if(!strncmp(pArray[3],"LEDO",4))
      {
          if(!strcmp(pArray[3],"LEDON")) ledState = true;
          else ledState = false;
          sprintf(sendBuf, "[%s]%s %s %s\n", pArray[0], pArray[1], pArray[2], pArray[3]);
      }
    }
    else if (!strcmp(pArray[2], "ON"))
    {
      sensorFlag = true;
      sprintf(sendBuf, "[%s]%s %s\n", pArray[0], pArray[1], pArray[2]);
    }
     sprintf(lcdLine1, "%s%s\0 ", pArray[1], pArray[2]);
  }
//SleepMode
  else if(!strcmp(pArray[1],"SleepMode"))
  {     
    if(!strcmp(pArray[2],"ON"))
    {
        FirstcdsFlag = true;
        sensorFlag = false;   
    }        
    else if(!strcmp(pArray[2],"OFF"))
    {
        FirstcdsFlag = false;
        sensorFlag = true;  
    } 
    sprintf(lcdLine1, "SENSOR %s\0 ", FirstcdsFlag ? "OFF" : "ON");  
    sprintf(sendBuf, "[%s]%s\n", recvId, FirstcdsFlag ? "SleepMode@ON" : "SleepMode@OFF");
    client.write(sendBuf, strlen(sendBuf));
  }

  //colorFlag
  else if (!strcmp(pArray[1], "color"))
  {
    
      if(!strcmp(pArray[2],"random"))
      {
        colorFlag = false;
        randomFlag = true; 
      } 
       
      else if(!strcmp(pArray[2],"basic")) colorFlag = true;
      else
      {
          R1 = atoi(pArray[2]);
          G1 = atoi(pArray[3]);
          BB1 = atoi(pArray[4]);
          colorFlag = false;
          randomFlag = false;
      }  
  }

   //Alarm
  else if(!strcmp(pArray[1], "Alarm"))
  {
    if(!strcmp(pArray[2],"ON"))
    {
    AlarmH = atoi(pArray[3]);
    AlarmM = atoi(pArray[4]);
    AlarmFlag = true;
    sensorFlag = false;
    ledState = false;
    sprintf(lcdLine1, "SENSOR OFF\0");
    }
    else
    {
      AlarmFlag = false;
      sensorFlag = true;
      ledState = true;
      sprintf(lcdLine1, "SENSOR ON\0");
    }
  }

  //~~
  else if (!strncmp(pArray[1], " New", 4)) // New Connected
  {
    Serial.write('\n');
    strcpy(lcdLine2, "Server Connected");
    lcdDisplay(0, 1, lcdLine2);
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {
    Serial.write('\n');
    client.stop();
    server_Connect();
    return ;
  }
  else
    return;

  client.write(sendBuf, strlen(sendBuf));
  client.flush();
}
void timerIsr()
{
  timerIsrFlag = true;
  secCount++;
}

void wifi_Setup() {
  wifiSerial.begin(19200);
  wifi_Init();
  server_Connect();
}
void wifi_Init()
{
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
#ifdef DEBUG_WIFI
      Serial.println("WiFi shield not present");
#endif
    }
    else
      break;
  } while (1);

#ifdef DEBUG_WIFI
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif
  while (WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {
#ifdef DEBUG_WIFI
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);
#endif
  }
  sprintf(lcdLine1, "ID:%s", LOGID);
  lcdDisplay(0, 0, lcdLine1);
  sprintf(lcdLine2, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0, 1, lcdLine2);

#ifdef DEBUG_WIFI
  Serial.println("You're connected to the network");
  printWifiStatus();
#endif
}
int server_Connect()
{

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
    client.print("["LOGID":"PASSWD"]");
  }
  else
  {

  }
}
void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
void lcdDisplay(int x, int y, char * str)
{
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}

void RGBcolor(int r, int b, int g)
{
    analogWrite(LEDR,r);
    analogWrite(LEDG,b);
    analogWrite(LEDB,g);  
}

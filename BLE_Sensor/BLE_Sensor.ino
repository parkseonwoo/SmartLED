#include <SoftwareSerial.h>
#include <Wire.h>
#include <TimerOne.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
//#define DEBUG
#define CDS_PIN A0
#define ARR_CNT 5
#define CMD_SIZE 60
#define SleepButton 7
#define SensorButton 6
#define LedButton 5

char lcdLine1[17] = "SleepMode off";
char lcdLine2[17] = "--------------";
char sendBuf[CMD_SIZE] = {0};
char recvId[10] = "PSW_WIFI";
int LastSleep = HIGH;     // 버튼의 이전 상태 저장
int LastSensor = HIGH;     // 버튼의 이전 상태 저장
int LastLed = HIGH;     // 버튼의 이전 상태 저장
int currentSleep = HIGH;    // 버튼의 현재 상태 저장
int currentSensor = HIGH;    // 버튼의 현재 상태 저장
int currentLed = HIGH;    // 버튼의 현재 상태 저장
bool SleepFlag = false;
bool SensorFlag = false;
bool LedFlag = false;
bool msgFlag = false;
volatile bool timerIsrFlag = false;
volatile bool timer1Flag = false;
unsigned int secCount;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

SoftwareSerial BTSerial(10, 11); // RX ==>BT:TXD, TX ==> BT:RX
//시리얼 모니터에서 9600bps, line ending 없음 설정 후 AT명령 --> OK 리턴
//AT+NAMEiotxx ==> OKname   : 이름 변경 iotxx

void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight(); 
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(0, 1, lcdLine2);
  BTSerial.begin(9600); // set the data rate for the SoftwareSerial port
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerIsr); // timerIsr to run every 1 seconds
  pinMode(SleepButton, INPUT_PULLUP);
  pinMode(SensorButton, INPUT_PULLUP);
  pinMode(LedButton,INPUT_PULLUP);
}

void loop()
{
  if (BTSerial.available())Bluetoothevent();

  
  if (timerIsrFlag)
  {
    timerIsrFlag = false;

  }

//SleepButton
    currentSleep = debounce1(LastSleep);   // 디바운싱된 버튼 상태 읽기
    if (LastSleep == HIGH && currentSleep == LOW)  // 버튼을 누르면...
    {
      SleepFlag = !SleepFlag;
      sprintf(sendBuf,"[%s]SleepMode@%s\n",recvId, SleepFlag ? "ON":"OFF");
      sprintf(lcdLine1,"SleepMode %s", SleepFlag ? "ON" : "OFF");    
      BTSerial.write(sendBuf);
      LastSleep = LOW;
    }
    else if(LastSleep == LOW && currentSleep == HIGH)
    {
       LastSleep = HIGH;  
    }

    //SensorButton
    currentSensor = debounce2(LastSensor);   // 디바운싱된 버튼 상태 읽기
    if (LastSensor == HIGH && currentSensor == LOW)  // 버튼을 누르면...
    {
      SensorFlag = !SensorFlag;
      sprintf(sendBuf,"[%s]SENSOR@%s\n",recvId, SensorFlag ? "ON@LEDON":"OFF");
      BTSerial.write(sendBuf);
      LastSensor = LOW;
    }
    else if (LastSensor == LOW && currentSensor == HIGH)
    {
        LastSensor = HIGH;  
    }

    //LedButton
    currentLed = debounce3(LastLed);   // 디바운싱된 버튼 상태 읽기
    if (LastLed == HIGH && currentLed == LOW)  // 버튼을 누르면...
    {
      LedFlag = !LedFlag;
      sprintf(sendBuf,"[%s]LED@%s\n",recvId, LedFlag ? "ON":"OFF");    
      BTSerial.write(sendBuf);
      LastLed = LOW;
    }
    else if(LastLed == LOW && currentLed == HIGH) LastLed = HIGH;
    
    
   lcdDisplay(0, 0, lcdLine1);
   lcdDisplay(0, 1, lcdLine2);
}

void Bluetoothevent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len;

  len = BTSerial.readBytesUntil('\n', recvBuf, sizeof(recvBuf) - 1);

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }

  //recvBuf : [XXX_ID]LED@ON
  //pArray[0] = "XXX_ID" > 송신자 ID
  //pArray[1] = "LED"
  //pArray[2] = "ON"
  //pArray[3] = 0x0
  
  if(!strcmp(pArray[1],"SleepMode"))
  { 
    if(!strcmp(pArray[2],"OFF")) sprintf(lcdLine2, "--------------\0");          
    sprintf(lcdLine1, "%s %s\0", pArray[1], pArray[2]);
  }

  if(!strcmp(pArray[1],"Good"))
  {
    if(!strcmp(pArray[2],"Morning")) sprintf(lcdLine2, "%s %s\0",pArray[2]);
    else if(!strcmp(pArray[2],"Night")) sprintf(lcdLine2, "%s %s\0",pArray[2]); 
  }
   
 


  else if (!strncmp(pArray[1], " New", 4)) // New Connected
  {
    return ;
  }

  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {
    return ;
  }
}

void timerIsr()
{
  timerIsrFlag = true;
  secCount++;
}

void lcdDisplay(int x, int y, char * str)
{
  int len = 16 - strlen(str);
  lcd.setCursor(x, y);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}

boolean debounce1(boolean last)
{
  boolean current = digitalRead(SleepButton);  // 버튼 상태 읽기
  if (last != current)      // 이전 상태와 현재 상태가 다르면...
  {
    delay(5);         // 5ms 대기
    current = digitalRead(SleepButton);  // 버튼 상태 다시 읽기
  }
  return current;       // 버튼의 현재 상태 반환
}

boolean debounce2(boolean last)
{
  boolean current = digitalRead(SensorButton);  // 버튼 상태 읽기
  if (last != current)    
  // 이전 상태와 현재 상태가 다르면...
  {
    delay(5);         // 5ms 대기
    current = digitalRead(SensorButton);  // 버튼 상태 다시 읽기
  }
  return current;       // 버튼의 현재 상태 반환
}

boolean debounce3(boolean last)
{
  boolean current = digitalRead(LedButton);  // 버튼 상태 읽기
  if (last != current)      // 이전 상태와 현재 상태가 다르면...
  {
    delay(5);         // 5ms 대기
    current = digitalRead(LedButton);  // 버튼 상태 다시 읽기
  }
  return current;       // 버튼의 현재 상태 반환
}

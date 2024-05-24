#include <SoftwareSerial.h>
#include <Wire.h>
#include "DFRobot_BMP280.h"
#define RX 8
#define TX 9
#define SEA_LEVEL_PRESSURE 1013

typedef DFRobot_BMP280_IIC    BMP;    // ******** use abbreviations instead of full names ********

BMP   bmp(&Wire, BMP::eSdoLow);

String AP = "ZTE_2.4G_dESSGg_plus";       // AP NAME
String PASS = "qwerty123"; // AP PASSWORD
String API = "AAFKGXH8KSHOKECS";   // Write API KEY
String HOST = "api.thingspeak.com";
String PORT = "80";
int countTrueCommand;
int countTimeCommand; 
boolean found = false;
unsigned long rpmtime;
float rpmfloat;
unsigned int rpm;
float tempthreshold = 31.00;
bool detected = 0; 
int motoronoff;

SoftwareSerial esp8266(RX,TX); 

void RPM () {
  rpmtime = TCNT1;
  TCNT1 = 0;
  detected = 1;
}

ISR(TIMER1_OVF_vect) {
  detected = 0;
}
 
void printLastOperateStatus(BMP::eStatus_t eStatus)
{
  switch(eStatus) {
  case BMP::eStatusOK:    Serial.println("everything ok"); break;
  case BMP::eStatusErr:   Serial.println("unknow error"); break;
  case BMP::eStatusErrDeviceNotDetected:    Serial.println("device not detected"); break;
  case BMP::eStatusErrParameter:    Serial.println("parameter error"); break;
  default: Serial.println("unknow status"); break;
  }
}
  
void setup() {
  Serial.begin(9600);
  esp8266.begin(115200);

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << CS12); //Prescaler 256
  TIMSK1 |= (1 << TOIE1);
  
  pinMode(2, INPUT);
  pinMode(4, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(2), RPM, FALLING);

  bmp.reset();
  Serial.println("bmp read data test");
  while(bmp.begin() != BMP::eStatusOK) {
    Serial.println("bmp begin faild");
    printLastOperateStatus(bmp.lastOperateStatus);
    delay(2000);
  }
  Serial.println("bmp begin success");
  delay(100);  
  sendCommand("AT",5,"OK");
  sendCommand("AT+CWMODE=1",5,"OK");
  sendCommand("AT+CWJAP=\""+ AP +"\",\""+ PASS +"\"",20,"OK");
}

void loop() {
 float temp = bmp.getTemperature();
 uint32_t    press = bmp.getPressure();
 float altitude = bmp.calAltitude(SEA_LEVEL_PRESSURE, press);

 if(detected == 0){
  rpm = 0;
  }
 else if(detected == 1){
  rpmfloat = 120 / (rpmtime/ 31250.00);
  rpm = round(rpmfloat);
  }

 if(bmp.getTemperature()>= 32){
  digitalWrite(4,LOW);
  }

 else if(bmp.getTemperature()<=32){
  digitalWrite(4,HIGH);
  }

 motoronoff = digitalRead(4);
 
 String getData = "GET /update?api_key="+ API +"&field1" +"="+temp +"&field2" +"="+altitude +"&field3" +"="+rpm +"&field4" +"="+motoronoff;
 sendCommand("AT+CIPMUX=1",5,"OK");
 sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,15,"OK");
 sendCommand("AT+CIPSEND=0," +String(getData.length()+4),4,">");
 esp8266.println(getData);delay(1500);countTrueCommand++;
 sendCommand("AT+CIPCLOSE=0",5,"OK");

 
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }
  
    countTimeCommand++;
  }
  
  if(found == true)
  {
    Serial.println("OYI");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  
  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  }
  
  found = false;
 }

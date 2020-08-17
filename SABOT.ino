/*notes
 VSPI is SCLK = 18, MISO = 19, MOSI = 23, SS = 5
 HSPI SCLK = 14, MISO = 12, MOSI = 13, SS = 15
use SD.begin(uint6_t cs) 
relay works with ACTIVE HIGH INPUT
panel 1 is CP
panel 2 is ucp
*/



#define DEBUG 1
/*-----Libraries-------*/
/***Temperature Sensor*/
#include<OneWire.h>
#include<DallasTemperature.h>
/***SDCARD*/
#include "FS.h"
#include "SD.h"
#include "SPI.h"

/***TIMEUTC*/
#include "time.h"
/***FTPCLIENT*/
#include "Arduino.h"
#include <WiFi.h>
#include <WiFiClient.h> 
#include <ESP32_FTPClient.h>
//Pin declarations
#define CP_CS 34
#define UCP_CS 33 
#define CP_VS 35
#define UCP_VS 25
#define batt_VS 26
#define irr_sense 27
#define relay_1 32
#define relay_3 22
#define relay_4 21
#define relay_5 17
#define relay_6 16 
/*---variables and objects---*/
//general variable declaration
int i; 
unsigned long int timing=0, lastTime =0;
bool flag[] ={0,0,0,0,0,0,0,0,0,0,0,0,0}; 
/*
  flag[0] is for battery voltage sensor
  flag[1] is for voltage sensor of uncleaned panel
  flag[2] is for voltage sensor of cleaned panel
  flag[3] is for current of uncleaned panel
  flag[4] is for current of cleaned panel
  flag[5] is for temperature sensor of the  uncleaned panel
  flag[6] is for temperature sensor of the cleaned panel
  flag[7] is for the temperature sensor of the battery/ambient
  flag[8] is for the irradiation sensor
  flag[9] is for the sd detection
  flag[10] is for the ftp server working
  flag[11] is for wifi connection
  flag[12]  is for time update from ntp server
*/
//Dynamic storage variables
float batt_volt = 0;
float ucp_volt = 0;
float cp_volt = 0;
float current_cp = 0;
float current_ucp = 0; 
float temp_ambient = 0;
float temp_ucp = 0;
float temp_cp = 0;
float irradiation = 0;


//current sensor global variables
float currentValue = 0;
float current = 0;
float currentRef = 2.5; //current Calibration value

//voltage sensor globla variables
float vout = 0,R1 = 56000, R2= 8200 ;
float vin = 0;
//Temperature Sensor
float tempBat=0, tempUcp=0, tempCp=0;
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
DeviceAddress batAddress,ucpAddress,cpAddress;
const int address[]={0x33,0x81,0xF9};

//WIFI CONNECTION
#define WIFI_SSID "Zer0" //SSID (Wifi name)
#define WIFI_PASS "fuckoff!!" //PASSWORD

//GETTING TIME
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;
time_t now;
struct tm timeinfo;
char H[3],M[3],S[3],day[10],mon[10],Y[5];
char UTC[20];
//SDCARD DECLARATIONS
SPIClass spiSD(HSPI);
#define SD_CS 15
#define SCK 14
#define MISO 12
#define MOSI 13

//FTP CLIENT DECLARATION
char ftp_server[] = "inspirece.com";
char ftp_user[]   = "Xenectra";
char ftp_pass[]   = "Xenectra@2807";
char buff[50];
const char *heading = "TIME \t CP_Voltage \t UCP_Voltage \t BATT_Voltage \t CP_Current \t UCP_Current\t Irradiation_sensor\t Temp_ambient\t temp_ucp \t temp_cp \n";
ESP32_FTPClient ftp (ftp_server,ftp_user,ftp_pass, 5000, 2);
/*--------FUNCTIONS-----------*/
//CONNECT TO WIFI FUNCTION
void ConnectWiFi(){
    if(WiFi.status()!=WL_CONNECTED) {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      Serial.println("Connecting WiFi....");
      delay(1000);
      if(WiFi.status()== WL_CONNECTED) {
        flag[11] = 1;
      }else {
        Serial.println("Wifi not connected..");
        flag[11] = 0;
        }
   }else {
      flag[11]=1;
    }
}
   

//Time function
void updateTime() {
  time(&now);
  getLocalTime(&timeinfo);
  snprintf(UTC, sizeof(UTC), "%lu.csv",now);
  strftime(H,3,"%H",&timeinfo);
  strftime(M,3,"%M",&timeinfo);
  strftime(S,3,"%S",&timeinfo);
  strftime(day,10,"%d",&timeinfo);
  strftime(mon,10,"%b",&timeinfo);
  strftime(Y,5,"%Y",&timeinfo);    
  }
//FTP CLIENT FUNCTION
void ftpClient() {
    Serial.print(UTC);
    Serial.println("Hello humans, I am working on ftp, Be patient! v.v");
    if(flag[12]==1) {
     updateTime(); //updates time
    }
    ftp.OpenConnection();
    ftp.ChangeWorkDir("/my_new_dir1");
    ftp.InitFile("Type A");
    snprintf(UTC,sizeof(UTC), "%lu.csv",now);
    Serial.println("Name of the file: ");
    Serial.println(UTC);
    ftp.NewFile(UTC);
    ftp.Write(heading);
    snprintf(buff, sizeof(buff),"\n%s:%s:%s ,%s , %s, %s\t",H,M,S,day,mon,Y);
    ftp.Write(buff);
    //Write data down here after openning the file
    snprintf(buff, sizeof(buff),"%.2f \t %.2f \t %.2f \t", cp_volt, ucp_volt,batt_volt);
    ftp.Write(buff);
    snprintf(buff,sizeof(buff),"%.2f \t%.2f \t%.2f \t", current_cp, current_ucp, irradiation);
    ftp.Write(buff);
    snprintf(buff, sizeof(buff), "%.2f \t%.2f \t%.2f \t",temp_ambient, temp_ucp, temp_cp);  
    ftp.Write(buff);
    
}
//SD card functions READ/WRITE
void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}
void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}
void sdcard() {
  spiSD.begin(SCK, MISO, MOSI, -1);
  SD.begin(SD_CS,spiSD);
  delay(5000);
  if(!SD.begin(SD_CS)){
    Serial.println("Card Mount Failed");
    flag[9] = 0;
  }else {flag[9]= 1;}
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  snprintf(UTC,sizeof(UTC), "/%lu.csv",now);
  writeFile(SD,UTC, heading);
  snprintf(buff, sizeof(buff),"\n%s:%s:%s ,%s , %s, %s\t",H,M,S,day,mon,Y);
  appendFile(SD,UTC, buff);
  snprintf(buff, sizeof(buff),"%.2f \t %.2f \t %.2f \t", cp_volt, ucp_volt,batt_volt);
  appendFile(SD,UTC,buff);
  snprintf(buff,sizeof(buff),"%.2f \t%.2f \t%.2f \t", current_cp, current_ucp, irradiation);
  appendFile(SD,UTC,buff);
  snprintf(buff, sizeof(buff), "%.2f \t%.2f \t%.2f \t",temp_ambient, temp_ucp, temp_cp);  
  appendFile(SD,UTC,buff);  
  
}


//print Temperature device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }

}
void tempCheck(){
    if(sensors.getDeviceCount() != 3){ 
      Serial.println("One or two sensors missing");
    }
    if (!sensors.getAddress(batAddress, 0)) {
      #ifdef DEBUG
        Serial.println("Unable to find address for batt temp");
      #endif
    } else {printAddress(batAddress);}
    if (!sensors.getAddress(ucpAddress, 1)) {
      #ifdef DEBUG
        Serial.println("Unable to find address for ucp temp ");
      #endif
    } else {printAddress(ucpAddress);}
    if (!sensors.getAddress(cpAddress, 2)) {
      #ifdef DEBUG
        Serial.println("Unable to find address for cp temp ");
      #endif
    }else {printAddress(cpAddress);}
    sensors.setResolution(batAddress,9); //we set the precision
    sensors.setResolution(ucpAddress,9);// 9bit or 11 bit or 12 bit
    sensors.setResolution(cpAddress,9);
       
  }
void tempFunc() {
    sensors.requestTemperatures();
    for(int i = 0;i < 3; i++)     //loop through the three different address to
      {                       //find the right one and assign it accordingly. for example
        if(batAddress[7] == address[i]) { //this line checks battery temperature sensor's last address
          tempBat = sensors.getTempC(batAddress); //this address is passed and battery temperature is
          #ifdef DEBUG
             Serial.println("Battery Temperature:"); //if it matches as per the data we gave
             printAddress(batAddress);   //this address is then is for battery temperature
             Serial.printf("\t Temp C: ");    //assigned to the tempBat variable
             Serial.println(tempBat);Serial.println("\n");
           #endif
       }
        if(ucpAddress[7]==address[i]) {//the same is repeated three times. (3 sensors )
          tempUcp = sensors.getTempC(ucpAddress);
          #ifdef DEBUG
            Serial.println("UCP Temperature:");
            printAddress(ucpAddress);
            Serial.printf("\t Temp C: ");
            Serial.println(tempUcp);Serial.println("\n");
          #endif
        }
         if(cpAddress[7]==address[i]) {
          tempCp = sensors.getTempC(cpAddress);
          #ifdef DEBUG
            Serial.println("CP Temperature:");
            printAddress(cpAddress);
            Serial.print("\t tempCp C: ");
            Serial.println(tempCp);Serial.println("\n");
          #endif
       }
  }
}

//Voltage function
float voltageFunction(int GPIO) {
    for (i=0;i<100;i++) {
        vout += (3.3/4095)*analogRead(GPIO);  
      }
      vout/=100;
      return (vout/R2/(R1+R2));
  } 

//current function
float currentFunction(int GPIO){
  
    for (i=0;i<=100;i++){
        currentValue+= (3.3/4095)*analogRead(GPIO);
        delay(5);
      }
    currentValue/=100;
    #ifdef DEBUG
    Serial.println("\ncurrent raw Value");
    Serial.println(currentValue);
  #endif
  if(currentValue >= currentRef-2 && currentValue <= currentRef+2) {  //the value of the current ref,
    currentValue = currentRef;  //is based the power source given to power the current sensor when there is no current flowing throw it
  }
  current = ((currentValue - currentRef )/0.066); //divide the error by sensitivty we get the acutal current
  return current;  
  }


//SETUP START  
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(15000);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting Wifi....");
  delay(5000);
  if(WiFi.status() != WL_CONNECTED) {
       flag[11] = 0;
       Serial.println("Wifi not connected");
  }else{
      flag[11] = 1;
      Serial.println("");
      Serial.print("IP address:");
      Serial.println(WiFi.localIP());
  }
    
  pinMode(CP_CS,INPUT);
  pinMode(UCP_CS, INPUT);
  pinMode(CP_VS, INPUT);
  pinMode(UCP_VS, INPUT);
  pinMode(batt_VS, INPUT);
  pinMode(irr_sense, INPUT);
  pinMode(relay_1, OUTPUT);
  pinMode(relay_3, OUTPUT);
  pinMode(relay_4, OUTPUT);
  pinMode(relay_5, OUTPUT);
  pinMode(relay_6, OUTPUT);
  delay(100);
  sensors.begin();  //Begin communicating with the temperature sensor if any connected
//Time code start
  if(WiFi.status()==WL_CONNECTED) {
    configTime(gmtOffset_sec, daylightOffset_sec,ntpServer);
    Serial.println("Time updated");
    flag[12] = 1;
  }else {
     Serial.println("Unable to update time, no wifi");
     flag[12] = 0; 
    }
  if(flag[12]==1) {  //only upade if local time was configured from ntp server
    updateTime();
   }  
//TIME CODE END 

//SDCARD CODE START
  spiSD.begin(SCK, MISO, MOSI, -1);
  SD.begin(SD_CS,spiSD);
  delay(5000);
  if(!SD.begin(SD_CS)){
    Serial.println("Card Mount Failed");
    flag[9] = 0;
  }else {flag[9]= 1;}
  uint16_t cardType = SD.cardType();
   if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    flag[9] = 0;
   }

  Serial.print("SD Card Type: ");
   if(cardType == CARD_MMC){
      Serial.println("MMC");
   }else if(cardType == CARD_SD){
      Serial.println("SDSC");
   }else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
   } else {
       Serial.println("UNKNOWN");
   }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
//SDCARD CODE END
  tempCheck();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
      
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("I am inside loop function");
  batt_volt = voltageFunction(batt_VS);
  Serial.printf("\n Battery voltage: %.2f",batt_volt);
  ucp_volt = voltageFunction(UCP_VS);
  Serial.printf("\n Panel 2 Voltage: %.2f", ucp_volt);
  cp_volt = voltageFunction(CP_VS);
  Serial.printf("\n Panel 1 voltage: %.2f", cp_volt);
  current_cp = currentFunction(CP_CS);
  Serial.printf("\n Current of panel 1 =: %.2f",current_cp);
  current_ucp = currentFunction(UCP_VS);
  Serial.printf("\n Current of panel 2 =: %.2f", current_ucp);
  irradiation = voltageFunction(irr_sense);
  Serial.printf("\n Irradiation data: %.2f", irradiation);
  ConnectWiFi();
  if(irradiation == 0) {
    flag[8]=0; 
    Serial.println("Irradiation Sensor not found/value Zero");
    }
   tempFunc();
  ConnectWiFi();
  
  if(flag[12]==1) { 
    updateTime();
   }else{
     if(flag[11==1]) {
      configTime(gmtOffset_sec, daylightOffset_sec,ntpServer);
      flag[12]=1;
      Serial.println("Updated time");
     }
    }
  timing = millis();
   if((timing - lastTime)>=1000) {
     lastTime = timing;
     if(flag[12]==1) {
       updateTime();
       snprintf(buff, sizeof(buff),"%s:%s:%s ,%s , %s, %s",H,M,S,day,mon,Y);
       Serial.println(buff);
     }
     //ftpServer(); 
    if(atoi(H)>=11 && atoi(H)<=14) {
      digitalWrite(relay_1, HIGH); //Switch on Heatsink connected to it
      Serial.println("Time to switch on the relay 1 & 2. Actually its on :p");
      delay(100);
    }else {
      digitalWrite(relay_1,LOW);
      Serial.println("Not the right time to switch on the relay 1 & 2 so, lets connect to battery");
      //The connection of hS+ and CC+ are swapped, so have in mind when working. either swap the pins or swap the high/low
    }
    if(atoi(S) >= 0 && atoi (S) <=5) {
        ftpClient();
        delay(1000);
        sdcard();
      } 
        
    }
     
}

  

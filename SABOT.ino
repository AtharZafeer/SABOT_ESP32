#define DEBUG 1
#include<OneWire.h>
#include<DallasTemperature.h>
/*---variables and objects---*/
//general variable declaration
int i;
//current sensor global variables
float currentValue = 0;
float current = 0;
float currentRef = 2.5; //current Calibration value
//voltage sensor globla variables
float vout = 0,R1 = 56000, R2= 8200 ;
//Temperature Sensor
float tempBat=0, tempUcp=0, tempCp=0;
const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
DeviceAddress batAddress,ucpAddress,cpAddress;
const int address[]={0x33,0x81,0xF9};
//
/*--------FUNCTIONS-----------*/


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
    Serial.println("current raw Value");
    Serial.println(currentValue);
  #endif
  if(currentValue >= currentRef-2 && currentValue <= currentRef+2) {  //the value of the current ref,
    currentValue = currentRef;  //is based the power source given to power the current sensor when there is no current flowing throw it
  }
  current = ((currentValue - currentRef )/0.066); //divide the error by sensitivty we get the acutal current
  return current;  
  }
void setup() {
  // put your setup code here, to run once:
pinMode(34, INPUT);
pinMode(33, INPUT);
pinMode(35, INPUT);
pinMode(25, INPUT);
pinMode(26, INPUT);
sensors.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}

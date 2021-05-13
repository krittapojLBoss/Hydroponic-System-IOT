
//include libary ที่ใช้
  #include "DHT.h"
  #include <time.h>
  #include <WiFi.h>
  #include <EEPROM.h>
  #include <OneWire.h>
  #include <DNSServer.h>
  #include <WebServer.h>
  #include <WiFiManager.h>  
  #include <FirebaseESP32.h>
  #include "DFRobot_ESP_PH.h"
  #include <DFRobot_ESP_EC.h>
  #include <DallasTemperature.h>
  #include <LiquidCrystal_I2C.h>
  
//กำหนดค่า macro ต่าง ๆ
  #define BTT 15
  #define DHTPIN 25 
  #define PH_PIN A6
  #define EC_PIN A7
  #define LED_esp 2
  #define RELAY_PIN 4
  #define DHTTYPE DHT21
  #define ONE_WIRE_BUS 13  
  #define Time_interval  5000
  #define Time_interval2 6000
  #define Time_interval_first  2500
  #define FIREBASE_HOST "iothdpfarm-default-rtdb.firebaseio.com"
  #define FIREBASE_AUTH "5Jv6EIaT0vT1l3DjtQiWAq9hlziSqR840NGuQCOP"
  
//กำหนดตัวแปลต่าง ๆ 
  String TimeOf;
  float Humidity,TemperaTure,Ph,Ec;
  char ntp_server1[20] = "ntp.ku.ac.th";
  char ntp_server2[20] = "fw.eng.ku.ac.th";
  char ntp_server3[20] = "time.uni.net.th";
  int Show = 1,Check = 1,Dst = 0,TimeZone = 7,StatusFb,RelayStatus,i=0;
  unsigned long StartPress = 0,TimeSetter = 0,TimeShow = 0,SetInfor = 0;
  
//กำหนด OBJ การใช้งาน
  DFRobot_ESP_EC EC;
  DFRobot_ESP_PH PH;
  FirebaseJson Json;
  DHT Dht(DHTPIN, DHTTYPE);
  FirebaseData FireBaseData;
  OneWire oneWire(ONE_WIRE_BUS);
  LiquidCrystal_I2C LCD(0x27, 16, 2); 
  DallasTemperature sensors(&oneWire);
 
 
void setup() {
  LCD.init();    
  EC.begin(); 
  PH.begin();
  Dht.begin();
  LCD.backlight();
  Serial.begin(115200);
  WiFiManager wifiManager;
  pinMode(LED_esp,OUTPUT); 
  pinMode(RELAY_PIN, OUTPUT); 
  digitalWrite(RELAY_PIN,1);
  LCD.clear();
  LCD.setCursor(3,0);
  LCD.print("AP Name is");
  LCD.setCursor(1,1);
  LCD.print("IOT_Hydroponic");
 
  wifiManager.resetSettings();
  wifiManager.setConfigPortalTimeout(180);
  if (!wifiManager.startConfigPortal("IOT_Hydroponic","P@ssw0rd4268")){LCD.clear();LCD.setCursor(0,0);LCD.println("AP Found problem!");delay(1000);}
  
   LCD.clear();
   LCD.setCursor(0,0);
   LCD.print("connected: ");
   LCD.setCursor(0,1);
   LCD.print(WiFi.localIP());
   
   configTime(TimeZone * 3600, Dst, ntp_server1, ntp_server2, ntp_server3);
    while (!time(nullptr)) {
      Serial.print(".");
      delay(500);
    }
   Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);  
}

void loop() {
  int CurrentState = digitalRead(BTT);
  if (CurrentState == 1)
  {
    long Duration = millis() - StartPress;
    if(Duration > 5000){ 
      if(i<=20){
        Ec = read_EC(ReadTemperature()); digitalWrite(LED_esp,1); i++;
      }
      else{
          digitalWrite(LED_esp,0);
      }
    }
  }
  else{
      i=0;
      StartPress = millis();
      if(millis() - TimeSetter > Time_interval_first){
        TimeSetter = millis();
        Humidity = Dht.readHumidity();
        TemperaTure = Dht.readTemperature();
        Ph = read_PH(ReadTemperature());
        RelayStatus = Check_Relay(TemperaTure);
        digitalWrite(RELAY_PIN,RelayStatus);
      }
      else if(millis() - TimeShow > Time_interval){
        TimeShow = millis();
        LCD.clear();
        if(Show == 1){
          if(isnan(Humidity) || isnan(TemperaTure)) 
          {
            LCD.setCursor(1, 0);
            LCD.print("Failed to read");
            LCD.setCursor(0, 1);
            LCD.print("check the sensor!");
          }
          else{
            LCD.setCursor(0, 0);
            LCD.print("Humid");
            LCD.setCursor(0, 1);
            LCD.print(Humidity,1);
            LCD.setCursor(7, 0);
            LCD.print("TW");
            LCD.setCursor(6, 1);
            LCD.print(ReadTemperature(),1);
            LCD.setCursor(13, 0);
            LCD.print("TN");
            LCD.setCursor(12, 1);
            LCD.print(TemperaTure,1);
          }
            Show = 0;
          }
          else{
            LCD.clear();
            LCD.setCursor(0, 0);
            LCD.print("PH sensor:");
            LCD.print(Ph);
            LCD.print("!");
            if(Ec > 0){LCD.setCursor(0, 1);LCD.print("EC sensor:");LCD.print(Ec);LCD.print("!");}
            else{LCD.setCursor(0, 1);LCD.print("Ec is Not Check!");}
            Show = 1;
          }
      }
       else if(millis() - SetInfor > Time_interval2){
        SetInfor = millis();
        TimeOf = NowString();
        StatusFb = send_data(Ph,Ec,Humidity,TemperaTure,TimeOf,RelayStatus);
        if(StatusFb != 1){LCD.clear();LCD.setCursor(0, 0);LCD.print("Found problem!");LCD.setCursor(0, 1);LCD.print("Can not Update");}
      }
   }
}

String NowString() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);
  String TmpNow = "";
  TmpNow += String(newtime->tm_year+1900);
  TmpNow += "-";
  TmpNow += String(newtime->tm_mon + 1);
  TmpNow += "-";
  TmpNow += String(newtime->tm_mday);
  TmpNow += " ";
  TmpNow += String(newtime->tm_hour);
  TmpNow += ":";
  TmpNow += String(newtime->tm_min);
  TmpNow += ":";
  TmpNow += String(newtime->tm_sec);
  return TmpNow;
}

float read_EC(float TemperaTureEc){
    float VolTage, EcValue;
    VolTage = analogRead(EC_PIN); 
    EcValue = EC.readEC(VolTage, TemperaTureEc); 
    delay(300);
    return EcValue;
}

float read_PH(float TemperaTurePh){
    float VolTage, PhValue;
    VolTage = analogRead(PH_PIN);
    PhValue = PH.readPH(VolTage, TemperaTurePh);
    delay(300);
    return PhValue;
}
int send_data(float PH_Value,float EC_Value,float HUMIN_Value,float TEMP_Value,String TIME_Value,int STATUS_RELAY){
   if(WiFi.status() != WL_CONNECTED){return 0;}
    else{
//         อันเก่านะ เป็นที่เอาไว้ push data ขึ้นไปนะอย่าลืมละ
//         json.set("Humidity", HUMIN_Value);
//         json.set("Temperature", TEMP_Value);
//         json.set("PH",PH_Value);
//         json.set("DateandTime",TIME_Value);
//         Firebase.pushJSON(firebaseData, "/อุปกรณ์ IOT/Sensor", json);
          Firebase.setFloat(FireBaseData, "/PH_water", PH_Value);
          Firebase.setFloat(FireBaseData, "/EC_water", EC_Value);
          Firebase.setFloat(FireBaseData, "/Humidity", HUMIN_Value);
          Firebase.setFloat(FireBaseData, "/Temperature", TEMP_Value);
          Firebase.setInt(FireBaseData, "/Relay", STATUS_RELAY);
          Firebase.setString(FireBaseData, "/DateandTime", TIME_Value);
          return 1;
    }
}

int Check_Relay(float TemperaTure){
  if(TemperaTure >= 30.00){return 0;}
  else{return 1;}
}
float ReadTemperature()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
  
}
 

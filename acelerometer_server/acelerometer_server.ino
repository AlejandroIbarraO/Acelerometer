#include<Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "Alejandro"
#define STAPSK  "trisb256"
#endif

unsigned int localPort = 8888;      // local port to listen on

const int MPU6050_addr=0x68;
int16_t AccX,AccY,AccZ,Temp,GyroX,GyroY,GyroZ;
String msg;
long timer = 0;
int time_step = 10;// time between readings in milliseconds
bool printing = false;
String output = "wifiudp"; // serial o wifiudp;
int number_of_readings = 0;
long time_zero = 0;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; 
char  ReplyBuffer[] = "acknowledged\r\n";
WiFiUDP Udp;

void setup() {
  Wire.begin();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  if (!MDNS.begin("accelerometer_stream")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }  
  Serial.println("mDNS responder started");
  Serial.printf("UDP server on port %d\n", localPort);
  Udp.begin(localPort);
  MDNS.addService("http", "_udp", localPort);
}

void loop() {
  MDNS.update();
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    msg = Serial.readString();
    msg = msg.substring(0,msg.length()-2);
    server_manager(msg);
  }
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);   
    packetBuffer[n] = 0;   
    server_manager(packetBuffer);
    Serial.print("Contents : ");
    Serial.println(packetBuffer);
  }
  
  if(millis()-timer>time_step){
    update_imu();  
    timer = millis();
  }
}

void server_manager(String message){
  print_data(message);
  int index_coma = message.indexOf(",");
  // one line command
  if(message =="readings"){
    printing = true;
    number_of_readings = 0;
    time_zero = millis();
  }
  if(message =="stop"){
    printing = false;
//    Serial.println(number_of_readings);
    print_data(String(number_of_readings));
  }
  if(index_coma>0){
    // set parameters
    String command = message.substring(0,index_coma);
    String val; 
    val = message.substring(index_coma+1);
    if(command == "time"){
      time_step = val.toInt();
    }
  }
}

void update_imu(){
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_addr,14,true);
  AccX=Wire.read()<<8|Wire.read();
  AccY=Wire.read()<<8|Wire.read();
  AccZ=Wire.read()<<8|Wire.read();
  Temp=Wire.read()<<8|Wire.read();
  GyroX=Wire.read()<<8|Wire.read();
  GyroY=Wire.read()<<8|Wire.read();
  GyroZ=Wire.read()<<8|Wire.read();
  if(printing){
    String data = "R";
    data.concat(String(millis()-time_zero));
    data.concat(',');
    data.concat(String(AccX));
    data.concat(',');
    data.concat(String(AccY));
    data.concat(',');
    data.concat(String(AccZ));
    data.concat(',');
    data.concat(String(GyroX));
    data.concat(',');
    data.concat(String(GyroY));
    data.concat(',');
    data.concat(String(GyroZ));
    //data = String(AccX);//+','+AccY+','+AccZ+','+GyroX+','+GyroY+','+GyroZ;
    print_data(data);
    number_of_readings = number_of_readings+1;
  }
}
//
//void print_readings(){
//  String data;
//  data = String(AccX);//+','+String(AccY)+','+String(AccZ)+','+String(GyroX)+','+String(GyroY)+','+String(GyroZ)+','+String(Temp/340 + 36.53);
//  print_data(data);
//}

void print_data(String out_message){
  if(output == "serial"){
    Serial.println(out_message);
  }
  if(output == "wifiudp"){
    char char_out[out_message.length()+1];
    out_message.toCharArray(char_out,out_message.length()+1);
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(char_out);
    Udp.endPacket();    
  }
}

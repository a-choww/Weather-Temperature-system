#include <DHT.h> //dht22 library by adafruit
#define DHT22PIN 7 //defining dht22 sensor

DHT dht(DHT22PIN,DHT22); //declaring dht22 sensor pins

#include <SoftwareSerial.h>
#include <WiFiEspAT.h> //including the wifi library

#include <Adafruit_GFX.h>      // Core graphics
#include <Adafruit_ST7735.h>   // ST7735 driver
#include <SPI.h>
#include "graphics.h"

Adafruit_ST7735 tft = Adafruit_ST7735(10, 8, 9);

SoftwareSerial esp(0,1); //delcaring esp01s wifi pins 0 is recieving data from wifi and 1 is sending data to wifi

#define SSID "TELUS40ED"
#define PASSWORD "mTRCECBLBBr6"
#define server "https://weather.gc.ca/rss/weather/49.245_-123.115_e.xml"



String ExtractSummaryTag(String xml){  //finds the location of the summary and returns the location of the summary

    int EndIndex = 0;
    int StartIndex = xml.indexOf("<summary type=\"html\">",EndIndex);
    EndIndex = xml.indexOf("</summary>", StartIndex);

    if(StartIndex == -1 || EndIndex == -1){
      return "";
    }
    StartIndex += String("<summary type=\"html\">").length();
    return xml.substring(StartIndex,EndIndex);
}


String ExtractData(String summary, String datapoint){ //extracts specific data base on keyword typed into funciton 

  int start = summary.indexOf("<b>" + datapoint + ":</b>"); //finds the category of data needed and sets location of that data  to start
  if( start == -1){
    return ""; //if not found return funciton 
  }

  start = summary.indexOf("</b>", start); //set the pointer to the end of </b> 
  if( start == -1){
    return "";
  }
  
  start += 4; //skip the pointer of </b> to the have the "pointer" start at the front of the content we need
  int end = summary.indexOf("<br/>", start); //get the end "pointer" of the content  
  if(end == -1){
     end = summary.length();
  } 

  String out = summary.substring(start, end); //we extract the string content between start and end index
  out.trim();
  return out;
}

String FetchSummary(){
  if(WiFi.status() != WL_CONNECTED){ //if we cannot connect to wifi, return function
    Serial.println("Wifi could not connect");
    return "";
  }

  WiFiSSLClient client;
  Serial.println("Connecting to Wifi");

  if (client.connect("weather.gc.ca", 443) == false) { //if we cannot reach the weather site return function.
    Serial.println("Connection to weather site failed");
    return "";
  }

  //Sending the request to get  the XML
  client.println("GET /rss/weather/49.245_-123.115_e.xml HTTP/1.1");
  client.println("Host: weather.gc.ca");
  client.println("Connection: close");
  client.println();


  //reading the response we got from request
  String response;
  while(client.connected() || client.available()) {
    if(client.available()) {
      response += (char)client.read();
    }
  }
  client.stop();

  //Only extract the summary text
  String summarytext;
  int BodyPosition = response.indexOf("\r\n\r\n");
  if(BodyPosition >= 0){ //check if our text may have something wrong.
    summarytext = response.substring(BodyPosition + 4); //+4 to change position to the front of the main text
  }
  else{ //something is wrong and we just return original text
    summarytext = response;
  }

  //use our ExtractSummaryTag to extract the summary and return it
  String summary = ExtractSummaryTag(summarytext);
  return summary;
}


void setup() {

  int index = 2; // for the second summary in the XML file 

  Serial.begin(115200);

  //setting up the dht22
  dht.begin();
  delay(500);

  //setting up wifi 

  WiFi.init(esp);
  WiFi.reset();
  delay(1000);

  Serial.println("Connecting to wifi");

  if (WiFi.begin(SSID, PASSWORD) != WL_CONNECTED) {
    Serial.println("WiFi could not be connected");
  }
  else{
    Serial.print("Wifi connected to IP: ");
    Serial.println(WiFi.localIP());
  } 
  
  //setting up the display screen
  tft.initR(INITR_BLACKTAB); 
  tft.setRotation(1); //landscape  view
  tft.fillScreen(ST77XX_BLACK); //black background


  tft.fillRect(0, 128, 160, 32, ST77XX_BLACK); 

}

void loop() {
  // put your main code here, to run repeatedly:

  float humidity = dht.readHumidity(); //reading the humidity
  float temperature = dht.readTemperature();//reading the temperature (Celsius)

  // Checking if dht22 fails to read
  if (isnan(humidity) || isnan(temperature)){
    Serial.println ("DHT22 failed to read...");
    return;
  }
  else{
    Serial.print ("Current Temperature: ");
    Serial.print (temperature,1);
    Serial.print (" °C and Humidity is ");
    Serial.print (humidity,1);
    Serial.print ("% ");
    Serial.println ("");
  }
 
  //fetching summary text from weather app and extracting current conditions
  String summary = FetchSummary();
  Serial.println(summary);
  Serial.println("Condition: "+ ExtractData(summary,"Condition"));

  //printing conditions on the display screen
  tft.setCursor(2,100);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.print("Current Conditions: ");
  //tft.print(ExtractData(summary,"Condition"));
 
  tft.setCursor(2,110);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
  tft.print("Temperature: ");
  tft.print(temperature,1);
  tft.print("C");

  tft.setCursor(2,120);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
  tft.print("Humidity: ");
  tft.print(humidity,0);
  tft.print("% ");

  if(ExtractData(summary,"Condition") == "Cloudy" || "Mostly Cloudy"){
    tft.drawRGBBitmap(48, 0, Cloudy, 64,64);
  }
  else if(ExtractData(summary,"Condition") == "Sunny" || "Mainly Sunny"){
    tft.drawRGBBitmap(48, 0, Sunny, 64,64);
  }
  else if(ExtractData(summary,"Condition") == "Lighting"){
    tft.drawRGBBitmap(48, 0, Lighting, 64,64);
  }
  else if(ExtractData(summary,"Condition") == "Rain" || "Light Showers"){
    tft.drawRGBBitmap(48, 0, Rain, 64,64);
  }
  else if(ExtractData(summary,"Condition") == "Snow"){
    tft.drawRGBBitmap(48, 0, Snow, 64,64);
  }
  else if(ExtractData(summary,"Condition") == "Windy"){
    tft.drawRGBBitmap(48, 0, Windy, 64,64);
  }
  /*else if(ExtractData(summary,"Condition") == "Clear"){
    tft.drawRGBBitmap(48, 0, Sunny, 64,64);
  }*/ //for night time
  
  delay(1000); //adding some buffer time before reading again

}

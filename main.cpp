#include <Arduino.h>

/*
   Device Title: MK-BlindsControl
    Device Description: MQTT Blinds Control
   Device Explanation: The device recieves an MQTT message from the server and
                       changes the position of the servo motor
   Device information: https://www.MK-SmartHouse.com/blinds-control

   Author: Matt Kaczynski
   Website: http://www.MK-SmartHouse.com
   Version 1 and 2

   Original software modified by Mountain-Eagle Pitt
   Website: www.mountaineagle-technologies.com.au V6 and above

   

   
*/

/* ---------- DO NOT EDIT ANYTHING IN THIS FILE UNLESS YOU KNOW WHAT YOU ARE DOING---------- */

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <LittleFS.h>
#include <Servo.h>
Servo myservo[2];    ///allow for 2 servo's to be connected


#include <WiFiClient.h>


#include <MQTTClient.h>           //https://github.com/256dpi/arduino-mqtt
#include <DNSServer.h>

#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson   V6 and above only
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>



#include <ESP8266WiFi.h>       // Built-in
#include <ESP8266WebServer.h>  // Built-in

ESP8266WebServer httpServer(80);

ESP8266HTTPUpdateServer httpUpdater;
#include <ESP8266SSDP.h>

#include <HAMqttDevice.h> //HA implementation



#define ServerVersion "8"
String  webpage = "";

#include "PageIndex.h" //--> Include the contents of the User Interface Web page, stored in the same folder as the .ino file
bool    SPIFFS_present = false;
#include "CSS.h"
#include <SPI.h>


#include "user_interface.h"
//wifi_set_sleep_type(MODEM_SLEEP_T);
//define your default values here, if there are different values in config.json, they are overwritten.
char host[34];
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_topic[50];  ///MQTT Device ID
char mqtt_isAuthentication[7] = "FALSE";
char mqtt_username[40];
char mqtt_password[40];
char update_username[40] = "admin";
char update_password[40] = "password";
char auto_discovery[20] = "DISABLED";
char remote_switch[6] = "NO";
char update_path[34] = "/firmware"; //Default
char blinds_speed[7] = "FAST";  //Default
char blinds_swing_direction[7] = "DOWN"; //Default
char blinds_servo_install[7] = "LEFT";  //Default
char blinds_trim_adjust[3] = "0";  //Default
char blinds_slip_correction[5] = "ON";  //Default
char OTAAuto_path[90] = "http://mountaineagle-technologies.com.au/tasmota/mk-blindcontrol.bin"; //updtate OTA_url server path and file
char tele_battery_set[4] = "60";   ////in seconds
char tele_update_set[4] = "60";
char open_limit_set[5] = ""; ///open limit set, set by user and program can be inverter
char close_limit_set[5] = "";  ////close limit set, set by user and program can be inverter
char open_limit_default[5] = ""; ///open limit default position
char close_limit_default[5] = "";  ////closeed limit default position

int openVal;
int openTemp = 20;
int closedVal;
int closedTemp;
String BufferPos = "0";


//Unique Software ID and Version Information
char software_name[40] = "MK-BlindsControl";
char software_version_old[7] = "V7"; ///previous software version
char software_version[7] = "V8";   //changing this value will cause WiFiManager to Reload and will have to re-configure Device eg if size of json changes then change this
char software_variant[7] = "00";   //change this for minor changes only to program...will not cause re configure options
String firmware_installed = String(software_version)+"."+String(software_variant);
String url = "http://mountaineagle-technologies.com.au/tasmota/version.json";
const char* POWER_TOPIC = "cmnd/power/POWER";
char data[80];
int msgcommand = 180;  //payload converted to initger number
String msgpayload = "NULL"; 
String msgString = "180";//
String TiltPos = "100"; ////tilt position
String HA_Blind_State = "OPEN"; /// state for HA open, closed, opening, closing, stopped
String Blind_STATE = "100";  ////used in tele data 
String blindoffset = "0";
String blindstrim = blinds_trim_adjust;


//char msgpayload[7] = "NULL";
int ServoPos = 180;
int reboot = 0;
int blinddelay = 0;
int restartflag = 1;
int bypassdevstat = 0;
boolean isNum = false;
//Unique device ID
const char* mqttDeviceID;

///////variable for battery monitor

int Battery_Cap = 100;
float Battery_Voltage = 4.7;
float Remaining_Time = 0.0;
float Discharge_Time = 6.5;   ///discharge time in hours
char battery_system[6] = "OFF";
char battery_capacity[8] = "3800";   //capacity in mHa
char system_power[8] = "10";   ///power in watts
unsigned long time_now_2 = 0;   ///use for battery monitor reportng

//////NEW CODE FOR 4 FUNCTION BUTTON STATE

const int buttonPin = 0;     // analog input pin to use as a digital input
//  MULTI-CLICK:  One Button, Multiple Events

// Button timing variables
unsigned long debounce = 50;   // ms debounce period to prevent flickering when pressing or releasing the button
unsigned long DCgap = 250;            // max ms between clicks for a double click event
unsigned long holdTime = 2000;        // ms hold period: how long to wait for press+hold event
unsigned long longHoldTime = 6000;    // ms long hold period: how long to wait for press+hold event
int button_result = 0;                    //button press result
int b = 0;
// Button variables
boolean buttonVal = HIGH;   // value read from button
boolean buttonLast = HIGH;  // buffered value of the button's previous state
boolean DCwaiting = false;  // whether we're waiting for a double click (down)
boolean DConUp = false;     // whether to register a double click on next release, or whether to wait and click
boolean singleOK = true;    // whether it's OK to do a single click
long downTime = -1;         // time the button was pressed down
long upTime = -1;           // time the button was released
boolean ignoreUp = false;   // whether to ignore the button release because the click+hold was triggered
boolean waitForUp = false;        // when held, whether to wait for the up event
boolean holdEventPast = false;    // whether or not the hold event happened already
boolean longHoldEventPast = false;// whether or not the long hold event happened already

bool lightOn = false;


/////////End Code 4 FUNCTION BUTTON STATE
String valueString = "180";
    int pos1 = 0;
    int pos2 = 0;

/////////new code for switch detection
// constants won't change. They're used here to set pin numbers:
//const int buttonPin = 0;    // the number of the pushbutton pin
const int ledPin = 12;      // the number of the LED pin

// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 200;    // the debounce time; increase if the output flickers


////////////////////////////
//Form Custom SSID based on Software name and version information and Chip ID
String ssidAP = String(software_name) + "-" + String(software_version) + "-" + String(ESP.getChipId());

//Set Default States

boolean invert_state = false;
boolean invert_command = false;
////////HA name conversiona
String _name;
String _identifier;
////////

///LWT Config settings
String returnmsg;
int reconnects = 0;
const int RSSI_MAX = -50; // define maximum strength of signal in dBm
const int RSSI_MIN = -100; // define minimum strength of signal in dBm


// Set tele reporting interval to MQTT broker for STATE, WiFi Level etc
int tele_period = 60000;  //time in mili seconds 1000 ms = 1 sec
unsigned long time_now = 0;


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback ()
{
  shouldSaveConfig = true;
}

WiFiClient net;
int MQTTsize = 1200;
MQTTClient client(1200);

unsigned long lastMillis = 0;
/////////////////////////////
void moveServo(double pos)
{
  
myservo[0].write(ServoPos);
myservo[1].write(ServoPos);
if (!myservo[0].attached()) {  // If the servo is not attached
    myservo[0].attach(13, 544, 2200);
    myservo[1].attach(14, 544, 2200);
  }

   delay(40); //40
 
blinddelay = 0;
if (String(blinds_speed).equalsIgnoreCase("LOW")){    
  blinddelay = 80;
  }
  
if (String(blinds_speed).equalsIgnoreCase("MED")){    
   blinddelay = 40;
  }

if (String(blinds_speed).equalsIgnoreCase("HIGH")){    
  blinddelay = 0;
  }  
  ///////////////////////////////
  
   //if ((String(blinds_speed).equalsIgnoreCase("LOW"))||(String(blinds_speed).equalsIgnoreCase("MED")))
  if (String(blinds_speed).equalsIgnoreCase("SLOW"))
  {
    if (pos < myservo[0].read())
    {
      for (int tempPos = myservo[0].read(); pos <= tempPos; tempPos--)
      {
        
     
    // write to each server 
        myservo[0].write(tempPos);
        myservo[1].write(tempPos);
                 
        delay(40); //40
      }
    }
    else
    {
      for (int tempPos = myservo[0].read(); pos >= tempPos; tempPos++)
      {
       
            
    // write to each server 
        myservo[0].write(tempPos);
        myservo[1].write(tempPos);
        
        delay(40); //40
      }
    }
  }
  else
  {
        
    // write to each server 
        myservo[0].write(pos);
        myservo[1].write(pos);
       
    delay(2000);
  }
  ServoPos = myservo[0].read();
}
///////////////////////////

//////////////////////////////

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void AutoConfigBlind(){
 //////////////////////////////////////////////////////////
  ////SET UP CODE FOR AUTO CONFIG
  ///Code to setup  servo,  for swing state etc based on initial setup of blinds_swing_direction and  side servo motor is installed on for blinds_servo_install
  ///If you change sides of sevo motor the commands will have to be inverted as well as the states, the code below automatically sets these based on
  ///WiFiManager Setup options . What Side Servo Motor Installed On and Swing Direction To Close.So Commands remain true states ie.  0=OPEN, 100=CLOSED etc
  // The Configuration Option Selected Is Published To MQTT on boot up of Blinds Controller
  
  
  if ((String(blinds_servo_install).equalsIgnoreCase("LEFT")) && (String(blinds_swing_direction).equalsIgnoreCase("DOWN")))
  {
    invert_command = false;
    invert_state = false;      
  }


  else if ((String(blinds_servo_install).equalsIgnoreCase("LEFT")) && (String(blinds_swing_direction).equalsIgnoreCase("UP")))
  {    
    invert_command = true;
    invert_state = true;        
  }


  else if ((String(blinds_servo_install).equalsIgnoreCase("RIGHT")) && (String(blinds_swing_direction).equalsIgnoreCase("DOWN")))
  {    
    invert_command = true;
    invert_state = true;   
  }

  else if ((String(blinds_servo_install).equalsIgnoreCase("RIGHT")) && (String(blinds_swing_direction).equalsIgnoreCase("UP")))
  {    
    invert_command = false;
    invert_state = false;      
  }

/////check for valid open limit set else pre configure
if ((String(open_limit_set).equalsIgnoreCase("")) || (String(close_limit_set).equalsIgnoreCase("")))
  {
    if (invert_command == true)
    
      {
        strcpy(open_limit_set, "180");
        strcpy(close_limit_set, "0");       
      }
    else
    {
        strcpy(open_limit_set, "0");
        strcpy(close_limit_set, "180");        
    }
   
  }
//////set default limits reference based on swing directoin etc
if (invert_command == true)

      {
        strcpy(open_limit_default, "180");
        strcpy(close_limit_default, "0");
       
      }
    else
    {
        strcpy(open_limit_default, "0");
        strcpy(close_limit_default, "180");  
       
    }


  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  httpServer.sendContent("");
  httpServer.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~
//////config copy upgrade

void configupgrade(){
//Serial.print("Checking For Configuration file "); 
  if (LittleFS.begin())
    {
    SPIFFS_present = true; 
    //////check for version 7 if so copy to V8

    //////
    
    if (LittleFS.exists("/" + String(software_version_old) + ".json"))
    {
      //file exists, reading and loading
      
      File configFile = LittleFS.open("/" + String(software_version_old) + ".json", "r");
      if (configFile)
      {
       //Serial.print("Con figuraton found....loading values ");
        size_t size = configFile.size();   
        ///size_t len = measureJson(configFile); json 6
        
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        
        DynamicJsonDocument json(1024);  ////json6
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
       
        configFile.close();
        LittleFS.remove("/" + String(software_version_old) + ".json");   ///remove existing file before re writting

        ///if (json.success())  /////JSON5
        if ( ! deserializeError )    ////JSON6
        {

         /////save to new file  
           //LittleFS.remove("/" + String(software_version_old) + ".json");   ///remove existing file before re writting
           File configFile = LittleFS.open("/" + String(software_version) + ".json", "w"); 

            serializeJson(json, Serial);    ////JSON6
            serializeJson(json, configFile);    ///JSON6

            configFile.close(); 
        }
        else
        {
        }
        /////erase spiffs


      }

    }

  }
  else
  {
  }
  //end read

}


//Procedure for handling servo control

void handleServo(){

  String POS = httpServer.arg("servoPOS");
  BufferPos = POS;

  int pos = POS.toInt();
  
   //moveServo(pos);
  myservo[0].write(pos);   //--> Move the servo motor according to the POS value
  delay(15);
  myservo[1].write(pos);   //--> Move the servo motor according to the POS value
  delay(15);

  
  httpServer.send(200, "text/plane","");

}

///////////////all function go here before setup and loop
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void process_state()

{
//client.publish("stat/" + _identifier + "/caculating_State ", "processing_state");
blindoffset = "0";
String str_open_limit_set = open_limit_set;
String str_open_limit_default = open_limit_default;
String str_close_limit_set = close_limit_set;
String str_close_limit_default = close_limit_default;
  
  if (HA_Blind_State == "OPENED")
    {
      
      if (invert_state == true)
      {
         /////////      180     minus    50 
          blindoffset = String(str_open_limit_set.toInt() - str_open_limit_default.toInt());          
      }
         else
         {
           ///////////   0       plus    20
           blindoffset = String(str_open_limit_default.toInt() + str_open_limit_set.toInt());           
         }
    }


    
  if (HA_Blind_State == "CLOSED")
    {
      // client.publish("stat/" + _identifier + "/Processing", "Closed");
      if (invert_state == true)
      {
        ////////////                  0          plus   20 
        blindoffset =  String(str_close_limit_default.toInt() + str_close_limit_set.toInt());   ////maybe plus       
      }
      else
      {
        /////                    180                minus   160
        blindoffset =  String(str_close_limit_set.toInt() - str_close_limit_default.toInt());        
      }
      
    }
 




  if (invert_state == true)
    //if (String(blinds_invert_state).equalsIgnoreCase("YES"))
    {
        if ((HA_Blind_State == "OPENED") || (HA_Blind_State == "CLOSED"))
           {
            ///use trim adjustmet on opened and closed vales only           
            Blind_STATE = String((int)(100 - ((myservo[0].read()-blindoffset.toInt()) / 1.8) + 0.5));
             ////new format
            TiltPos = String((int)(100 - ((myservo[0].read() / 1.8) - 0))); //
          }
          else 
          {
            ///////don't use trim offsets if blind is not opem\ned or closed fully            
            Blind_STATE = String((int)(100 - (myservo[0].read() / 1.8) + 0.5));
            TiltPos = String((int)(100 - ((myservo[0].read() / 1.8) - 0) )); // 
          }
 ///////////////////////////////////////////   
    } 
    
    
  else  
        //////////////////opened = 0     closed   180     non inverted
    {
      if ((HA_Blind_State == "OPENED") || (HA_Blind_State == "CLOSED"))
           {
            
            ///use trim adjustmet on opened and closed vales only                        
            Blind_STATE = String((int)(((myservo[0].read()-blindoffset.toInt()) / 1.8) + 0.5)); 
            TiltPos = String((int)(((myservo[0].read() / 1.8) - 0) )); // add trim offsets

                  
          }
          else 
          {           
            ///////don't use trim offsets if blind is not opem\ned or closed fully
            Blind_STATE = String((int)((myservo[0].read() / 1.8) + 0.5)); 
            TiltPos = String((int)(((myservo[0].read() / 1.8) - 0) )); // add trim offsets
      
          }
       
     
    }
    ServoPos = myservo[0].read();   //set servo posiotion to save in
}

////////////////////////////////////////////
void publish_state()

{

client.publish("stat/" + _identifier + "/STATE", Blind_STATE); 
client.publish("stat/" + _identifier + "/HA_STATE", HA_Blind_State); 
client.publish("stat/" + _identifier + "/SPEED", String(blinds_speed)); //publish blid speed
client.publish("stat/" + _identifier + "/position", String(TiltPos));
client.publish("stat/" + _identifier + "/tilt-state", TiltPos); //tit message=power message





}

//////////////////////////////
int dBmtoPercentage(int dBm)
{
  int quality;
  if (dBm <= RSSI_MIN)
  {
    quality = 0;
  }
  else if (dBm >= RSSI_MAX)
  {
    quality = 100;
  }
  else
  {
    quality = 2 * (dBm + 100);
  }

  return quality;
}//dBmtoPercentage

/////////////////////////////
void HA_State()
{
//////Create HA_Blind_State 
 ///HA_Blind_State
 ////only do this while servo connected
// client.publish("stat/" + _identifier + "/HA_STATE","processig1");
 HA_Blind_State = "OPEN";
  
 
 if (String(myservo[0].read()) == String(open_limit_set))
 {
   HA_Blind_State = "OPENED";
 }
 if (String(myservo[0].read()) == String(close_limit_set))
 {
   HA_Blind_State = "CLOSED";

 }
 
}


//////////////////
void HAMDiscovery()
{
_name = String(mqtt_topic);  ///aka mqtt device ID will change in main program
HAMqttDevice mkblindcontrol(_name, HAMqttDevice::COVER);


  // Configure extra config vars for Home assistant
  mkblindcontrol
    .enableAttributesTopic()
    
    //.addConfigVar("cmd_t", "cmnd/"+_identifier+"/POWER")   //command topic
    //.addConfigVar("stat_t", "stat/"+_identifier+"/STATE")    //STATE topic
    .addConfigVar("retain", "false")   ///retain flag
    .addConfigVar("availability_topic", "tele/"+_identifier+"/LWT")
    .addConfigVar("payload_open", "OPEN")   ///pay load open
    .addConfigVar("payload_close", "CLOSED")   ///payload closed
    .addConfigVar("payload_stop", "STOP")   ///pay load stop
    .addConfigVar("stat_open", "0")   ///state open
    .addConfigVar("state_opening", "opening")   ///state opening
    .addConfigVar("state_closed", "100")   ///state closed
    .addConfigVar("state_closing", "closing")   ///state closing
    .addConfigVar("payload_available", "online")   ///payload available
    .addConfigVar("payload_not_available", "offline")   ///payload not avilable
    .addConfigVar("optimistic", "false")   ///optimistic
    .addConfigVar("value_template", "{{ value.x }}")   ///value template
    .addConfigVar("position_template", "{{ value.y }}")   ///position value
    .addConfigVar("tilt_command_topic", "cmnd/"+_identifier+"/tilt")   ///tilt command topic
    .addConfigVar("tilt_status_topic", "stat/"+_identifier+"/STATE")   ///tilt status topic
    .addConfigVar("tilt_status_template", "{{ value_json['PWM']['PWM1'] }}")   ///tilt status template
    .addConfigVar("tilt_min", "0")   ///tilt minium
    .addConfigVar("tilt_max", "100")   ///tilt maxium
    .addConfigVar("tilt_closed_value", "0")   ///tilt closed value
    .addConfigVar("tilt_opened_value", "100")   ///tilt open value
    .addConfigVar("device_class", "blind");   ///device class
   
int len2 = mkblindcontrol.getConfigPayload().length()+1;
client.publish(mkblindcontrol.getConfigTopic(), mkblindcontrol.getConfigPayload(), len2, true);

mkblindcontrol
          .clearAttributes()
          .addAttribute("IP", WiFi.localIP().toString());
        client.publish(mkblindcontrol.getAttributesTopic(), mkblindcontrol.getAttributesPayload());


}







//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
////HA Auto descovery type1

/////////////////////////////////////
//
void HA_Discovery(){
  //Serial.print("Generating Homeassistant Discovery ");         
/////asign variable from main
_name = String(mqtt_topic);  ///aka mqtt device ID will change in main program
HAMqttDevice mkblindcontrol(_name, HAMqttDevice::COVER);


  // Configure extra config vars for Home assistant
  mkblindcontrol
    .enableAttributesTopic();

// Id = name to lower case replacing spaces by underscore (ex: name="Kitchen Light" -> id="kitchen_light")
    _identifier = _name;
    _identifier.replace(' ', '_');
    _identifier.toLowerCase();

  StaticJsonDocument<800> root;


    root["platform"] = "mqtt";
    root["~"] = _identifier;
    root["name"] = String(mqtt_topic);
    root["unique_id"] = _identifier;
    root["command_topic"] = "cmnd/"+_identifier+"/POWER";
    root["state_topic"] =  "stat/"+_identifier+"/STATE";
    root["availability_topic"] = "tele/"+_identifier+"/LWT";
    root["retain"] = false;
    root["payload_open"] = "0";
    root["payload_close"] = "100";
    root["state_open"] = "0";
    root["state_closed"] = "100";
    root["payload_available"] = "Online";
    root["payload_not_available"] = "Offline";
    //root["value_template"] = "{% if value == '100' -%}closed{%- else -%}open{%- endif %}";
    if (String(auto_discovery) == "ENABLED-TILT")
    {
    root["position_open"] = 0;
    root["position_closed"] = 100;
    root["tilt_command_topic"] = "cmnd/"+_identifier+"/tilt";
    root["tilt_status_topic"] = "stat/"+_identifier+"/tilt-state";
    root["tilt_min"] = 0;
    root["tilt_max"] = 100;
    root["tilt_closed_value"] = 0;
    root["tilt_opened_value"] = 100;
    }
    root["device_class"] = "blind";
    
    size_t len = measureJson(root);   ///////JSON6
    //size_t len = root.measureLength();    ////JSON5
    size_t size = len + 1;

    char JSONmessageBuffer[size];

    //root.printTo(JSONmessageBuffer, size);   /////json5
    serializeJson(root, JSONmessageBuffer, size);   ////json6
  
    client.publish("homeassistant/cover/"+_identifier+"/config", JSONmessageBuffer, size, true);
    //client.publish("ha/cover/"+_identifier+"/config", JSONmessageBuffer, size, true);
    ///

    ///////process attributes
    mkblindcontrol
          .clearAttributes()
          .addAttribute("IP", WiFi.localIP().toString())
          .addAttribute("manufacturer", "MK-Smarthouse")
          .addAttribute("model", "MK-BlindControl")
          .addAttribute("sw_version", String(firmware_installed));
        client.publish(mkblindcontrol.getAttributesTopic(), mkblindcontrol.getAttributesPayload());

//////////ADD SENSOR DATA FOR BATTERY MONITOR



//////////////////////////////////////////////
}

void SendHTML_Header(){
  httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  httpServer.sendHeader("Pragma", "no-cache"); 
  httpServer.sendHeader("Expires", "-1"); 
  httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  httpServer.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  httpServer.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  httpServer.sendContent(webpage);
  webpage = "";
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  //webpage += F("<a href='/'><button>Back</button></a>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSPIFFSNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>No SPIFFS present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  //webpage += F("<a href='/'><button>Back</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}

///---------------------------------------------------------------------------
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DownloadFile(String filename){
  if (SPIFFS_present) { 
   /// File download = SPIFFS.open("/"+filename,  "r");    ///old system
    File download = LittleFS.open("/"+filename,  "r");
    if (download) {
      httpServer.sendHeader("Content-Type", "text/text");
      httpServer.sendHeader("Content-Disposition", "attachment; filename="+filename);
      httpServer.sendHeader("Connection", "close");
      httpServer.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSPIFFSNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload(){
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  //webpage += F("<a href='/'><button>Back</button></a>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  
  append_page_footer();
  httpServer.send(200, "text/html",webpage);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SPIFFS_file_delete(String filename) { // Delete the file 
  if (SPIFFS_present) { 
    SendHTML_Header();
    ///File dataFile = SPIFFS.open("/"+filename, "r"); // Now read data from SPIFFS Card /////old system
    File dataFile = LittleFS.open("/"+filename, "r"); // Now read data from SPIFFS Card 
    if (dataFile)
    {
      ///if (SPIFFS.remove("/"+filename)) {    ////old system
      if (LittleFS.remove("/"+filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '"+filename+"' has been erased</h3>"; 
        webpage += F("<a href='/'><button>Back</button></a>");
        //webpage += F("<a href='/delete'>[Back]</a><br><br>");
      }
      else
      { 
        webpage += F("<h3>File was not deleted - error</h3>");
       webpage += F("<a href='/'><button>Back</button></a>");
        //webpage += F("<a href='delete'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSPIFFSNotPresent();
} 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Delete(){
  if (httpServer.args() > 0 ) { // Arguments were received
    if (httpServer.hasArg("delete")) SPIFFS_file_delete(httpServer.arg(0));
  }
  else SelectInput("Select a File to Delete","delete","delete");
}
//////////////////////////////////////
/////////////////////////////
///////////////////////////
void get_save_state() {

//////start checkpoint1
    ///if (SPIFFS.exists("/devicestate.dat")) ///old system
    if (LittleFS.exists("/devicestate.dat"))
    
    {
      Serial.print("Retriving Divice State..devicestate.data ");
      //file exists, reading and loading
      ///File configFile = SPIFFS.open("/devicestate.dat", "r");    ///old system
      File configFile = LittleFS.open("/devicestate.dat", "r");
      if (configFile)
      {
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        DynamicJsonDocument devicestate(1024);     ////JSON6
        auto deserializeError = deserializeJson(devicestate, buf.get());     ////json6
        serializeJson(devicestate, Serial);     //////JSON6


        ///DynamicJsonBuffer jsonBuffer;   /////JSON5
        //JsonObject& devicestate = jsonBuffer.parseObject(buf.get());    //////JSON5
        ///devicestate.printTo(Serial);    /////JSON5


        configFile.close();



        ///if (devicestate.success())  ///JSON5
        if ( ! deserializeError )   ////JSON6
        {
          //client.publish("tele/" + String(_identifier) + "/DEVICE_STATE", "RETRIEVED");
           ServoPos = devicestate["ServoPos"]; 
           msgcommand = devicestate["POWER"];                          
          Blind_STATE = devicestate["STATE"].as<const char*>();           
           msgString = devicestate["TILT"].as<const char*>(); 
           TiltPos = devicestate["TILT"].as<const char*>();
           HA_Blind_State = devicestate["HAState"].as<const char*>(); 
          TiltPos = devicestate["PWM1"].as<const char*>();  
          msgpayload = devicestate["cmnd"].as<const char*>(); 
          strcpy(host, devicestate["Device"]);
          strcpy(blinds_speed, devicestate["SPEED"]);
          
          
        }
        else
        {
        }
      }

    }

}
///////////////////////////////////////////////////////////////////////
///OTA Server needs to be running eg on Raspberry Pi etc or access to resporitory 
void OTAUpgrade() {
//Serial.print("OTA Upgrade Iniated... ");
restartflag = 1;
SendHTML_Header();
  webpage += F("<h3 class='rcorners_m'>Auto Firmware Updater</h3><br>");
  webpage += F("<h3> Connecting to OTA Server </h3>");
  webpage += F("<h3> Downloading/Installing Firmware </h3>");
  webpage += F("<h3> If Sucessfull and No futher Progress Displayed  </h3>");
  webpage += F("<h3> Wait about 20 seconds</h3>");
  webpage += F("<h3> Press Reboot to Activate New Firmware  </h3>");
  webpage += F("<h3> Wait for about 10 seconds before pressing Back</h3>");
  //webpage += F("<h3> Blind Will Jolt After Restart Then Press Back  </h3>");
  webpage += F("<a href='/'><button>Back</button></a>");
 webpage += F("<a href='/reboot'><button>Reboot</button></a>");
  SendHTML_Content();
            
  //ESPhttpUpdate.rebootOnUpdate(false);
  //t_httpUpdate_return ret = ESPhttpUpdate.update(net, OTAAuto_path);
  t_httpUpdate_return ret = ESPhttpUpdate.update(net, OTAAuto_path);
  //t_httpUpdate_return ret = ESPhttpUpdate.update(net, "http://mountaineagle-technologies.com.au/tasmota/mk-blindcontrol.bin");
  
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.print("OTA Upgrade Failed ");
      client.publish("tele/" + _identifier + "/UPDATE", "HTTP_UPDATE_FAILD");
      //Serial.println("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      //httpServer.send(201, "text/plain", "OTA Update Failed Error DownLoading Firmware from OTA Server");
      
      webpage += F("<h3> OTA Update Failed Error DownLoading Firmware from OTA Server  </h3>");
      //webpage += F("<h3> OTA Update Failed Error DownLoading Firmware from OTA Server  </h3>");
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.print("No Updates Available from OTA Server ");
      client.publish("tele/" + _identifier + "/UPDATE", "HTTP_UPDATE_NO_UPDATES");
      //httpServer.send(201, "text/plain", "No Updates Available from OTA Server ");
      webpage += F("<h3> No Updates Available from OTA Server  </h3>");
      //USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      client.publish("tele/" + String(_identifier) + "/UPDATE", "HTTP_UPDATE_OK");
      //httpServer.send(201, "text/plain", "OTAUpdate OK , DownLoading Firmware from OTA Server and Installing ");
      Serial.println("HTTP_UPDATE_OK");
      webpage += F("<h3> OTAUpdate OK   </h3>");
            
      break;
  }
  
   webpage += F("<a href='/'><button>Back</button></a>"); 
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent

}
//////////////////////////////////////////////

void handleMoveServo() {

     
    // Attach the servo to the servo object 
   //(13, 544, 2200);
  // myservo[0].write(ServoPos);
  if (!myservo[0].attached()) {  // If the servo is not attached
    myservo[0].attach(13, 544, 2200);
    myservo[1].attach(14, 544, 2200);
  }
  
  
  
  //myservo[0].attach(13, 544, 2200);
  //myservo[1].attach(14, 544, 2200);
    // Wait 500 milliseconds 
    delay(500);  
  



String s = MAIN_page; //Read HTML contents
//String x = document.getElementById("demo");
SendHTML_Header();
   httpServer.arg("this.value") == String(myservo[0].read());
   httpServer.arg("demo") == String(myservo[0].read());
   webpage += F("<h3 class='rcorners_m'>Set Open and Closed Limits</h3><br>");

   webpage += F("<table align='center'>");
  webpage += "<tr><td>System Generated Open Limit </td><td>"+String(open_limit_default)+"</td></tr>"; 
  webpage += "<tr><td>User Set Open Limit</td><td>"+String(open_limit_set)+"</td></tr>";
  webpage += "<tr><td>System Generated Closed Limit</td><td>"+String(close_limit_default)+"</td></tr>";
  webpage += "<tr><td>User Set Closed Limit</td><td>"+String(close_limit_set)+"</td></tr>";
  webpage += F("</table>");
   webpage += s;
  
  webpage += F("<a href='/SetOpenLimit'><button>Set Open</button></a>");
  webpage += F("<a href='/ResetLimits'><button>Reset</button></a>");
  webpage += F("<a href='/SetClosedLimit'><button>Set Closed</button></a>");
  
 //String s = MAIN_page; //Read HTML contents
append_page_footer();
  SendHTML_Content();

  SendHTML_Stop(); // Stop is needed because no content length was sent
 //httpServer.send(200, "text/html", s); //Send web page

}
////////////////////////////////////
void SetOpenLimit(){
 SendHTML_Header();
   webpage += F("<h3 class='rcorners_m'>Set Open and Closed Limits</h3><br>");
  Serial.print("Open Limit Set To");
  Serial.println(BufferPos.toInt());
  strcpy(open_limit_set, BufferPos.c_str());
  webpage += ("<h3>Open Limit Has Been Set to <h3>"+String(open_limit_set)+"</h3>");
  webpage += F("<h3> Setting has not been saved </h3>");
  webpage += F("<h3> After Setting Limits, select SAVE from menu  </h3>");
  webpage += F("<h3> Press back to return to Limit settings </h3>");
  webpage += F("<a href='/set-limits'><button>Back</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
////////////////
void SetClosedLimit(){
SendHTML_Header();
   webpage += F("<h3 class='rcorners_m'>Set Open and Closed Limits</h3><br>");
  Serial.print("Closed Limit Set To");
  Serial.println(BufferPos.toInt());
  strcpy(close_limit_set, BufferPos.c_str());
  webpage += ("<h3>Closed Limit Has Been Set to <h3>"+String(close_limit_set)+"</h3>"); ///tr
  webpage += F("<h3> Setting has not been saved </h3>");
  webpage += F("<h3> After Setting Limits, select SAVE from menu  </h3>");
  webpage += F("<h3> Press back to return to Limit settings </h3>");
  webpage += F("<a href='/set-limits'><button>Back</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();

}
////////////////
////////////////
void ResetLimits(){
SendHTML_Header();
   webpage += F("<h3 class='rcorners_m'>Set Open and Closed Limits</h3><br>");
  Serial.print("Reset Limits Set To Default");
  strcpy(close_limit_set, "");
  strcpy(open_limit_set, "");
  webpage += F("<h3> Closed and Open Limits Have Been Set to Defaults</h3>");
  webpage += ("<h3>Closed Limit Has Been Set to Auto Setup<h3>");
  webpage += ("<h3>Open Limit Has Been Set to Auto Setup<h3>"); ///tr ///tr
  webpage += ("<h3>System will re-configure on reboot<h3>"); ///tr ///tr
  
  webpage += F("<h3> Settings have not been saved </h3>");
  webpage += F("<h3> After Re-Setting Limits, select SAVE from menu  </h3>");
  webpage += F("<h3> Restart is Required for setting to take place  </h3>");
  reboot = 1;
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
  
}
////////////
////////////////////////////////////
void save_state() {
//New code to safe device state to SPIFF file
//Serial.print("Saving device state ");

//const size_t capacitystate2 = JSON_OBJECT_SIZE(12);
StaticJsonDocument<512> devicestate;

devicestate["Device"] = String(host);
devicestate["Name"] = String(mqtt_topic);
devicestate["ServoPos"] = ServoPos; 
devicestate["cmnd"] = msgpayload;  
devicestate["POWER"] = msgcommand; 
devicestate["SPEED"] = String(blinds_speed);  
devicestate["HAState"] = String(HA_Blind_State); 
devicestate["TILT"] = String(TiltPos);  
devicestate["PWM1"] = String(TiltPos); 
devicestate["STATE"] = Blind_STATE;

/////save  software status.dat file///
    //File configFile = SPIFFS.open("/devicestate.dat", "w");    ////old system
    File configFile = LittleFS.open("/devicestate.dat", "w");
    if (!configFile)
    {
    }

    ///devicestate.printTo(Serial);   ///JSON5
    ////devicestate.printTo(configFile);    ///JSON5

    serializeJson(devicestate, Serial);   ////JSON6
    serializeJson(devicestate, configFile);    /////JSON6




    configFile.close();
  ////detach all servos
myservo[0].detach();
myservo[1].detach();


}

////////////////////////////////////////////
boolean isValidNumber(String str)
{
  isNum = false;
  if (!(str.charAt(0) == '+' || str.charAt(0) == '-' || isDigit(str.charAt(0)))) return false;

  for (byte i = 1; i < str.length(); i++)
  {
    if (!(isDigit(str.charAt(i)) || str.charAt(i) == '.')) return false;
  }
  return true;
}

/////////////////////////////////////////////
void tele_update() {
    
  int tele_period = String(tele_update_set).toInt() * 1000; //time converted to millseconds
     
    if ((millis() > time_now + tele_period) || (reboot==1)) {

    

////start new json coding
  
  StaticJsonDocument<512> rootstate;
  
  rootstate["Device"] = mqttDeviceID;
  rootstate["Name"] = String(mqtt_topic);
  rootstate["ServoPos"] = ServoPos;
  rootstate["POWER"] = msgcommand;
  rootstate["HAState"] = HA_Blind_State;
  rootstate["TILT"] = TiltPos;
  rootstate["STATE"] = Blind_STATE;
  rootstate["cmnd"] = String(msgpayload);
  rootstate["SPEED"] = String(blinds_speed); 
  rootstate["BattVoltage"] = Battery_Voltage;
  rootstate["ChargeCapacity"] = Battery_Cap;
  rootstate["DischargeTime"] = Discharge_Time;
  rootstate["RemainingTime"] = Remaining_Time;
  rootstate["PWM1"] = TiltPos.toInt();

   JsonObject Wifi = rootstate.createNestedObject("Wifi");   ////JSON6
  
  Wifi["AP"] = String(ssidAP);
  Wifi["SSId"] = String(WiFi.SSID());  
  Wifi["MAC"] = WiFi.macAddress();
  Wifi["Channel"] = WiFi.channel();
  Wifi["RSSI"] = dBmtoPercentage(WiFi.RSSI());
  Wifi["Signal"] = WiFi.RSSI();
  Wifi["IPAddress"] = WiFi.localIP().toString();
//////end json encoding

        
    time_now = millis();

    size_t lenstate = measureJson(rootstate);  ///JSON6
   // size_t lenstate = rootstate.measureLength();   ///JSON5
    size_t size = lenstate + 1;
    char JSONmessageBufferstate[size];
    serializeJson(rootstate, JSONmessageBufferstate, size);   //JSON6
    //rootstate.printTo(JSONmessageBufferstate, size);   ///JSON5
    
///////////get offset value based on open and lsosed limits and state of blind ie opened or closed
  //////////////////////////////////////////
  
  publish_state();
  
   
client.publish("tele/" + _identifier + "/STATE", JSONmessageBufferstate);////full telementry only


SSDP.begin();
  
    }

////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////

//////////////////////////////////////////////
void connect() {
  String STRING_LWT_TOPIC = "tele/" + _identifier + "/LWT";
  const char* MQTT_LWT_TOPIC = STRING_LWT_TOPIC.c_str();
  String STRING_POWER_TOPIC = "cmnd/" + _identifier + "/POWER";
  POWER_TOPIC = STRING_POWER_TOPIC.c_str();

  const char* MQTT_LWT_MESSAGE = "Online";
  client.setWill(MQTT_LWT_TOPIC, "Offline", 0, true);  //Set LWT state
  //boolean connect (clientID, username, password, willTopic, willQoS, willRetain, willMessage, cleanSession)
WiFi.hostname(host);
WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }



  //If authentication true then connect with username and password
  if (String(mqtt_isAuthentication).equalsIgnoreCase("TRUE")) {

    while (!client.connect(mqttDeviceID, mqtt_username, mqtt_password)) {
      delay(1000);
    }
  }
  else
  {
    while (!client.connect(mqttDeviceID)) {
      delay(1000);
    }
  }

  int len = strlen(MQTT_LWT_MESSAGE) + 1;
  
  client.publish(MQTT_LWT_TOPIC, MQTT_LWT_MESSAGE, len, true);
  client.subscribe("cmnd/" + _identifier + "/#");
  
  client.subscribe("cmnd/tasmotas/#");   //allows for device reset, firmare upgrade etc commands via MQTT
  Serial.print("cmnd/tasmotas/#");
  if (String(auto_discovery).equalsIgnoreCase("ENABLED-BASIC") || String(auto_discovery).equalsIgnoreCase("ENABLED-TILT"))
  {
     client.subscribe("homeassistant/status");
     
  };
///////////////////////////////////////////////////////
 
 
//get_save_state(); ///get saved state on reboot servo position message command speed

//moveServo(ServoPos); ///moves servo to set pos as on powerup servo pos defaults to 90 regardless of previous osition

tele_update();    ///publish saved states

publish_state();   ///publish states to MQTT


}

///end connection to WiFi and MQTT

/////Process message received

void messageReceived(String &topic, String &payload)
{

  bypassdevstat = 0;  ////allow for bypass servo read when disconnested if messages are non servo movement.... e.h. HA activationand firmware update and device stat request
  msgString = payload.c_str();
  msgpayload = msgString;
  
  String blindstrim = blinds_trim_adjust;  //new varible for open state adjustment

///////Home Assistant Auto discovery
    
  if ((topic == "homeassistant/status"||topic == "cmnd/" + _identifier + "/Config") && (msgString == "online"||msgString == "Set"))
  {
    bypassdevstat = 1;
    HA_Discovery();   ////JSON GENERATED
    
    
    delay(1000);   ///wait 1 second to publish and settle

  }
    
  else if ((topic == "cmnd/tasmotas/Restart" || topic == "cmnd/" + _identifier + "/Restart") && msgString.toInt() == 1)
  {
    
    Serial.print("tele/" + String(mqtt_topic) + "/RESTART"+ " ACTIVATED");
    ESP.reset();
  }


  else if ((topic == "cmnd/tasmotas/Upgrade" || topic == "cmnd/" + _identifier + "/Upgrade") && msgString.toInt() == 1)
  {
    Serial.print("tele/" + _identifier + "/UPGRADE"+ " ACTIVATED");
    ESPhttpUpdate.rebootOnUpdate(true);  ///auto restart after firmware downloaded
    bypassdevstat = 1;
    OTAUpgrade();
  }

/////make Status STATUS
  //////////Device Status
  else if ((topic == "cmnd/tasmotas/STATUS" ||  topic == "cmnd" + _identifier + "/STATUS") && msgString.toInt() == 2)
  {
    //Serial.print("stat/" + _identifier + "/STATUS2"+ " GENERATED");
    bypassdevstat = 1;
        
    StaticJsonDocument<512> root;
    
    JsonObject StatusFWR = root.createNestedObject("StatusFWR");   ////JSON6
    StatusFWR["Hostname"] = String(host);
    StatusFWR["Name"] = String(mqtt_topic);
    StatusFWR["Version"] = software_version;
    StatusFWR["Software"] = software_name;
    StatusFWR["Variant"] = software_variant;
    StatusFWR["Speed"] = blinds_speed;
    StatusFWR["Swing"] = blinds_swing_direction;
    StatusFWR["Installed"] = blinds_servo_install;
    StatusFWR["Slip"] = blinds_slip_correction;
    StatusFWR["Trim"] = blinds_trim_adjust;


    //root.printTo(Serial);
    size_t len = measureJson(root);    //json6
    //size_t len = root.measureLength();     ///json5

    size_t size = len + 1;
    char JSONmessageBuffer[size];

    serializeJson(root, JSONmessageBuffer, size);
    //root.printTo(JSONmessageBuffer, size);    ////json5

    //client.publish("stat/" + String(mqtt_topic) + "/MessageSize", String(len));  ///enable for testing
    client.publish("stat/" + _identifier + "/STATUS2", JSONmessageBuffer);
    //Serial.print("stat/" + _identifier + "/STATUS2 "+ JSONmessageBuffer);

  }

///////
////make Status STATUS 5
  //////////Device Status
  else if ((topic == "cmnd/tasmotas/STATUS" ||  topic == "cmnd" + _identifier + "/STATUS") && msgString.toInt() == 5)
  {
    bypassdevstat = 1;
    StaticJsonDocument<512> root;
    
    JsonObject StatusNET = root.createNestedObject("StatusNET");   ////JSON6

    StatusNET["Hostname"] = String(host);
    StatusNET["Name"] = String(mqtt_topic);
    StatusNET["IPAddress"] = WiFi.localIP().toString();
    StatusNET["Gateway"] = WiFi.gatewayIP().toString();
    StatusNET["Subnetmask"] = WiFi.subnetMask().toString();
    StatusNET["DNSServer"] = WiFi.dnsIP().toString();
    StatusNET["Mac"] = WiFi.macAddress();
    StatusNET["Webserver"] = 2;
    StatusNET["WifiConfig"] = WiFi.channel();
    StatusNET["WifiPower"] = dBmtoPercentage(WiFi.RSSI());
    
    //root.printTo(Serial);
    size_t len = measureJson(root);   ////json6
    //size_t len = root.measureLength();   ////json5

    size_t size = len + 1;
    char JSONmessageBuffer[size];

    serializeJson(root, JSONmessageBuffer, size);  /////json6
    //root.printTo(JSONmessageBuffer, size);  //////json5

    //client.publish("stat/" + String(mqtt_topic) + "/MessageSize", String(len));  ///enable for testing
    client.publish("stat/" + _identifier + "/STATUS5", JSONmessageBuffer);
    

  }


///////

  //////Device power command//Process non numeric commands eg up, down, on, off also convert command to numeric for Trim offset etc
//////////////////////
//SPEED Set
 else if (topic == "cmnd/" + _identifier + "/SPEED")
  //else if (payload == "OPEN" && topic == "cmnd/" + String(mqtt_topic) + "/POWER")
  
  {
    //msgString = "0"; //allows for trim adj
    
    if (String(msgString).equalsIgnoreCase("SLOW"))
        {
        
        strcpy(blinds_speed, "SLOW");
        //client.publish("tele/" + String(mqtt_topic) + "/SPEED_NEW", String(blinds_speed));  //testing
        }
         else
         {
        
         strcpy(blinds_speed, "FAST");
         //client.publish("tele/" + String(mqtt_topic) + "/SPEED_CURRENT", String(blinds_speed));  //testing
         
         }
        client.publish("stat/" + _identifier + "/SPEED", String(blinds_speed));  //testing 
        bypassdevstat = 1;
  }
  
  ////////////////////////////////////////////


 //////////////////////
//if (msgString.equalsIgnoreCase("OPEN"))   OPEN OFF and 0
 else if ((String(msgString).equalsIgnoreCase("OPEN") || String(msgString).equalsIgnoreCase("OFF") || String(msgString).equalsIgnoreCase("0")) && (topic == "cmnd/" + _identifier + "/POWER"))
    
  {
   //client.publish("stat/" + _identifier + "/COMMAND", "OPEN,0,OFF PROCESSED" ); //publish position status must be nurmic
    msgString = "0"; //allows for trim adj 
    TiltPos = "0";
    client.publish("stat/" + _identifier + "/HA_STATE", "OPENING"); 
    //HA_Blind_State = "open";
     client.publish("stat/" + _identifier + "/position", "0" ); //publish position status must be nurmic
    if (invert_command == true)
    //if (String(blinds_invert_command).equalsIgnoreCase("YES"))
      {   
      
      moveServo(String(open_limit_set).toInt());  ///old method
      }
    else
      {
      
     moveServo(String(open_limit_set).toInt());  ///new method
      }
  }
  ////////////////////////////////////////////

  //if (msgString.equalsIgnoreCase("CLOSE")) or ON or 1
  else if ((String(msgString).equalsIgnoreCase("CLOSE") || String(msgString).equalsIgnoreCase("ON") || String(msgString).equalsIgnoreCase("1") || String(msgString).equalsIgnoreCase("100")) && (topic == "cmnd/" + _identifier + "/POWER"))
  {
    //client.publish("stat/" + _identifier + "/COMMAND", "CLOSE,1,100,ON PROCESSED" );
    msgString = "100";
    TiltPos = "100";
    //HA_Blind_State = "closed";
    client.publish("stat/" + _identifier + "/HA_STATE", "CLOSING"); 
    client.publish("stat/" + _identifier + "/position", "100" ); //publish position status
    if (invert_command == true)
    //if (String(blinds_invert_command).equalsIgnoreCase("YES"))
      {
      
      
       moveServo(String(close_limit_set).toInt());  ////new method
      }
      else
      {
       
      moveServo(String(close_limit_set).toInt());  ////new method
      }
      
  }

  //if (msgString.equalsIgnoreCase("STOP"))
  else if (String(msgString).equalsIgnoreCase("STOP") && topic == "cmnd/" + _identifier + "/POWER")
  {
    
    //myservo[0].detach();
    //myservo[1].detach();
    
  }
 
  //All other numeric values go here Note No Trim will be applied to these settings
  //If the Blind Needs to be in the 50 percent range opened , then need to open blind first then set to new position
  //This is due to mechanical slip of blind shaft mechinisim
  
  
  else if ((topic == "cmnd/" + _identifier + "/POWER" || topic == "cmnd/" + _identifier + "/tilt") &&  (isValidNumber(msgString)))
  {
    TiltPos = msgString;
    //HA_Blind_State = "open";
    //client.publish("stat/" + _identifier + "/COMMAND", "NURMERIC RANGE PROCESSED" );
    if (invert_command == true)
    //if (String(blinds_invert_command).equalsIgnoreCase("YES"))
     {
      
      //client.publish(String(mqtt_topic) + "/Other Settings Applied inverted");
      if (msgString.toInt() <= 60 && msgString.toInt() >= 40 && String(blinds_slip_correction).equalsIgnoreCase("ON"))
      {
        //client.publish("stat/" + _identifier + "/ALLIGNING", "INVERTED" );
        if (ServoPos/1.8 <= 60 && ServoPos/1.8 >=40)
        {
          //client.publish("stat/" + _identifier + "/ALLIGNING", "INVERTED" );
        }
         else{
           moveServo(180); //Reset Allignment to closed state to obtain correct positioning if around 1/2 range
         }      

      }
      //actual move after alignment
     // client.publish("stat/" + _identifier + "/MOVE-INVERTED", "MOVING" );
      moveServo((100 - msgString.toInt()) * 1.8);
    }


    else
    {

      if (msgString.toInt() <= 60 && msgString.toInt() >= 40 && String(blinds_slip_correction).equalsIgnoreCase("ON"))
      {
        if (ServoPos/1.8 <= 60 && ServoPos/1.8 >= 40)
            {
              //client.publish("stat/" + _identifier + "/ALLIGNING", "INVERTED" );
            }
            else{
               moveServo(0); //Reset Allignment to open state to obtain correct positioning if around 1/2 range
            }
        
        //moveServo(180); //Reset Allignment to open state to obtain correct positioning if around 1/2 range
       //client.publish("stat/" + _identifier + "/ALLIGNING", "NON-INVERTED" );
        //moveServo(0); //Reset Allignment to open state to obtain correct positioning if around 1/2 range
      }
      //Actual move after alignment
      //client.publish(String(mqtt_topic) + "/Other Settings Applied NOT inverted"); //can be removed Testing Debug
      //client.publish("stat/" + _identifier + "/MOVE-NONINVERTED", "MOVING" );
      moveServo(msgString.toInt() * 1.8); //Move the Servo to  other position
    }
  }

  //Publish sevo blind state position before Trim adjustment is applied...this keeps Icon States true no fudge factor
  //RollerShutter Icon position and 3 Button choice OPEN ,HALF, CLOSE and states Remain TRUE indication

  msgcommand = msgString.toInt();
 ////////////////////////////////////
 //////publish states etc////////////////////////////
///////////////////////////////////////////////to be re programmed////////////////////////////////////////
  if (invert_state == true)
  //if (String(blinds_invert_state).equalsIgnoreCase("YES"))
  {
  //client.publish("stat/" + _identifier + "/STATE", String((int)(100 - (myservo.read() / 1.8) + 0.5))); //publish before trim applied
  
  }
  else
  {  
  //client.publish("stat/" + _identifier + "/STATE", String((int)((myservo.read()) + 0.5))); //publish before trim applied
  
  }

  
  ///New code for Open Only Trim Adjustment after publishing state back to mqtt server
  //////begin if statement
  /////////////////////////////////////////////////////////////////////////////////
  if (blindstrim.toInt() > 0 && blindstrim.toInt() <= 75 && msgString.toInt() == 0)
  {
    client.publish(String(mqtt_topic) + "/Executing Trim Adjustment of:-  " + String(blindstrim) + " Percent");
    if (invert_command == true)
    
     {
       moveServo((100 - blindstrim.toInt()) * 1.8);
    }
    else
    {
     moveServo(blindstrim.toInt() * 1.8);
     }
  }
////////////////////////////////////////////////////////////////////////////////////////////////


if (bypassdevstat == 0)
  {
    HA_State();  ////Create HA_Blind_State
    process_state(); ///process state 
    publish_state();   ///publish states to MQTT
    save_state(); ///save data to SPIFF File
  }
}

/////////////////////////////////////
void File_Manager(){
  SendHTML_Header();
  //webpage += "<tr><td>"+String(software_version)+"</td>";
  webpage += F("<h3 class='rcorners_m'>File Manager</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("</table>");
  webpage += F("<a href='/download'><button>Download</button></a>");
  webpage += F("<a href='/upload'><button>Upload</button></a>");
  webpage += F("<a href='/delete'><button>Delete</button></a>");
  webpage += F("<a href='/dir'><button>Directory</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}


//~///////~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Firmware_Update(){
  ESPhttpUpdate.rebootOnUpdate(false); ///prevents reboot on AUTO upgrade selection user must restart manually
  SendHTML_Header();
  
  webpage += F("<h3 class='rcorners_m'>Firmware Updater</h3><br>");
  webpage += F("<table align='center'>");
  
  webpage += F("<h3> MANUAL - Upload Firmware from local folder or directory. User selects file </h3>");
  webpage += F("<h3> AUTO - Upload Firmware from OTA Server as set it SETUP 'OTAAuto path' </h3>");
  webpage += F("<h3> CHECK - Check Resportory Server for latest Firmware release, and advises of upgrade and impact advice</h3>");
  webpage += F("<table align='center'>");
  webpage += F("</table>");
  webpage += F("<a href='/firmware'><button>Manual</button></a>");
  webpage += F("<a href='/firmwareauto'><button>Auto</button></a>");
  webpage += F("<a href='/firmwarecheck'><button>Check</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Device_Reboot(){
  SendHTML_Header();
   
  webpage += F("<h3 class='rcorners_m'>Restart Controller</h3><br>");
  webpage += F("<table align='center'>");
  //webpage += F("<h3> Device will AUTO Restart In 5 Seconds</h3>");
  webpage += F("<h3> Blind Will Jolt After Restart </h3>");
  webpage += F("<h3> Wait 10 seconds before pressing back </h3>");
  webpage += F("<h3> After Restart Press BACK to return to Home</h3>");
  webpage += F("<table align='center'>");
  webpage += F("</table>");
  webpage += F("<a href='/'><button>Back</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
//delay(5000);
ESP.restart();
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Set_Servo(){
  SendHTML_Header();
   
  webpage += F("<h3 class='rcorners_m'>Aligning Servo Motor</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("<h3> Servo Motor Positioned To OPEN State For Install </h3>");
  webpage += F("<h3> Please Disconnect Power From Device within 20 Seconds...' </h3>");
  webpage += F("<h3> Postion blind to open zero plane then connect' </h3>");
  webpage += F("<h3> Servo to Blind Shaft And Power Back Up</h3>");
  webpage += F("<h3> After Restart Press BACK to return to Home</h3>");
  webpage += F("<table align='center'>");
  webpage += F("</table>");
  //webpage += F("<a href='/'>[BACK]</a><br><br>");
  webpage += F("<a href='/'><button>Back</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent

if (invert_command == true)
//if (String(blinds_invert_command).equalsIgnoreCase("YES"))
   {
      moveServo(180);
      moveServo(180); //command to move 3 plus times to push servo motor, noticed some difference with just one command
      moveServo(180);
      moveServo(180);
  //    httpServer.send(201, "text/plain", "Servo Motor Positioned To OPEN State For Install,  Please Disconnect Power From Device NOW...Before Device AUTO Restarts In 20Seconds ");
    } 
    else
    {
      moveServo(0);
      moveServo(0);  //command to move 3 plus times to push servo motor, noticed some difference with just one command
      moveServo(0);
      moveServo(0);
  //    httpServer.send(201, "text/plain", "Servo Motor Positioned To OPEN State For Install,  Please Disconnect Power From Device NOW...Before Device AUTO Restarts In 20 Seconds ");

    }
////////////


delay(20000);
    ESP.restart();
}
 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Config_Setup(){
// <input type="text" ng-model="inputText" placeholder="{{somePlaceholder}}" />
  SendHTML_Header();
  
  //webpage += F("<FORM action='/' method='post' enctype='multipart/form-data'>");
  webpage += F("<FORM action='/submitconfig' method='post' enctype='multipart/form-data'>");
  webpage += F("<h3 class='rcorners_m'>Configuration Setup</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("<tr><th>Name</th><th style='width:50%'>Set Value</th><th>New Value</th></tr>");
webpage += "<tr><td>DHCP/Network Device Name</td><td>"+String(host)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_host' placeholder = 'BlindControl1'></td></tr>");
webpage += "<tr><td>Device Friendly Name</td><td>"+String(mqtt_topic)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_mqtt_topic'  placeholder = 'Kitchen Blind'></td></tr>");
webpage += "<tr><td>MQTT Server</td><td>"+String(mqtt_server)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_mqtt_server'  placeholder = '192.168.0.100'></td></tr>");
webpage += "<tr><td>MQTT Port</td><td>"+String(mqtt_port)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_host'  placeholder = '1883'></td></tr>");
webpage += "<tr><td>MQTT User ID</td><td>"+String(mqtt_username)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_mqtt_username'  placeholder = 'openhabian'></td></tr>");
webpage += "<tr><td>MQTT Password</td><td>"+String(mqtt_password)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_mqtt_password'  placeholder = 'password'></td></tr>");
webpage += "<tr><td>MQTT Authentication</td><td>"+String(mqtt_isAuthentication)+"</td></td>"; ///tr
webpage += F("<td><select name='input_mqtt_isAuthentication'><option value=''>         </option><option value='FALSE'>FALSE</option><option value='TRUE'>TRUE</option></select></td></tr>");
webpage += "<tr><td>Admin Password</td><td>"+String(update_password)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_update_password' placeholder = 'password'></td></tr>");
webpage += "<tr><td>OTAAuto path</td><td>"+String(OTAAuto_path)+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_OTAAuto_path' placeholder = 'http://mountaineagle-technologies.com/tasmota/mk-blindcontrol.bin'></td></tr>");
webpage += "<tr><td>Blind Speed</td><td>"+String(blinds_speed)+"</td></td>"; ///tr
webpage += F("<td><select name='input_blinds_speed'><option value=''>         </option><option value='SLOW'>SLOW</option><option value='FAST'>FAST</option></select></td></tr>");
webpage += "<tr><td>Motor Installed Side</td><td>"+String(blinds_servo_install)+"</td></td>"; ///tr
webpage += F("<td><select id='input_blinds_servo_install' name='input_blinds_servo_install'><option value=''>      </option><option value='LEFT'>LEFT</option><option value='RIGHT'>RIGHT</option></select></td></tr>");
webpage += "<tr><td>Swing Direction To Close</td><td>"+String(blinds_swing_direction)+"</td></td>"; ///tr
webpage += F("<td><select name='input_blinds_swing_direction'><option value=''>       </option><option value='UP'>UP</option><option value='DOWN'>DOWN</option></select></td></tr>");
webpage += "<tr><td>Open Limit Default "+String(open_limit_default)+"</td><td>"+String(open_limit_set)+" Degrees"+"</td></td>"; ///tr

//webpage += "<tr><td>Open Limit 0-180</td><td>"+String(open_limit_set)+" Degrees"+"</td></td>"; ///tr

webpage += F("<td><input class='text' style='width:90%' name='input_open_limit_set' placeholder = '0'></td></tr>");
webpage += "<tr><td>Closed Limit Default "+String(close_limit_default)+"</td><td>"+String(close_limit_set)+" Degrees"+"</td></td>"; ///tr
//webpage += "<tr><td>Closed Limit 0-180 </td><td>"+String(close_limit_set)+" Degrees"+"</td></td>"; ///tr
webpage += F("<td><input class='text' style='width:90%' name='input_close_limit_set' placeholder = '180'></td></tr>");


webpage += "<tr><td>Weight Slip Correction</td><td>"+String(blinds_slip_correction)+"</td></td>"; ///tr
webpage += F("<td><select name='input_blinds_slip_correction'><option value=''>        </option><option value='ON'>ON</option><option value='OFF'>OFF</option></select></td></tr>");
 
webpage += "<tr><td>HA Auto Discovery</td><td>"+String(auto_discovery)+"</td></td>"; ///tr
webpage += F("<td><select name='input_auto_discovery'><option value=''>        </option><option value='ENABLED-BASIC'>ENABLED-BASIC</option><option value='ENABLED-TILT'>ENABLED-TILT</option>   <option value='DISABLED'>DISABLED</option></select></td></tr>");
webpage += "<tr><td>Telemetry Period in Sec</td><td>"+String(tele_update_set)+"</td></td>"; ///tr
webpage += "<td><input type='text' style='width:90%' name='input_tele_update_set'  id='input_tele_update_set' placeholder = '60'></td></tr>";
webpage += "<tr><td>Remote Button Connected</td><td>"+String(remote_switch)+"</td></td>"; ///tr
webpage += F("<td><select name='input_remote_switch'><option value=''>        </option><option value='YES'>YES</option><option value='NO'>NO</option></select></td></tr>");
webpage += "<tr><td>Battery System</td><td>"+String(battery_system)+"</td></td>"; ///tr
webpage += F("<td><select name='input_battery_system'><option value=''>        </option><option value='ON'>ON</option><option value='OFF'>OFF</option></select></td></tr>");



if (String(battery_system).equalsIgnoreCase("ON"))
             {
              webpage += "<tr><td>Battery Capacity mAh</td><td>"+String(battery_capacity)+"</td></td>"; ///tr
              webpage += "<td><input type='text' style='width:90%' name='input_battery_capacity'  id='input_battery_capacity' placeholder = '3600'></td></tr>";
              webpage += "<tr><td>System Power Watts</td><td>"+String(system_power)+"</td></td>"; ///tr
              webpage += "<td><input type='text' style='width:90%' name='input_system_power'  id='input_system_power' placeholder = '10'></td></tr>";
              webpage += "<tr><td>Battery check in Sec</td><td>"+String(tele_battery_set)+"</td></td>"; ///tr
              webpage += "<td><input type='text' style='width:90%' name='input_tele_battery_set'  id='input_tele_battery_set' placeholder = '10'></td></tr>";
               }

webpage += F("</table>");

  webpage += F("<button type='submit' formaction='/submitconfig' method='post' enctype='multipart/form-data'>Submit</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Help(){
  SendHTML_Header();
   
  webpage += F("<h3 class='rcorners_m'>Home</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("</table>");
  append_page_footer();//
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage(){
if (myservo[0].attached()) {  // If the servo is attached
    myservo[0].detach();  // detach the servo motor
    myservo[1].detach();
  }

//myservo[0].detach();
//myservo[1].detach();
  SendHTML_Header();
  
  
   webpage += F("<h3 class='rcorners_m'>Status</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("<tr><th>Name</th><th style='width:50%'>Set Value</th></tr>");
 webpage += "<tr><td>Firmware</td><td>"+String(firmware_installed)+"</td></tr>";
 webpage += "<tr><td>DHCP/Network Device Name</td><td>"+String(host)+"</td></tr>";
  webpage += "<tr><td>Device Friendly Name</td><td>"+String(mqtt_topic)+"</td></tr>";   ////friendly name 
  webpage += "<tr><td>Devive Unique Name</td><td>"+String(_identifier)+"</td></tr>";
  
  webpage += "<tr><td>SSID</td><td>"+String(WiFi.SSID())+"</td></tr>";
  webpage += "<tr><td>IP Address</td><td>"+WiFi.localIP().toString()+"</td></tr>";
  webpage += "<tr><td>RSSI </td><td>"+String(dBmtoPercentage(WiFi.RSSI()))+"</td></tr>";
  webpage += "<tr><td>Signal Strengrh</td><td>"+String(WiFi.RSSI())+"</td></tr>";
  webpage += "<tr><td>Servo Position</td><td>"+String(ServoPos)+"</td></tr>";
  webpage += "<tr><td>Blind STATE</td><td>"+Blind_STATE+"</td></tr>";
  webpage += "<tr><td>Friendly Blind STATE</td><td>"+HA_Blind_State+"</td></tr>";
  webpage += "<tr><td>Blind Tilt Position</td><td>"+TiltPos+"</td></tr>";  
  webpage += "<tr><td>MQTT Server</td><td>"+String(mqtt_server)+"</td></tr>";
  webpage += "<tr><td>MQTT Command Topic</td><td>cmnd/"+_identifier+"/POWER</td></tr>";
  webpage += "<tr><td>MQTT Status Topic</td><td>stat/"+_identifier+"/STATE</td></tr>";

//if (String(auto_discovery).equalsIgnoreCase("ENABLED-TILT"))
 //            {
              webpage += "<tr><td>MQTT Tilt Command Topic</td><td>cmnd/"+_identifier+"/tilt</td></tr>";
            webpage += "<tr><td>MQTT Tilt Status Topic</td><td>stat/"+_identifier+"/tilt-state</td></tr>";
                   
 //           } 

  webpage += "<tr><td>Blind Speed</td><td>"+String(blinds_speed)+"</td></tr>";
  webpage += "<tr><td>Servo Motor Installed Side</td><td>"+String(blinds_servo_install)+"</td></tr>";
  webpage += "<tr><td>Blinds Swing Direction To Close</td><td>"+String(blinds_swing_direction)+"</td></tr>";
  //webpage += "<tr><td>Blinds Trim Adjustment Open State %</td><td>"+String(blinds_trim_adjust)+"</td></tr>";
  webpage += "<tr><td>Blinds Weight Slip Correction</td><td>"+String(blinds_slip_correction)+"</td></tr>";
 webpage += "<tr><td>HA Auto Discovery</td><td>"+String(auto_discovery)+"</td></tr>";
  webpage += "<tr><td>System Telemetry Period</td><td>"+String(tele_update_set)+" seconds</td></tr>";
  webpage += "<tr><td>Remote Button Connected</td><td>"+String(remote_switch)+"</td></tr>";
  webpage += "<tr><td>Default Open Limit </td><td>"+String(open_limit_default)+"</td></tr>"; 
  webpage += "<tr><td>User Set Open Limit</td><td>"+String(open_limit_set)+"</td></tr>";
  webpage += "<tr><td>Default Closed Limit</td><td>"+String(close_limit_default)+"</td></tr>";
  webpage += "<tr><td>User Set Closed Limit</td><td>"+String(close_limit_set)+"</td></tr>";
  webpage += "<tr><td>Battery System</td><td>"+String(battery_system)+"</td></tr>";
  if (String(battery_system).equalsIgnoreCase("ON"))
             {
              webpage += "<tr><td>Battery Voltage</td><td>"+String(Battery_Voltage)+" Volts</td></tr>";
              webpage += "<tr><td>Charge Capacity</td><td>"+String(Battery_Cap)+" %</td></tr>";
               webpage += "<tr><td>Discharge Time</td><td>"+String(Discharge_Time)+" Minutes</td></tr>";
                webpage += "<tr><td>Remaining Time</td><td>"+String(Remaining_Time)+" Minutes</td></tr>"; 
               webpage += "<tr><td>Battery Check Period</td><td>"+String(tele_battery_set)+" Seconds</td></tr>"; 
                   
          } 
  
 ////
webpage += F("</table>");
    SendHTML_Content();
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   //Stop is needed because no content length was sent


}
//~~~~~~~~


void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  if (httpServer.args() > 0 ) { // Arguments were received
    if (httpServer.hasArg("download")) DownloadFile(httpServer.arg(0));
  }
  else SelectInput("Enter filename to download","download","download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File UploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& uploadfile = httpServer.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    ///SPIFFS.remove(filename);                  // Remove a previous version, otherwise data is appended the file again   old system
    LittleFS.remove(filename);                  // Remove a previous version, otherwise data is appended the file again
    ///UploadFile = SPIFFS.open(filename, "w");  // Open the file for writing in SPIFFS (create it, if doesn't exist)   ////old system
    UploadFile = LittleFS.open(filename, "w");  // Open the file for writing in SPIFFS (create it, if doesn't exist)
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)          // If the file was successfully created
    {                                    
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>"; 
      webpage += F("<a href='/'>[Back]</a><br><br>");
      append_page_footer();
      httpServer.send(200,"text/html",webpage);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef ESP32
void SPIFFS_dir(){ 
  if (SPIFFS_present) {
    ///File root = SPIFFS.open("/"); /////old system
    File root = LittleFS.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header();
      webpage += F("<h3 class='rcorners_m'>SPIFFS Contents</h3><br>");
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Name/Type</th><th style='width:20%'>Type File/Dir</th><th>File Size</th></tr>");
      printDirectory("/",0);
      webpage += F("</table>");
      SendHTML_Content();
      root.close();
    }
    else 
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  } else ReportSPIFFSNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printDirectory(const char * dirname, uint8_t levels){
  ///File root = SPIFFS.open(dirname);    /////old system
  File root = LittleFS.open(dirname);
  if(!root){
    return;
  }
  if(!root.isDirectory()){
    return;
  }
  File file = root.openNextFile();
  while(file){
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if(file.isDirectory()){
      webpage += "<tr><td>"+String(file.isDirectory()?"Dir":"File")+"</td><td>"+String(file.name())+"</td><td></td></tr>";
      printDirectory(file.name(), levels-1);
    }
    else
    {
      webpage += "<tr><td>"+String(file.name())+"</td>";
      webpage += "<td>"+String(file.isDirectory()?"Dir":"File")+"</td>";
      int bytes = file.size();
      String fsize = "";
      if (bytes < 1024)                     fsize = String(bytes)+" B";
      else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
      else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
      else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
      webpage += "<td>"+fsize+"</td></tr>";
    }
    file = root.openNextFile();
  }
  file.close();
}
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef ESP8266
void SPIFFS_dir(){
  String str;
  if (SPIFFS_present) { 
    ///Dir dir = SPIFFS.openDir("/");    ////old system
    Dir dir = LittleFS.openDir("/");
    
    SendHTML_Header();
    webpage += F("<h3 class='rcorners_m'>SPIFFS Contents</h3><br>");
    webpage += F("<table align='center'>");
    webpage += F("<tr><th>Name/Type</th><th style='width:40%'>File Size</th></tr>");
    while (dir.next()) {
      Serial.print(dir.fileName());
      webpage += "<tr><td>"+String(dir.fileName())+"</td>";
      str  = dir.fileName();
      str += " / ";
      if(dir.fileSize()) {
        File f = dir.openFile("r");
        Serial.println(f.size());
        int bytes = f.size();
        String fsize = "";
        if (bytes < 1024)                     fsize = String(bytes)+" B";
        else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
        else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
        else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
        webpage += "<td>"+fsize+"</td></tr>";
        f.close();
      }
      str += String(dir.fileSize());
      str += "\r\n";
      Serial.println(str);
    }
    webpage += F("</table>");


    ////////////////////
  webpage += F("<a href='/download'><button>Download</button></a>");
  webpage += F("<a href='/upload'><button>Upload</button></a>");
  webpage += F("<a href='/delete'><button>Delete</button></a>");
  webpage += F("<a href='/dir'><button>Directory</button></a>");
    
    /////////////////////
    SendHTML_Content();
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  } else ReportSPIFFSNotPresent();
}
#endif

///---------------------------------------------------------------------------
void Submit_Config() { // submit config for changes
     ////copy update to current variables
 ///////////////// 
 
 SendHTML_Header();
if (httpServer.arg("input_host")!= ""){    
  strcpy(host, httpServer.arg("input_host").c_str());
  reboot = 1;
  }
   if (httpServer.arg("input_mqtt_topic")!= ""){    
  strcpy(mqtt_topic, httpServer.arg("input_mqtt_topic").c_str());
  reboot = 1;
  } 
   
   if (httpServer.arg("input_mqtt_server")!= ""){    
  strcpy(mqtt_server, httpServer.arg("input_mqtt_server").c_str());
  reboot = 1;
  }
 if (httpServer.arg("input_mqtt_port")!= ""){    
  strcpy(mqtt_port, httpServer.arg("input_mqtt_port").c_str());
  reboot = 1;
  }
 if (httpServer.arg("input_mqtt_username")!= ""){    
  strcpy(mqtt_username, httpServer.arg("input_mqtt_username").c_str());
  reboot = 1;
  }
 if (httpServer.arg("input_mqtt_password")!= ""){    
  strcpy(mqtt_password, httpServer.arg("input_mqtt_password").c_str());
  reboot = 1;
  }
 if (httpServer.arg("input_mqtt_isAuthentication")!= ""){    
  strcpy(mqtt_isAuthentication, httpServer.arg("input_mqtt_isAuthentication").c_str());
  }
  if (httpServer.arg("input_update_password")!= ""){    
  strcpy(update_password, httpServer.arg("input_update_password").c_str());
  }
 if (httpServer.arg("input_OTAAuto_path")!= ""){    
  strcpy(OTAAuto_path, httpServer.arg("input_OTAAuto_path").c_str());
  }
 if (httpServer.arg("input_blinds_speed")!= ""){    
  strcpy(blinds_speed, httpServer.arg("input_blinds_speed").c_str());
  }
 if (httpServer.arg("input_blinds_servo_install")!= ""){    
  strcpy(blinds_servo_install, httpServer.arg("input_blinds_servo_install").c_str());
  reboot = 1;
  strcpy(close_limit_set, "");
  strcpy(open_limit_set, "");
  }
 if (httpServer.arg("input_blinds_swing_direction")!= ""){    
  strcpy(blinds_swing_direction, httpServer.arg("input_blinds_swing_direction").c_str());
  reboot = 1;
  strcpy(close_limit_set, "");
  strcpy(open_limit_set, "");
  }
 //if (httpServer.arg("input_blinds_trim_adjust")!= ""){    
  //strcpy(blinds_trim_adjust, httpServer.arg("input_blinds_trim_adjust").c_str());
 // }
 if (httpServer.arg("input_blinds_slip_correction")!= ""){    
  strcpy(blinds_slip_correction, httpServer.arg("input_blinds_slip_correction").c_str());
  }
 if (httpServer.arg("input_tele_update_set")!= ""){    
  strcpy(tele_update_set, httpServer.arg("input_tele_update_set").c_str());
  }
  
  if (httpServer.arg("input_battery_system")!= ""){    
  strcpy(battery_system, httpServer.arg("input_battery_system").c_str());
  }
  
  if (httpServer.arg("input_remote_switch")!= ""){    
  strcpy(remote_switch, httpServer.arg("input_remote_switch").c_str());
  }
  
  if (httpServer.arg("input_battery_capacity")!= ""){    
  strcpy(battery_capacity, httpServer.arg("input_battery_capacity").c_str());
  }

  if (httpServer.arg("input_system_power")!= ""){    
  strcpy(system_power, httpServer.arg("input_system_power").c_str());
  }
  if (httpServer.arg("input_tele_battery_set")!= ""){    
  strcpy(tele_battery_set, httpServer.arg("input_tele_battery_set").c_str());
  }

   if (httpServer.arg("input_auto_discovery")!= ""){    
  strcpy(auto_discovery, httpServer.arg("input_auto_discovery").c_str());
  reboot = 1;
  }

  if (httpServer.arg("input_open_limit_set")!= ""){    
  strcpy(open_limit_set, httpServer.arg("input_open_limit_set").c_str());
 }

  if (httpServer.arg("input_close_limit_set")!= ""){    
  strcpy(close_limit_set, httpServer.arg("input_close_limit_set").c_str());
  }
      
      webpage += "<h3>Changes Have Been Submitted BUT not saved</h3>"; 
      //webpage += F("<a href='/setup'>[Back]</a><br><br>");
     webpage += F("<a href='/'><button>Cancel</button></a>");
     webpage += F("<a href='/saveconfig'><button>Save</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
 } 

///////////////////////////////////////////////////////////////////////////////////////
void Save_Config() { //save configuration to spiffs
  

    DynamicJsonDocument json(1024);   ////json6

    json["host"] = host;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;   ///device friendly name
    json["mqtt_isAuthentication"] = mqtt_isAuthentication;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["update_username"] = update_username;
    json["update_password"] = update_password;
    json["battery_system"] = battery_system;
    json["remote_switch"] = remote_switch;
    json["auto_discovery"] = auto_discovery;
    json["open_limit_set"] = open_limit_set;
    json["close_limit_set"] = close_limit_set;
    json["battery_capacity"] = battery_capacity;
    json["system_power"] = system_power;
    json["tele_battery_set"] = tele_battery_set;
    json["update_path"] = update_path;
    json["blinds_speed"] = blinds_speed;
    json["blinds_swing_direction"] = blinds_swing_direction;
    json["blinds_servo_install"] = blinds_servo_install;
    json["blinds_trim_adjust"] = blinds_trim_adjust;
    json["blinds_slip_correction"] = blinds_slip_correction;
    json["OTAAuto_path"] = OTAAuto_path;
    json["tele_update_set"] = tele_update_set;

  
  if (SPIFFS_present) { 
   SendHTML_Header();
    
    if (LittleFS.exists("/" + String(software_version) + ".json"))
    {
      
      LittleFS.remove("/" + String(software_version) + ".json");   ///remove existing file before re writting
    }
    
    
    File configFile = LittleFS.open("/" + String(software_version) + ".json", "w");
   
   
    if (configFile)
    {
      
      serializeJson(json, Serial);     ////json6
      serializeJson(json, configFile);    ////json6
      
      configFile.close();
      
      webpage += "<h3>Changes Have Been Saved. </h3>";
        if (String(auto_discovery) != "DISABLED")
      {
        HA_Discovery();
        webpage += "<h3>Auto Discovery Updated. </h3>";
        
      }
        
        if (reboot == 1)
        {
          webpage += "<h3> (Reboot) Required  </h3>";
          webpage += F("<a href='/reboot'><button>Reboot</button></a>");
        }
        
     
       webpage += F("<a href='/'><button>Back</button></a>");
       
      
    } else ReportFileNotPresent("delete");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
    
 } else ReportSPIFFSNotPresent();


} 

////////////////////////button control toggle open close external button on PGM pin nd ground

///Blind Open/close  with trim adjustment enabled
void clickEvent() {
    
   if ((HA_Blind_State == "OPEN") || (HA_Blind_State == "OPENED"))
   {
      
       moveServo(String(close_limit_set).toInt());  ////new method ///
       HA_Blind_State == "CLOSED";
     }
    else
    {
     
     moveServo(String(open_limit_set).toInt());  ////new method
     HA_Blind_State == "OPENED";
    }
    HA_State();
    process_state();
    publish_state();
    save_state();
    
  }
 
///Blind position 50%
void doubleClickEvent() {
    ///code with slip correction added. i.e. if closed go fully open then back to 50%
    //client.publish("stat/"+_identifier+"/ClickEventprocess", "DoublePress");
    HA_Blind_State == "OPEN";
    moveServo(90);
    HA_State();
    process_state();
    publish_state();
    save_state();
  }
   

void holdEvent() {
   
   
    //client.publish("stat/"+_identifier+"/ClickEvent", "ShortHold");
    //if (String(blinds_invert_command).equalsIgnoreCase("YES"))
    //  moveServo(90);
    //else
   //   moveServo(90);
  }
   


///Reset Controller to default and wipe all sata restart in AP mode
void longHoldEvent() {
   //client.publish("stat/"+_identifier+"/ClickEvent", "LongPress");
   SendHTML_Header();
   
  webpage += F("<h3 class='rcorners_m'>Reset Controller</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("<h3> All Settings Have Been Deleted and Set To Defaults</h3>");
  webpage += F("<h3> File System  Erased and Reformated.....</h3>");
  webpage += F("<h3> You Will Need To Connect Via AP Mode And Setup Device Again</h3>");
  webpage += F("<h3> Device Will AUTO Restart In 10 Seconds in AP Setup mode</h3>");
  //webpage += F("<h3> Press Continue to Proceed or Cancel to Exit</h3>");
  webpage += F("<a href='/'><button>Back</button></a>");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
   
     
     WiFiManager wifiManager;
    wifiManager.resetSettings();
    LittleFS.format(); // refomats SFIFFS erases all files clean setup
    delay(10000);
    ESP.restart(); ///better option then ESP.reset()
    
   
}

///
void checkButton() {    
   int event = 0;
   buttonVal = digitalRead(buttonPin);
   // Button pressed down
   if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce)
   {
      
       downTime = millis();
       ignoreUp = false;
       waitForUp = false;
       singleOK = true;
       holdEventPast = false;
       longHoldEventPast = false;
       if ((millis()-upTime) < DCgap && DConUp == false && DCwaiting == true)  DConUp = true;
       else  DConUp = false;
       DCwaiting = false;
   }
   // Button released
   else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce)
   {        
       if (not ignoreUp)
       {
           upTime = millis();
           if (DConUp == false) DCwaiting = true;
           else
           {
                //client.publish("stat/" + _identifier + "/Button_PRESS", "Event2" );
               
               event = 2;
               DConUp = false;
               DCwaiting = false;
               singleOK = false;
           }
       }
   }
   // Test for normal click event: DCgap expired
   if ( buttonVal == HIGH && (millis()-upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && event != 2)
   {
       client.publish("stat/" + _identifier + "/Button_PRESS", "Event1" );
       event = 1;
       DCwaiting = false;
   }
   // Test for hold
   if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
       // Trigger "normal" hold
       if (not holdEventPast)
       {
           //client.publish("stat/" + _identifier + "/Button_PRESS", "Event3" );
           event = 3;
           waitForUp = true;
           ignoreUp = true;
           DConUp = false;
           DCwaiting = false;
           //downTime = millis();
           holdEventPast = true;
       }
       // Trigger "long" hold
       if ((millis() - downTime) >= longHoldTime)
       {
           if (not longHoldEventPast)
           {
               //client.publish("stat/" + _identifier + "/Button_PRESS", "Event4" );
               event = 4;
               longHoldEventPast = true;
           }
       }
   }
   buttonLast = buttonVal;
   button_result = event;
   
}

///////
// Allocate a 1024-byte buffer for the JSON document.
void FirmwareCheck() {

char firmware_release[7] = "Vx.xx";
char firmware_impact[9] = "UNKNOWN";
char firmware_date[12] = "99/99/9999";

HTTPClient http;
 
 DynamicJsonDocument firmware(1024);    //json6
SendHTML_Header();

webpage += F("<h3 class='rcorners_m'>Check Firmware Update</h3><br>");
  webpage += F("<table align='center'>");
  ///webpage += F("<table align='center'>");
 
 

 http.setTimeout(1000);
  http.begin(net, url);
  
 int status = http.GET();
 if (status <= 0) {
   Serial.printf("HTTP error: %s\n", 
       http.errorToString(status).c_str());
   webpage += F("<h3>Can not connect to REPO Update Server</h3>");
   //return;
 }
 String payload = http.getString();
//const String& payload = http.getString();
http.end(); 


 //JsonObject& firmware = jsonBuffer.parseObject(payload);

        
        auto deserializeError = deserializeJson(firmware, payload);
        serializeJson(firmware, Serial);
        
  
 
    if (! deserializeError)    ///json6
   ///if (firmware.success())     ///json5
   {
         
         strcpy(firmware_release, firmware["release"]);
         strcpy(firmware_impact, firmware["impact"]);
         strcpy(firmware_date, firmware["date"]);
         webpage += F("<h3> Firmware found on Repository Server</h3>");
         webpage += F("<table align='center'>");
         
         webpage += F("<tr><th>Installed on Device</th><th style='width:50%'>Version Available </th><th>Date Released</th></tr>");
         webpage += "<tr><td>"+String(firmware_installed)+"</td><td>"+String(firmware_release)+"</td><td>"+String(firmware_date)+"</td></tr>";
         webpage += F("</table>");
         //strcpy(blinds_speed, firmware["SPEED"]);
         ////
          if (firmware_installed==firmware_release)
          {
          webpage += F("<h3> No Update Required, You have latest version installed</h3>");
          }
          else
          {
            webpage += F("<h3>Update Required, Advised to install latest version</h3>");
          }
          if (String(firmware_impact).equalsIgnoreCase("HIGH"))
             {
              webpage += F("<h3> Update has HIGH IMPACT to existing functions, resulting in possible AP configuration, see release notes for impact</h3>");
          } 
            else
          {
              webpage += F("<h3> Update has LOW IMPACT to existing functions, provides bug fixes and enhancements only </h3>");
      
          }
           
        }
        else
        {
        
        }

  
  webpage += F("<a href='/firmware'><button>Manual</button></a>");
  webpage += F("<a href='/firmwareauto'><button>Auto</button></a>");
    webpage += F("<a href='/firmwareupdate'><button>Back</button></a>");
  //webpage += F("<a href='/firmwareupdate'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void Battery_Check(){
 int tele_period = String(tele_battery_set).toInt() * 1000; //time converted to millseconds
 if (millis() > time_now_2 + tele_period) 
 {
 int nVoltageRaw = analogRead(A0);
 float fVoltage = (float)nVoltageRaw * 0.00486; 
 String S_battery_capacity = battery_capacity;
String S_system_power = system_power;

float fVoltageMatrix[22][2] = {
    {4.2,  100},
    {4.15, 95},
    {4.11, 90},
    {4.08, 85},
    {4.02, 80},
    {3.98, 75},
    {3.95, 70},
    {3.91, 65},
    {3.87, 60},
    {3.85, 55},
    {3.84, 50},
    {3.82, 45},
    {3.80, 40},
    {3.79, 35},
    {3.77, 30},
    {3.75, 25},
    {3.73, 20},
    {3.71, 15},
    {3.69, 10},
    {3.61, 5},
    {3.27, 0},
    {0, 0}
  };

  int i, perc;

  perc = 100;

  for(i=20; i>=0; i--) {
    if(fVoltageMatrix[i][0] >= fVoltage) {
      perc = fVoltageMatrix[i + 1][1];
      break;
    }
  }
//Battery_Cap = perc;
Battery_Voltage = fVoltage;


//////test caculatoins
//Battery_Voltage = 3.7;
//S_system_power = "10";
//S_battery_capacity = "1800";
//perc = 50;

///////////////////////////////////////
///for a 1800mAH 3.7v 18650 battery to power a 3.7V 10W digital device, how to calculate the running time?

/////for 3.7V 10W device，working current would be 10÷3.7 = 2.7027A = 2702.7 mA
//////In theory that’s: 1800mAh ÷ 2702.7 mA = 0.666 h = 40 min
//////In reality that’s: 1800mAh ÷ 2702.7 mA    *0.9 = 0.599h = 36 min

////////Quick Notes: 1A=1000mA (mA is current, mAh is Capacity)

//////Or you can use 3.7V*1.8Ah(1800mAh)*0.9/10W=0.599h=36min
///////////////////////////////////////
 // DischargTime=Battery Capacity * Battery Volt*0.9 / Device Watt;

 
/////////////////////////////////////////  


Discharge_Time = (S_battery_capacity.toInt()/ (S_system_power.toFloat()/Battery_Voltage)/1000* 0.9)*60; ///convert to min 
Remaining_Time = Discharge_Time*perc/100;
Battery_Cap = perc;

if (fVoltage <= 3) 
 {Remaining_Time = 0;
Discharge_Time = 0;
 }

//Discharge_Time = ((S_battery_capacity.toInt()/ (S_system_power.toInt()/Battery_Voltage)*1000)* 0.9)*60 ;

StaticJsonDocument<256> ups;

ups["Device"] = String(host);
ups["Name"] = String(mqtt_topic);
ups["SystemPower"] = system_power;
ups["BatterySize"] = battery_capacity;
ups["BattVoltage"] = fVoltage;
ups["AOPINVoltage"] = nVoltageRaw;
ups["ChargeCapacity"] = perc;
ups["DischargeTime"] = Discharge_Time;
ups["RemainingTime"] = Remaining_Time;


size_t len = measureJson(ups);   /////json6

    size_t size = len + 1;
    char JSONmessageBuffer[size];

    
    serializeJson(ups, JSONmessageBuffer, size);    ///json6
    
    //client.publish(_topic, JSONmessageBuffer, size, true);
    client.publish("tele/"+_identifier+"/batt-state", JSONmessageBuffer);
    client.publish("stat/"+_identifier+"/batt-state", JSONmessageBuffer);
time_now_2 = millis();
 } 
}
//////////////////////////////////////////////////////////////~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 




 
  ///////////////////////////////////////////////////////////////////////
void setup()
{
  //client.packetSize(512);
   // Configure extra config vars for Home assistant
  
  ///////////////////////////////////////////////
  Serial.begin(115200);

  pinMode(A0, INPUT);  ////setup for battery monitor
  pinMode(0, INPUT);    // sets the digital pin 13 as output
  //wifi_set_sleep_type(MODEM_SLEEP_T);
  // Serial.print("Checking For Configuration file "); 
  if (LittleFS.begin())
    {
    SPIFFS_present = true; 
    //////check for version 6 if so copy to V7
   configupgrade();    ////check for config file upgrade
    //////
    
    if (LittleFS.exists("/" + String(software_version) + ".json"))
       {
         //file exists, reading and loading
      
        File configFile = LittleFS.open("/" + String(software_version) + ".json", "r");
          if (configFile)
            {
              //Serial.print("Configuraton found....loading values ");
              size_t size = configFile.size();   ///json 5
              ///size_t len = measureJson(configFile); json 6
        
              // Allocate a buffer to store contents of the file.
              std::unique_ptr<char[]> buf(new char[size]);

              configFile.readBytes(buf.get(), size);
              /////////new code JSON6
              DynamicJsonDocument json(1024);
              auto deserializeError = deserializeJson(json, buf.get());
              serializeJson(json, Serial);
              
        //if (json.success())    /////json5
        if ( ! deserializeError )    /////json6

        {
          
          strcpy(host, json["host"]);
          strcpy(update_username, json["update_username"]);
          strcpy(update_password, json["update_password"]);
  
        if (json.containsKey("battery_system")) {strcpy(battery_system, json["battery_system"]);}
        if (json.containsKey("battery_capacity")) {strcpy(battery_capacity, json["battery_capacity"]);}
        if (json.containsKey("system_power")) {strcpy(system_power, json["system_power"]);}
        if (json.containsKey("tele_battery_set")) {strcpy(tele_battery_set, json["tele_battery_set"]);}
        if (json.containsKey("remote_switch")) {strcpy(remote_switch, json["remote_switch"]);}
        if (json.containsKey("auto_discovery")) {strcpy(auto_discovery, json["auto_discovery"]);}   
        if (json.containsKey("open_limit_set")) {strcpy(open_limit_set, json["open_limit_set"]);} 
        if (json.containsKey("close_limit_set")) {strcpy(close_limit_set, json["close_limit_set"]);} 
          strcpy(mqtt_isAuthentication, json["mqtt_isAuthentication"]);          
          strcpy(mqtt_username, json["mqtt_username"]);
          strcpy(mqtt_password, json["mqtt_password"]);

          strcpy(update_path, json["update_path"]);
          
          strcpy(mqtt_server, json["mqtt_server"]);

          strcpy(mqtt_port, json["mqtt_port"]);        
          strcpy(mqtt_topic, json["mqtt_topic"]);   ///device friendly name          
          strcpy(blinds_speed, json["blinds_speed"]);          
          strcpy(blinds_swing_direction, json["blinds_swing_direction"]);          
          strcpy(blinds_servo_install, json["blinds_servo_install"]);          
          strcpy(blinds_trim_adjust, json["blinds_trim_adjust"]);          
          strcpy(blinds_slip_correction, json["blinds_slip_correction"]);          
          strcpy(OTAAuto_path, json["OTAAuto_path"]);          
          strcpy(tele_update_set, json["tele_update_set"]);
          
        }
        else
        {
        }
        configFile.close();
      }

    }

  }
  else
  {
  }
  
  
  
  
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_text0("<p>Select your wifi network and type in your password, if you do not see your wifi then scroll down to the bottom and press scan to check again.");
  WiFiManagerParameter custom_text1("<h1>DHCP/Network Device Hostname Name ID</h1>");
  WiFiManagerParameter custom_text2("<p>Enter a name for this device which will be used as the DHCP/network name e.g. Blindcontrol1.");
  WiFiManagerParameter custom_host("name", "DHCP/Network Device Name", host, 32);
  WiFiManagerParameter custom_text30("<h1>Device Friendly Name</h1>");
  WiFiManagerParameter custom_text31("<p>Enter a Friendly Name can contain space, upper case etc e.g. Kitchen Blind, this will be converted to Unique ID  .");
  WiFiManagerParameter custom_mqtt_topic("topic", "Device Friendly Name", mqtt_topic, 50);
  WiFiManagerParameter custom_text3("<h1>MQTT</h1>");
  WiFiManagerParameter custom_text4("<p>Enter the details of your MQTT server and then enter the topic for which the device listens to MQTT commands from. If your server requires authentication then set it to True and enter your server credentials otherwise leave it at false and keep the fields blank.");
  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server IP", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Server Port", mqtt_port, 5);
  WiFiManagerParameter custom_mqtt_isAuthentication("isAuthentication", "MQTT Authentication?", mqtt_isAuthentication, 7);
  WiFiManagerParameter custom_mqtt_username("userMQTT", "Username For MQTT Account", mqtt_username, 40);
  WiFiManagerParameter custom_mqtt_password("passwordMQTT", "Password For MQTT Account", mqtt_password, 40);
  WiFiManagerParameter custom_text5("<h1>Web Updater/Reset</h1>");
  WiFiManagerParameter custom_text6("<p>The web updater allows you to update the firmware of the device or reset the device via a web browser by going to HOST NAME or DEVICE IP ADDRESS/firmware eg. 192.168.0.5/firmware you can change the update path below. The update page and reset optiion is protected so enter a username and password you would like to use to access it.");
  WiFiManagerParameter custom_update_username("user", "Username For Web Updater", update_username, 40);
  WiFiManagerParameter custom_update_password("password", "Password For Web Updater", update_password, 40);
  WiFiManagerParameter custom_battery_system("battery system", "Enable/Disable Battery System", battery_system, 12);
  WiFiManagerParameter custom_device_path("path", "Updater Path", update_path, 32);
  WiFiManagerParameter custom_text25("<p>OTA Path to Firmware Update Server");
  WiFiManagerParameter custom_OTAAuto_path("OTA Path", "Path to Remote OTA Server?", OTAAuto_path, 90);
  WiFiManagerParameter custom_text26("<p>Telemetry Period in Seconds, Default 60s");
  WiFiManagerParameter custom_tele_update_set("Telemetry Period", "Telemetry Period In Seconds", tele_update_set, 6);
  WiFiManagerParameter custom_text15("<h1>Custom Blind Control</h1>");
  WiFiManagerParameter custom_text7("<p>Speed You Want The Blinds To Move?  FAST or SLOW?");
  WiFiManagerParameter custom_blinds_speed("Speed", "Move Blinds Speed?", blinds_speed, 7);
  WiFiManagerParameter custom_text8("<p>*To physically reset device settings restart the device and quickly move the jumper from RUN to PGM, wait 10 seconds and put the jumper back to RUN.*");
  WiFiManagerParameter custom_text9("<p>*To remote reset device delete all settings via web browser by going to  HOST NAME or DEVICE IP ADDRESS/reset  .eg. .192.168.0.5/reset  ..Wait 10 seconds for reboot*");
  WiFiManagerParameter custom_text20("<p>What Side Is The Servo Motor Installed? LEFT/RIGHT, (Default is Left)");
  WiFiManagerParameter custom_blinds_servo_install("Servo Install", "Servo Motor Installed?", blinds_servo_install, 7);
  WiFiManagerParameter custom_text21("<p>Swing direction of blind from Open to Close?, DOWN/UP  (Default is DOWN)");
  WiFiManagerParameter custom_blinds_swing_direction("Swing Direction", "Swing Direction?", blinds_swing_direction, 7);
  WiFiManagerParameter custom_text17("<p>To adjust the trim angle of the blinds physical open position against software open position. Between  0-75 Percent  Default=0 ");
  WiFiManagerParameter custom_blinds_trim_adjust("Open Trim Adjustment", "Open Trim Adjustment Percent ? ", blinds_trim_adjust, 3);
  WiFiManagerParameter custom_text24("<p>Blinds Weight Slip Adjustment ? Moves Blind to Open Position before Moving to Mid Range Position, ON/OFF ,  OFF will give incorrect Position 25 percent error, Default is ON)*");
  WiFiManagerParameter custom_blinds_slip_correction("Slip Correction", "Weight Slip Correction ? ", blinds_slip_correction, 5);
  WiFiManagerParameter custom_text16("<h1>Setup Notes</h1>");
  
WiFiManagerParameter custom_text27("<p>*Once you have Configured SSID, PASSWORD, MQTT etc, SAVE, then goto IP/Host name and select Setup to configure extra paramaters, i.e. motor install, swing direction, speed etc *");

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  wifi_station_set_hostname(host);
  WiFi.hostname(host);
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.setCustomHeadElement("<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:oswald;} button{border:0;background-color:#313131;color:white;line-height:2.4rem;font-size:1.2rem;text-transform: uppercase;width:100%;font-family:oswald;} .q{float: right;width: 65px;text-align: right;} body{background-color: #575757;}h1 {color: white; font-family: oswald;}p {color: white; font-family: open+sans;}a {color: #78C5EF; text-align: center;line-height:2.4rem;font-size:1.2rem;font-family:oswald;}</style>");
  wifiManager.addParameter(&custom_text0);
  wifiManager.addParameter(&custom_text1);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_host);
  wifiManager.addParameter(&custom_text30);
  wifiManager.addParameter(&custom_text31);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_text3);
  wifiManager.addParameter(&custom_text4);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_isAuthentication);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_text16);
  wifiManager.addParameter(&custom_text27);
  
  //reset settings - for testing
  //wifiManager.resetSettings();

  //wifi_station_set_hostname(host);
 //WiFi.hostname(host);
  //WiFi.hostname(host);
  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(120);
  wifiManager.setConfigPortalTimeout(120);   ///new code replaces setTimeout()
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  
  ///check here for possible error leaving AP open
    ///if (!wifiManager.autoConnect(networkhost.c_str()))
  if (!wifiManager.autoConnect(ssidAP.c_str()))
 {
    
    delay(6000);
    //reset and try again, or maybe put it to deep sleep
   
    ESP.reset();
    delay(5000);
  }

  //read updated parameters
  strcpy(host, custom_host.getValue());
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());    ////device full friendly name
  strcpy(mqtt_isAuthentication, custom_mqtt_isAuthentication.getValue());
  strcpy(mqtt_username, custom_mqtt_username.getValue());
  strcpy(mqtt_password, custom_mqtt_password.getValue());
  strcpy(update_username, custom_update_username.getValue());
  strcpy(update_password, custom_update_password.getValue());
  strcpy(update_password, custom_update_password.getValue());
  strcpy(update_path, custom_device_path.getValue());
  strcpy(blinds_speed, custom_blinds_speed.getValue());
  strcpy(blinds_swing_direction, custom_blinds_swing_direction.getValue());
  strcpy(blinds_servo_install, custom_blinds_servo_install.getValue());
  strcpy(blinds_trim_adjust, custom_blinds_trim_adjust.getValue());
  strcpy(blinds_slip_correction, custom_blinds_slip_correction.getValue());
  strcpy(OTAAuto_path, custom_OTAAuto_path.getValue());
  strcpy(tele_update_set, custom_tele_update_set.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
   // Serial.print("Saving configuration file ");

    
    DynamicJsonDocument json(1024);    ////JSON6

    json["host"] = host;
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_topic"] = mqtt_topic;   ////device full friendly name
    json["mqtt_isAuthentication"] = mqtt_isAuthentication;
    json["mqtt_username"] = mqtt_username;
    json["mqtt_password"] = mqtt_password;
    json["update_username"] = update_username;
    json["update_password"] = update_password;
    json["battery_system"] = battery_system;
    json["remote_switch"] = remote_switch;
    json["auto_discovery"] = auto_discovery;
    json["battery_capacity"] = battery_capacity;
    json["tele_battery_set"] = tele_battery_set;
    json["system_power"] = system_power;
    json["update_path"] = update_path;
    json["blinds_speed"] = blinds_speed;
    json["blinds_swing_direction"] = blinds_swing_direction;
    json["blinds_servo_install"] = blinds_servo_install;
    json["blinds_trim_adjust"] = blinds_trim_adjust;
    json["blinds_slip_correction"] = blinds_slip_correction;
    json["OTAAuto_path"] = OTAAuto_path;
    json["tele_update_set"] = tele_update_set;
    json["open_limit_set"] = open_limit_set;
    json["close_limit_set"] = close_limit_set;

    /////save  software config.json file///
    //File configFile = SPIFFS.open("/" + String(software_version) + ".json", "w"); ////old system
    File configFile = LittleFS.open("/" + String(software_version) + ".json", "w");
    if (!configFile)
    {
    }
    
    //////JSON6
    serializeJson(json, Serial);   /////JSON6
    serializeJson(json, configFile);    //////JSON6
    //////////

    configFile.close();

  }

  delay(5000);

  if (digitalRead(0) == LOW || String(host).length() == 0 || String(mqtt_server).length() == 0 || String(mqtt_topic).length() == 0)
  {
    
    wifiManager.resetSettings();
    ESP.reset();
  }

  mqttDeviceID = host; /////host is the nerwork name
  /////asign variable from main
      _name = String(mqtt_topic);  ///aka mqtt device ID will change in main program friendly name
// Id = name to lower case replacing spaces by underscore (ex: name="Kitchen Light" -> id="kitchen_light")
    _identifier = _name;
    _identifier.replace(' ', '_');
    _identifier.toLowerCase();
////////////////////////////////




/////////////////////////////////
  client.begin(mqtt_server, atoi(mqtt_port), net);
  client.onMessage(messageReceived);  // existing process MQTT Messages
  
  connect();

  MDNS.begin(host);

  httpUpdater.setup(&httpServer);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

//////new code
  //----------------------------------------------------------------------   
  
  
    httpServer.on("/description.xml", HTTP_GET, []() {
      SSDP.schema(httpServer.client());
    });
  
  
  
  ///////////////////////////// Server Commands 
  httpServer.on("/",         HomePage);
  httpServer.on("/download", File_Download);
  httpServer.on("/upload",   File_Upload);
  httpServer.on("/help",   Help);
  httpServer.on("/fupload",  HTTP_POST,[](){ httpServer.send(200);}, handleFileUpload);
  httpServer.on("/reboot",  Device_Reboot);
  httpServer.on("/delete",   File_Delete);
  httpServer.on("/dir",      SPIFFS_dir);
  httpServer.on("/setup",      Config_Setup);
  httpServer.on("/firmware",      Firmware_Update);
  httpServer.on("/firmwareupdate",      Firmware_Update);
  httpServer.on("/firmwareauto",      OTAUpgrade);
  httpServer.on("/firmwareauto",      OTAUpgrade);
  httpServer.on("/set-servo",         Set_Servo);
  httpServer.on("/set-limits",         handleMoveServo);
  
    //httpServer.on("/servoaction",    Servo_Action);
  httpServer.on("/firmwarecheck",      FirmwareCheck);
  httpServer.on("/filemanager",      File_Manager);
  httpServer.on("/saveconfig",      Save_Config);
  httpServer.on("/submitconfig",      Submit_Config);
  // httpServer.on("/action_page", handleForm);
  httpServer.on("/exit",      HomePage);
  httpServer.on("/resetcmd",      longHoldEvent); 
  httpServer.on("/setPOS", handleServo); //--> Sets servo position from Web request
  httpServer.on("/SetOpenLimit",      SetOpenLimit); 
  httpServer.on("/SetClosedLimit",      SetClosedLimit); 
  httpServer.on("/ResetLimits",      ResetLimits); 
  ///////////////////////////// End of Request commands
  // Send web page with input fields to client

     
  //New Code To run Reset Ap And Re Launch WiFiManager Portal
  httpServer.on("/reset", []() {
    if (!httpServer.authenticate(update_username, update_password))
      return httpServer.requestAuthentication();
    Serial.print("Resetting Devkce... ");
  SendHTML_Header();
   
  webpage += F("<h3 class='rcorners_m'>Reset Controller</h3><br>");
  webpage += F("<table align='center'>");
  webpage += F("<h3> All Settings Will Be Deleted and Set To Defaults</h3>");
  webpage += F("<h3> File System Will Be Reformated.....</h3>");
  webpage += F("<h3> You Will Need To Connect Via AP Mode And Setup Device Again</h3>");
  //webpage += F("<h3> Device Will AUTO Restart In 10 Seconds in AP Setup mode</h3>");
  webpage += F("<h3> Press Continue to Proceed or Cancel to Exit</h3>");
  webpage += F("<table align='center'>");
  webpage += F("</table>");
  webpage += F("<a href='/resetcmd'><button>Continue</button></a>");
  webpage += F("<a href='/'><button>Cancel</button></a>");
   append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent

  });

  httpServer.begin();

//////////////////////////////
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName(host);
  SSDP.setModelName("Mk-Blindcontrol");
  SSDP.setURL("/");
  SSDP.setDeviceType("upnp:rootdevice");
  SSDP.setSerialNumber(ESP.getChipId());
  SSDP.setModelNumber("MK-Blindcontrol V7");
  SSDP.setModelURL("https://mountaineagle-technologies.com.au");
  SSDP.setManufacturer("MK Smarthouse");
  SSDP.setManufacturerURL("https://www.mksmarthouse.com");
SSDP.begin();

////////////////////////////
  MDNS.addService("http", "tcp", 80);
  //MDNS.addService("https", "tcp", 443);
/////////////////////////////////////////
AutoConfigBlind();
get_save_state();

moveServo(ServoPos); ///moves servo to set pos as on powerup servo pos defaults to 90 regardless of previous osition
myservo[0].detach();
myservo[1].detach();

 }
///////main loop
void loop()
{
  client.loop();
  delay(10);

  if (!client.connected())
  {
    //connect();
    client.disconnect();
    connect();

  }

  httpServer.handleClient();
  MDNS.update();
  /////set to 0 to stop telementry update. help power save 
 if (String(tele_update_set).toInt()>0)
  {
  //Serial.printlnln("Telementry update...");
  tele_update();
  }

  
////Battery Check Monitor
  if (String(battery_system).equalsIgnoreCase("ON"))
  {
    //Serial.println("Checking Battery Status.. ");
    Battery_Check();   
  }
  
// Get button event and act accordingly
    button_result = 0;
    checkButton();
if (String(remote_switch).equalsIgnoreCase("YES"))
  {
   
   
   if (button_result == 1) {clickEvent();}
   if (button_result == 2) {doubleClickEvent();}
   if (button_result == 3) {holdEvent();}
   if (button_result == 4) {longHoldEvent();} ////allows for reset operation
  }
   
 
}

/////end void loop

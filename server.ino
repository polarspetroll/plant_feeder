#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <stdio.h>

#define relay_pin 1

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);

/* server handlers */
void root_handler();
void change_info();
void control();
void pump_off();
void pump_on();
void not_found();
void insert_db();
void script();
/******************/

void setup() {
  /// WI-FI ///
  Serial.begin(115200);
  SPIFFS.begin();
  File ssid_conf = SPIFFS.open("ssid.txt", "r");
  File password_conf = SPIFFS.open("password.txt", "r");
  if (!ssid_conf || !password_conf) {
    Serial.println("error while openning the config file.");
  }
  String ssid;
  String password;
  int i;
  for (i = 0;i < ssid_conf.size(); i++) {
    ssid += String((char)ssid_conf.read());
  }
  for (i = 0; i < password_conf.size(); i++) {
    password += String((char)password_conf.read());
  }

  ssid_conf.close();
  password_conf.close();

  if (ssid.length() < 1) {
    ssid = "ESP8266";
  }
  if (password.length() < 8) {
    password = "password";
  }
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.print("http://");
  Serial.println(WiFi.softAPIP());

  ///Web server///
  server.on("/", HTTP_GET, root_handler);
  server.on("/change", HTTP_ANY, change_info);
  server.on("/change/apply", HTTP_POST, insert_db);
  server.on("/pump", HTTP_GET, control);
  server.on("/pump/on", HTTP_GET, pump_on);
  server.on("/pump/off", HTTP_GET, pump_off);
  server.on("/script", HTTP_ANY, script);
  server.onNotFound(not_found);
  server.begin();

  /// pump ////
  pinMode(relay_pin, OUTPUT);

}

void loop(){
  server.handleClient();
}

void root_handler() {
  server.send(200, "text/html", "<html><body style=background-color:gray;><div align=center><fieldset style=width:200px;height:70px><a href=/change>Change credentials</a><br><br><a href=/pump>Control pump</a><br></fieldset></div></body></html>");
}

void script() {
  server.send(200, "application/javascript", "function change_val(change_to){var path='';var c='';if(change_to==1){path='/on';c='0'}else{path='/off';c='1'}var r='';var el=document.getElementById('st');fetch('http://192.168.4.1/pump'+path).then(response=>response.json()).then(res=>st.innerHTML=res.status);document.getElementById('btn').attributes[2].value=`change_val(${ c })`}");
}

void control() {
  String stat;
  int cval;
  int val = digitalRead(relay_pin);

  if (val == 1) {
    stat = "on";
    cval = 0;
  }else {
    stat = "off";
    cval = 1;
  }
  char resp[290];
  sprintf(resp, "<html><body style=background-color:gray;><p>current status :</p><p id=\"st\">%s</p><script src=\"/script\"></script><center><button id=\"btn\" style=\"height:100px;width:100px;border-radius:50px;\" onclick=\"change_val(%d)\"><h1 style=color:red;>on/off</h1></button></center></body></html>", stat, cval);
  server.send(200, "text/html", resp);
}

void pump_off() {
  digitalWrite(relay_pin, HIGH);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\": \"off\"}");
}
void pump_on() {
  digitalWrite(relay_pin, LOW);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\": \"on\"}");
}

void change_info() {
  server.send(200, "text/html", "<body style=background-color:gray><center><form action=/change/apply method=POST><input name=ssid placeholder=ssid> <input name=password placeholder=password> <button>submit</button></form></center>");
}

void insert_db() {
  String new_ssid = server.arg("ssid");
  String new_password = server.arg("password");
  if (new_password.length() < 8 || new_ssid.length() < 1) {
    server.send(400, "text/plain", "invalid ssid or password");
    return;
  }
  SPIFFS.format();
  File ssid_file = SPIFFS.open("ssid.txt", "w");
  File password_file = SPIFFS.open("password.txt", "w");
  ssid_file.print(new_ssid);
  password_file.print(new_password);
  password_file.close();
  ssid_file.close();

  server.send(200, "text/plain", "changed! Please restart the board");
}



void not_found() {
  server.send(404, "text/html", "<html><body style=background-color:gray;><center><h1 style=color:red;>404 NOT FOUND</h1></center></body></html>");
}

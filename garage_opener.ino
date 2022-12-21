#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// Replace with your network credentials
const char* ssid = "xxxx";
const char* password = "xxxx";

//const char* PARAM_INPUT_1 = "state";

const int relayPin = D1;
const int sensorPin = D2;
const long pressTime = 500;
bool doorOpen;

bool doorShallSwitch = false;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Garage Door Opener</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block;} 
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    #output {width: 300px; height: 100px;}
  </style>
</head>
<body>
  <!---<h2>Garage Door Opener</h2>--->
  %BUTTONPLACEHOLDER%
  
<script>
function toggleGarageDoor(element) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/door", true);
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      var buttonText;
      if( this.responseText == 1){ 
        inputChecked = "#00FF00";
        buttonText = "CLOSE";
        outputStateM = "OPEN";
      }
      else { 
        inputChecked = "#FF0000";
        buttonText = "OPEN";
        outputStateM = "CLOSED";
      }
      document.getElementById("output").style.backgroundColor = inputChecked
      document.getElementById("output").value= buttonText;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String buttonString = doorStateButtonString();
    buttons+= "<h2>Garage Door is <span id=\"outputState\"></span></h2><label class=\"switch\"><button id=\"output\" onclick=\"toggleGarageDoor(this)\">" + buttonString +"</button>";  /*<input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + doorState + "><span class=\"slider\"></span></label>";*/
    return buttons;
  }
  return String();
}

String doorStateButtonString(){
  if(doorOpen){
    return "CLOSE";
  }
  else {
    return "OPEN";
  }
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  switchGarageDoor();

  pinMode(relayPin, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  doorOpen = digitalRead(sensorPin);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  attachInterrupt(digitalPinToInterrupt(sensorPin), changeDoorStatus, CHANGE);

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  
  server.on("/door", HTTP_GET, [] (AsyncWebServerRequest *request) {
    doorShallSwitch = true;
    request->send(200, "text/plain", "OK");
  });

  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(doorOpen).c_str());
  });


  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/json", "{\"result\": \"success\"}");
    ESP.restart();
  });
  
  AsyncElegantOTA.begin(&server);
  server.begin();
}


ICACHE_RAM_ATTR void changeDoorStatus() {
  doorOpen = digitalRead(sensorPin);
  
  Serial.print("State changed to:");
  Serial.println(doorOpen);
}

void switchGarageDoor() {
  Serial.println("Enabling and disabling the relay");
  digitalWrite(relayPin, HIGH); // turn on relay with voltage HIGH
  delay(pressTime);             // pause
  digitalWrite(relayPin, LOW);  // turn off relay with voltage LOW
}
  
void loop() {
  if(doorShallSwitch) {
    switchGarageDoor(); 
    doorShallSwitch = false;
  }
}

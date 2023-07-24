#include <stdio.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <myWiFi.h>  // Includes Jon's WIFI SSID and PASSWORD
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WebSocketsServer.h>

ESP8266WebServer server(80);  // Port 80 is the standard http server port
WebSocketsServer webSocket = WebSocketsServer(443);  // Port 443 is the standard Web Sockets port
Adafruit_SSD1306 display(128, 64);
extern void webpage();

void setup() {
  // put your setup code here, to run once:
  Wire.begin(2, 0);  // Specific to ESP-01 module, and my setup.  You may want Wire.begin();
  Serial.begin(115200);
  delay(200);
  
  Serial.println("Initializing Display...");
  // initialize with the I2C addr 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(500);
  
  // Clear the Screen
  display.clearDisplay();
  display.setTextSize(1);
  display.println("Connecting to:");
  display.println(MY_WIFI_SSID);
  display.display();
  
  Serial.print("Connecting to WIFI...");
  WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);  // put your data "mySSIDValue", "myWiFiPassword" here

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  

  server.begin();  //Start the Web Server
  webSocket.begin();   // Start the websockets server
  server.on("/", webpage); // Set up the default page server
}

uint32_t nextScreenTransmit;
void loop() {
  server.handleClient();
  webSocket.loop();

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(4);
  display.setCursor(0, 0);
  display.println(millis() / 1000);
  display.setTextSize(1);
  display.print("\n\n\n");
  display.println(WiFi.localIP());
  display.display();

  if (millis() > nextScreenTransmit)
  {
    char s[100];
    nextScreenTransmit = millis() + 1000;
    webSocket.broadcastBIN(display.getBuffer(), 1024); // If you get an error about buffer being protected, you
    // need to modify Adafruit_SSD1306.h to make buffer public, not protected:w

    sprintf(s, "{\"u32UpTime\" : \"%lu\"}", millis());
    webSocket.broadcastTXT(s);
  }
}

extern const char webpageCode0[] PROGMEM;
extern const char webpageCode1[] PROGMEM;
//=======================================
//handle function: send webpage to client
//=======================================
void webpage()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send ( 200, "text/html", webpageCode0);
  server.sendContent(WiFi.localIP().toString());
  server.sendContent ( webpageCode1);
  server.client().stop();
}


//=====================
//HTML code for webpage
//=====================
const char webpageCode0[] PROGMEM =
  R"=====(
<!doctype html>
<html>

<head>
    <title>ScreenBroadcast</title>
</head>
<!-------------------------------C S S------------------------------>

<body>
    <h1>ScreenBroadcast</h1>
    <canvas id="canvas" height="256" width="512"></canvas><br>
    Up Time: <span id="u32UpTime"></span>


    <script>

        const canvas = document.getElementById("canvas");
        const ctx = canvas.getContext("2d");
            ctx.scale(4,4);

        InitWebSocket();
        
        function InitWebSocket() {
            websock = new WebSocket('ws://)=====";

// Dynamically put the IP address here

const char webpageCode1[] PROGMEM =
  R"=====(:443/');
            var arrayBuffer;
            var fileReader = new FileReader();
            fileReader.onload = function (event) {
                arrayBuffer = event.target.result;
                var data = new Uint8Array(arrayBuffer);
                var sum = 0;
                var count = 0;

                ctx.fillStyle = "black";
                ctx.fillRect(0, 0, 128, 64);
                ctx.fillStyle = "cyan";
                for (var y = 0; y < 64; ++y) {
                    for (var x = 0; x < 128; ++x) {
                        var bit = data[x + Math.floor(y / 8) * 128] & (1 << (y & 7))
                        if (bit) {
                            ctx.fillRect(x, y, 1, 1);
                        }
                    }
                }
            };
            websock.onmessage = function (evt) {
                var t = typeof evt.data;
                if (typeof evt.data == 'object') {
                    var d = evt.data;
                    fileReader.readAsArrayBuffer(d);
                }
                else if (typeof evt.data == 'string') {
                    JSONobj = JSON.parse(evt.data);
                    if (JSONobj.u32UpTime != undefined) {
                        document.getElementById('u32UpTime').innerHTML = JSONobj.u32UpTime;
                    }
                }
            }
        }
        //-------------------------------------------------------------
        

        function windowResize() {
            canvas.width = window.innerWidth * .95;

        };

        window.addEventListener('resize', windowResize);
    </script>
</body>

</html>
)=====";

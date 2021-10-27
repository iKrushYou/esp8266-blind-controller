/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-async-web-server-espasyncwebserver-library/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>

// Replace with your network credentials
const char *ssid = "brookesbar";
const char *password = "brookelindstrom";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// // Define a stepper and the pins it will use
AccelStepper stepper(AccelStepper::DRIVER, 4, 5);
long oneRotation = 200 * 5.18 * 16;
long openPosition = oneRotation * 5;
long zeroPosition = 0;
bool zeroClosed = true;

long getOpenPos()
{
  return zeroClosed ? zeroPosition + openPosition : zeroPosition;
}

long getClosedPos()
{
  return zeroClosed ? zeroPosition : zeroPosition - openPosition;
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name=\"viewport\" content=\"width=device-width\">
<body>

<h1>
  TV Blind Automation Server
</h1>
<div style="margin-bottom: 30px">
  <label for="positionInput" style="display: block">
    Current Position
  </label>
  <input id="positionInput" value="%CURRENT_POSITION%" />
  <button onclick="handleSetPosition()">
    Set Position
  </button>
</div>
<div style="margin-bottom: 30px">
  <label for="positionInput" style="display: block">
    Zero Position
  </label>
  <input id="positionInput" value="%ZERO_POSITION%" disabled />
  <button onclick="handleZero('open')">
    Zero Open
  </button>
  <button onclick="handleZero('closed')">
    Zero Closed
  </button>
</div>
<div style="margin-bottom: 30px">
  <button onclick='onSetOpenClosed(`open`)'>
    Open
  </button>
  <button onclick='onSetOpenClosed(`closed`)'>
    Close
  </button>
</div>

<script>
  async function onSetOpenClosed(openClosed) {
    await fetch(`/api/position?value=${openClosed}`, {
      method: "PUT"
    });
  }

  async function handleSetPosition() {
    const position = document.getElementById("positionInput").value;
    if (position.length === 0) return;
    await fetch(`/api/position?value=${position}`, {
      method: "PUT"
    });
  }

  async function handleZero(openClosed) {
    await fetch(`/api/zero?position=${openClosed}`, {
      method: "PUT"
    });
  }
</script>

</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String &var)
{
  //Serial.println(var);
  if (var == "CURRENT_POSITION")
  {
    Serial.println("currentPosition: " + String(stepper.currentPosition()));
    return String(stepper.currentPosition());
  }
  else if (var == "ZERO_POSITION")
  {
    return String(zeroPosition);
  }
  return String();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("Starting Up");

  // Connect to Wi-Fi
  Serial.print("Connecting to network ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  stepper.setEnablePin(14);
  stepper.setPinsInverted(false, false, false, false, true);
  stepper.setMaxSpeed(oneRotation);
  stepper.setAcceleration(oneRotation);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  server.on("/api/position", HTTP_PUT, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("value"))
              {
                String positionValue = request->getParam("value")->value();
                Serial.println("value=" + positionValue);
                long moveToPos = stepper.currentPosition();

                if (positionValue == "open")
                {
                  moveToPos = getOpenPos();
                }
                else if (positionValue == "closed")
                {
                  moveToPos = getClosedPos();
                }
                else
                {
                  moveToPos = positionValue.toInt();
                }
                stepper.moveTo(moveToPos);
                Serial.println("Moving to " + String(moveToPos));
              }
              request->send(200, "text/plain", "OK");
            });

  server.on("/api/zero", HTTP_PUT, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("position"))
              {
                String positionValue = request->getParam("position")->value();
                zeroClosed = positionValue == "closed";
                zeroPosition = stepper.currentPosition();
              }
              request->send(200, "text/plain", "OK");
            });

  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              int status = stepper.currentPosition() == getOpenPos() ? 1 : 0;
              request->send(200, "text/plain", String(status));
            });

  // Start server
  server.begin();
}

bool stepperEnabled = true;

void loop()
{
  stepper.run();
  if (stepper.currentPosition() == getClosedPos() || stepper.currentPosition() == getOpenPos())
  {
    if (stepperEnabled)
    {
      Serial.println("disabling outputs");
      stepper.disableOutputs();
      stepperEnabled = false;
    }
  }
  else
  {
    if (!stepperEnabled)
    {
      Serial.println("enabling outputs");
      stepper.enableOutputs();
      stepperEnabled = true;
    }
  }
}

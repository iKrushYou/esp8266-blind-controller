// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <AccelStepper.h>

// Replace with your network credentials
const char *ssid = "brookesbar";
const char *password = "brookelindstrom";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String blindPosition = "open";

// // Define a stepper and the pins it will use
AccelStepper stepper(AccelStepper::DRIVER, 4, 5);
long oneRotation = 200 * 5.18 * 16;
long openPosition = -oneRotation * 5;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup()
{
    Serial.begin(115200);

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    stepper.setMaxSpeed(oneRotation);
    stepper.setAcceleration(oneRotation);

    server.begin();
}

char *getHomePage()
{
    return "<!DOCTYPE html><html>"
           "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
           "<link rel=\"icon\" href=\"data:,\">"
           "<body>"

           "<h1>"
           "  TV Blind Automation Server"
           "</h1>"
           "<div style='margin-bottom: 30px'>"
           "  <label for='positionInput' style='display: block'>"
           "    Current Position"
           "  </label>"
           "  <input id='positionInput' />"
           "  <button onclick='handleSetPosition()'>"
           "    Set Position"
           "  </button>"
           "</div>"
           "<div style='margin-bottom: 30px'>"
           "  <button onclick='onSetOpenClosed(\"open\")'>"
           "    Open"
           "  </button>"
           "  <button onclick='onSetOpenClosed(\"closed\")'>"
           "    Close"
           "  </button>"
           "</div>"

           "<script>"
           "async function onSetOpenClosed(openClosed) {"
           "  await fetch(`/api/position/${openClosed}`, {"
           "    method: 'PUT'"
           "  })"
           "}"
           ""
           "async function handleSetPosition() {"
           "  const position = document.getElementById('positionInput').value;"
           "  if (position.length === 0) return;"
           "  await fetch(`/api/position/${position}`)"
           "}"
           "</script>"

           "</body>"
           "</html>";
}

void handleApi()
{
}

void loop()
{
    stepper.run();

    if (Serial.available() > 0)
    {
        int incomingByte = Serial.read();
        if (incomingByte >= 49 && incomingByte <= 52)
        {
            double rotation = oneRotation * (incomingByte - 48) * .25;
            stepper.move(rotation);
            Serial.println("Moving stepper: " + String(rotation));
        }
        else if (incomingByte >= 55 && incomingByte <= 58)
        {
            double rotation = oneRotation * (incomingByte - 54) * -.25;
            Serial.println("Moving stepper: " + String(rotation));
            stepper.move(rotation);
            Serial.println("Moving stepper: " + String(rotation));
        }
        else if (incomingByte == 48)
        {
            double rotation = oneRotation * -1;
            Serial.println("Moving stepper: " + String(rotation));
            stepper.move(rotation);
        }
        else if (incomingByte == 10)
        {
            Serial.println("Stepper is at position: " + String(stepper.currentPosition()));
        }
    }

    WiFiClient client = server.available(); // Listen for incoming clients

    if (client)
    {                                  // If a new client connects,
        Serial.println("New Client."); // print a message out in the serial port
        String currentLine = "";       // make a String to hold incoming data from the client
        currentTime = millis();
        previousTime = currentTime;
        while (client.connected() && currentTime - previousTime <= timeoutTime)
        { // loop while the client's connected
            currentTime = millis();
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                Serial.write(c);        // print it out the serial monitor
                header += c;
                if (c == '\n')
                { // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();

                        // turns the GPIOs on and off
                        if (header.indexOf("GET /5/on") >= 0)
                        {
                            Serial.println("GPIO 5 on");
                            blindPosition = "open";
                            stepper.moveTo(0);
                        }
                        else if (header.indexOf("GET /5/off") >= 0)
                        {
                            Serial.println("GPIO 5 off");
                            blindPosition = "closed";
                            stepper.moveTo(openPosition);
                        }

                        client.println(getHomePage());

                        // The HTTP response ends with another blank line
                        client.println();
                        // Break out of the while loop
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}

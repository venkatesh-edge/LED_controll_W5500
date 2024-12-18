#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <SparkFunHTU21D.h>  // Include HTU21D library or your temperature sensor library

const int ledPins[] = { 18, 11, 9, 8 };             // Pins for the LEDs
bool ledStates[] = { false, false, false, false };  // Track LED states

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // MAC address
byte ip[] = { 10, 10, 10, 133 };                      // Static IP
byte subnet[] = { 255, 255, 255, 0 };                 // Subnet mask
byte gateway[] = { 10, 10, 10, 1 };                   // Gateway

EthernetServer server(80);  // Create a server that listens on port 80

HTU21D myHTU21D;  // Create an instance of the HTU21D sensor

void setup() {
  // Initialize LED pins
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // Start Ethernet
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.begin(9600);
  
  // Initialize the temperature sensor
  myHTU21D.begin();
  
  Serial.println("Server is ready.");
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());  // Print the assigned IP
}

void loop() {
  EthernetClient client = server.available();  // Check for incoming clients

  if (client) {
    String currentLine = "";  // Make a String to hold incoming data
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();  // Read a byte
        if (c == '\n') {
          // Check if the request is to turn the LEDs on or off
          if (currentLine.indexOf("GET /on/") >= 0) {
            int ledIndex = currentLine.charAt(currentLine.indexOf("/") + 4) - '0';  // Extract LED index
            if (ledIndex >= 0 && ledIndex < 4) {
              digitalWrite(ledPins[ledIndex], HIGH);  // Turn the LED on
              ledStates[ledIndex] = true;
              Serial.print("LED ");
              Serial.print(ledIndex);
              Serial.println(" is ON");
            }
          } else if (currentLine.indexOf("GET /off/") >= 0) {
            int ledIndex = currentLine.charAt(currentLine.indexOf("/") + 5) - '0';  // Extract LED index
            if (ledIndex >= 0 && ledIndex < 4) {
              digitalWrite(ledPins[ledIndex], LOW);  // Turn the LED off
              ledStates[ledIndex] = false;
              Serial.print("LED ");
              Serial.print(ledIndex);
              Serial.println(" is OFF");
            }
          } else if (currentLine.indexOf("GET /status") >= 0) {
            // Send JSON response with current LED states and temperature
            float temperature = myHTU21D.readTemperature();
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println();
            client.print("{ \"leds\": [");
            for (int i = 0; i < 4; i++) {
              client.print(ledStates[i] ? "true" : "false");
              if (i < 3) client.print(", ");
            }
            client.print("], \"temperature\": ");
            client.print(temperature);
            client.println(" }");
            break;  // Break after sending the status
          }

          // Send a response back to the client (standard page response with temperature)
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<html><body><h1>LED Control & Temperature</h1>");

          // Show LED statuses and control buttons
          for (int i = 0; i < 4; i++) {
            client.print("<p>LED ");
            client.print(i);
            client.print(" is currently ");
            client.print(ledStates[i] ? "ON" : "OFF");
            client.println("</p>");
            client.print("<form action=\"/on/");
            client.print(i);
            client.println("\" method=\"get\"><button>Turn ON LED ");
            client.print(i);
            client.println("</button></form>");
            client.print("<form action=\"/off/");
            client.print(i);
            client.println("\" method=\"get\"><button>Turn OFF LED ");
            client.print(i);
            client.println("</button></form>");
          }

          // Display the current temperature
          float temperature = myHTU21D.readTemperature();
          client.print("<h2>Current Temperature: ");
          client.print(temperature);
          client.println(" °C</h2>");
          
          client.println("</body></html>");
          break;  // Break the loop after sending the response
        } else {
          currentLine += c;  // Add the character to the current line
        }
      }
    }
    client.stop();  // Close the connection
  }
}

#include <SPI.h>
#include <Ethernet.h>

const int ledPins[] = { 20, 11, 9, 8 }; // Pins for the LEDs
bool ledStates[] = { false, false, false, false }; // Track LED states

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC address
byte ip[] = { 10, 10, 10, 133 };  

EthernetServer server(12); // Create a server that listens on port 80

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(9600);
  Serial.println("Server is ready.");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP()); // Print the assigned IP
}

void loop() {
  EthernetClient client = server.available(); // Check for incoming clients

  if (client) {
    String currentLine = ""; // Make a String to hold incoming data
    while (client.connected()) {
      if (client.available()) {
        char c = client.read(); // Read a byte
        if (c == '\n') {
          // Check if the request is to turn the LEDs on or off
          if (currentLine.indexOf("GET /on/") >= 0) {
            int ledIndex = currentLine.charAt(currentLine.indexOf("/") + 4) - '0'; // Extract LED index
            if (ledIndex >= 0 && ledIndex < 4) {
              digitalWrite(ledPins[ledIndex], HIGH); // Turn the LED on
              ledStates[ledIndex] = true;
              Serial.print("LED ");
              Serial.print(ledIndex);
              Serial.println(" is ON");
            }
          } else if (currentLine.indexOf("GET /off/") >= 0) {
            int ledIndex = currentLine.charAt(currentLine.indexOf("/") + 5) - '0'; // Extract LED index
            if (ledIndex >= 0 && ledIndex < 4) {
              digitalWrite(ledPins[ledIndex], LOW); // Turn the LED off
              ledStates[ledIndex] = false;
              Serial.print("LED ");
              Serial.print(ledIndex);
              Serial.println(" is OFF");
            }
          }

          // Send a response back to the client
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<html><body><h1>LED Control</h1>");
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
          client.println("</body></html>");
          break; // Break the loop after sending the response
        } else {
          currentLine += c; // Add the character to the current line
        }
      }
    }
    client.stop(); // Close the connection
  }
}

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <SparkFunHTU21D.h>
#include <EEPROM.h>

const int ledPins[] = { 13, 11, 9, 8 };
bool ledStates[] = { false, false, false, false };

// Structure to hold network settings
struct NetworkConfig {
  byte mac[6];
  byte ip[4];
  byte subnet[4];
  byte gateway[4];
  int port;
};

// Default configuration values
NetworkConfig config = {
  { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }, // MAC address
  { 10, 10, 10, 133 },                     // IP address
  { 255, 255, 255, 0 },                    // Subnet mask
  { 10, 10, 10, 1 },                       // Gateway
  80                                       // Port number
};

EthernetServer server(config.port);
HTU21D myHTU21D;

void setup() {
  // Load configuration from EEPROM
  EEPROM.get(0, config);
  
  Serial.begin(9600);
  Serial.print("Using MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(config.mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  
  Serial.print("Using IP: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(config.ip[i]);
    if (i < 3) Serial.print(".");
  }
  Serial.println();

  // Set up LED pins
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  Ethernet.begin(config.mac, config.ip, config.gateway, config.subnet);
  server.begin();

  Serial.println("Server is ready.");
}


void loop() {
  EthernetClient client = server.available();

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          // Handle configuration updates
          if (currentLine.indexOf("GET /config/") >= 0) {
            // Parse new configuration values from the URL
            int startIndex = currentLine.indexOf("GET /config/") + 13;
            String newConfig = currentLine.substring(startIndex);
            // Example: newConfig = "10.10.10.134/255.255.255.0/10.10.10.1/80"
            int firstSlash = newConfig.indexOf('/');
            int secondSlash = newConfig.indexOf('/', firstSlash + 1);
            int thirdSlash = newConfig.indexOf('/', secondSlash + 1);

            // Parse the new IP, subnet, gateway, and port
            if (firstSlash != -1 && secondSlash != -1 && thirdSlash != -1) {
              // Update IP address
              String ipStr = newConfig.substring(0, firstSlash);
              sscanf(ipStr.c_str(), "%hhu.%hhu.%hhu.%hhu",
                     &config.ip[0], &config.ip[1], &config.ip[2], &config.ip[3]);
              // Update Subnet mask
              String subnetStr = newConfig.substring(firstSlash + 1, secondSlash);
              sscanf(subnetStr.c_str(), "%hhu.%hhu.%hhu.%hhu",
                     &config.subnet[0], &config.subnet[1], &config.subnet[2], &config.subnet[3]);
              // Update Gateway
              String gatewayStr = newConfig.substring(secondSlash + 1, thirdSlash);
              sscanf(gatewayStr.c_str(), "%hhu.%hhu.%hhu.%hhu",
                     &config.gateway[0], &config.gateway[1], &config.gateway[2], &config.gateway[3]);
              // Update Port number
              config.port = newConfig.substring(thirdSlash + 1).toInt();

              // Save new configuration to EEPROM
              EEPROM.put(0, config);
              Serial.println("Configuration updated!");
              Serial.println("New IP: " + String(config.ip[0]) + "." + String(config.ip[1]) + "." + String(config.ip[2]) + "." + String(config.ip[3]));
              Serial.println("New Subnet: " + String(config.subnet[0]) + "." + String(config.subnet[1]) + "." + String(config.subnet[2]) + "." + String(config.subnet[3]));
              Serial.println("New Gateway: " + String(config.gateway[0]) + "." + String(config.gateway[1]) + "." + String(config.gateway[2]) + "." + String(config.gateway[3]));
              Serial.println("New Port: " + String(config.port));

              // Reinitialize Ethernet
              Ethernet.begin(config.mac, config.ip, config.gateway, config.subnet);
            }
          }

          // Serve the main HTML page with LED control and configuration form
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

          // Configuration form
          client.println("<h2>Configure Network Settings</h2>");
          client.println("<form action=\"/config/\" method=\"get\">");
          client.println("IP: <input type=\"text\" name=\"ip\" value=\"" + String(config.ip[0]) + "." + String(config.ip[1]) + "." + String(config.ip[2]) + "." + String(config.ip[3]) + "\"><br>");
          client.println("Subnet: <input type=\"text\" name=\"subnet\" value=\"" + String(config.subnet[0]) + "." + String(config.subnet[1]) + "." + String(config.subnet[2]) + "." + String(config.subnet[3]) + "\"><br>");
          client.println("Gateway: <input type=\"text\" name=\"gateway\" value=\"" + String(config.gateway[0]) + "." + String(config.gateway[1]) + "." + String(config.gateway[2]) + "." + String(config.gateway[3]) + "\"><br>");
          client.println("Port: <input type=\"text\" name=\"port\" value=\"" + String(config.port) + "\"><br>");
          client.println("<input type=\"submit\" value=\"Update Settings\">");
          client.println("</form>");

          client.println("</body></html>");
          break;
        } else {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}

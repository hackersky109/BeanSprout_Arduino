#include <ArduinoJson.h>
#include <SPI.h>
#include <Ultrasonic.h>
#include <LWiFi.h>

char ssid[] = "********";      //  your network SSID (name)
char pass[] = "********";   // your network password

int status = WL_IDLE_STATUS;
// Initialize the Wifi client library
WiFiClient client;
Ultrasonic ultrasonic(4);

String sensorId = "********";
String uploadKey = "********";

const char* server = "********m";  // server's address
String location = "/api/v2/sensors/"+sensorId+"/beansproutdata";
const char* resource = location.c_str();                    // http resource

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 30L * 60L * 1000L; // delay between updates, in milliseconds
const unsigned long firstSetupTime = 3L * 60L * 1000L;

bool first_flag = true;

const unsigned long HTTP_TIMEOUT = 10000;  // max respone time from server
const size_t MAX_CONTENT_SIZE = 512;       // max size of the HTTP response

int getDistance() {
  int RangeInCentimeters;
  RangeInCentimeters = ultrasonic.MeasureInCentimeters();
  Serial.println("Distance:");
  Serial.print(RangeInCentimeters);
  Serial.println(" cm!");
  return RangeInCentimeters;
}

// The type of data that we want to extract from the page
struct UserData {
  char percentage[32];
  char inProgress[32];
};

// ARDUINO entry point #1: runs once when you press reset or power the board
void setup() {
    //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("connected!!!");
}
void uploadData() {
  if (connect(server)) {
    if (sendRequest(server, resource) && skipResponseHeaders()) {
      UserData userData;
      if (readReponseContent(&userData)) {
        printUserData(&userData);
      }
    }
  }
  disconnect();
}
// ARDUINO entry point #2: runs over and over again forever
void loop() {
    if(millis() > firstSetupTime && first_flag) {
      uploadData();
      first_flag = false;
    }
    if (millis() - lastConnectionTime > postingInterval) {
      uploadData();
    }
}

// Open connection to the HTTP server
bool connect(const char* hostName) {
  Serial.print("Connect to ");
  Serial.println(hostName);

  bool ok = client.connect(hostName, 80);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

// Send the HTTP GET request to the server
bool sendRequest(const char* host, const char* resource) {
  int distance = getDistance();
  client.print("GET ");
  client.println(location+"?value="+distance);
  client.println(" HTTP/1.0");
  client.print("Host: ");
  client.println(host);
  client.println("Connection: close");
  client.println();
  lastConnectionTime = millis();
  return true;
}

// Skip HTTP headers so that we are at the beginning of the response's body
bool skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
    Serial.println("No response or invalid response!");
  }

  return ok;
}

bool readReponseContent(struct UserData* userData) {
  // Compute optimal size of the JSON buffer according to what we need to parse.
  // See https://bblanchon.github.io/ArduinoJson/assistant/
  const size_t BUFFER_SIZE =
      JSON_OBJECT_SIZE(8)    // the root object has 8 elements
      + JSON_OBJECT_SIZE(5)  // the "address" object has 5 elements
      + JSON_OBJECT_SIZE(2)  // the "geo" object has 2 elements
      + JSON_OBJECT_SIZE(3)  // the "company" object has 3 elements
      + MAX_CONTENT_SIZE;    // additional space for strings

  // Allocate a temporary memory pool
  DynamicJsonBuffer jsonBuffer(BUFFER_SIZE);

  JsonObject& root = jsonBuffer.parseObject(client);

  if (!root.success()) {
    Serial.println("JSON parsing failed!");
    return false;
  }

  // Here were copy the strings we're interested in
  strcpy(userData->percentage, root["percentage"]);
  strcpy(userData->inProgress, root["InProgress"]);

  // It's not mandatory to make a copy, you could just use the pointers
  // Since, they are pointing inside the "content" buffer, so you need to make
  // sure it's still in memory when you read the string

  return true;
}

// Print the data extracted from the JSON
void printUserData(const struct UserData* userData) {
  Serial.print("Percentage = ");
  Serial.println(userData->percentage);
  Serial.println("InProgress = ");
  Serial.println(userData->inProgress);
}

// Close the connection with the HTTP server
void disconnect() {
  Serial.println("Disconnect");
  client.stop();
}


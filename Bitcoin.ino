#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//------- Replace the following! ------
char ssid[] = "NETGEAR14";       // your network SSID (name)
char password[] = "elegantbug653";  // your network key
// For Non-HTTPS requests
  //WiFiClient client;

// For HTTPS requests
WiFiClientSecure client;


// Just the base of the URL you want to connect to
#define TEST_HOST "api.cryptonator.com"

// OPTIONAL - The finferprint of the site you want to connect to.
#define TEST_HOST_FINGERPRINT "10 76 19 6B E9 E5 87 5A 26 12 15 DE 9F 7D 3B 92 9A 7F 30 13"
// The finger print will change every few months.

const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<title>Number Display</title>
<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
  body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}
  .button {display: block;width: 80px;background-color: #1abc9c;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}
  .button:active {background-color: #16a085;}
  p {font-size: 14px;color: #888;margin-bottom: 10px;}
</style>
<body>

<h2>A Number Display<h2>
<h3> By Frank Li</h3>

<form action="/input_page">
  <label for="number">Enter a Number between 0-9:</label>
  <input type="number" id="number" name="number" min="0" max="9">
  <input type="submit" value="Submit">
</form>

<p>Increment Button</p><a class="button" href="increment">Increment</a>
<p>Decrement Button</p><a class="button" href="decrement">Decrement</a>

<p>Display Bitcoin Last Digit</p><a class="button" href="bitcoin">Bitcoin</a>

</body>
</html>
)=====";

ESP8266WebServer server(80);
int globalNum;

void handleRoot() {
  String s = MAIN_page;
  server.send(200, "text/html", s);
}

void handleForm() {
  int number = getNumber();
  globalNum = number;
  String s = MAIN_page;
  server.send(200, "text/html", s);
  chooseNumber(number);
}

void handleIncrement() {
  String s = MAIN_page;
  int input = globalNum;
  Serial.println(input);
  int number = (input + 1)%10;
  globalNum = number;
  server.send(200, "text/html", s);
  chooseNumber(number);
}

void handleDecrement() {
  String s = MAIN_page;
  int input = globalNum;
  Serial.println(input);
  int number = ((input - 1)%10);
  if(number < 0) number=number+10;
  globalNum = number;
  server.send(200, "text/html", s);
  chooseNumber(number);
}

void handleBitcoin() {
  String s = MAIN_page;
  makeHTTPRequest();
  server.send(200, "text/html", s);
}
void setup() {

  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.begin(115200);

  // Connect to the WiFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  //--------

  // If you don't need to check the fingerprint
  // client.setInsecure();

  // If you want to check the fingerprint
  client.setFingerprint(TEST_HOST_FINGERPRINT);
  server.on("/", handleRoot);
  server.on("/input_page", handleForm);
  server.on("/increment", handleIncrement);
  server.on("/decrement", handleDecrement);
  server.on("/bitcoin", handleBitcoin);
  server.begin();
  Serial.println("HTTP started");
}

void makeHTTPRequest() {

  // Opening connection to server (Use 80 as port if HTTP)
  if (!client.connect(TEST_HOST, 443))
  {
    Serial.println(F("Connection failed"));
    return;
  }

  // give the esp a breather
  yield();

  // Send HTTP request
  client.print(F("GET "));
  // This is the second half of a request (everything that comes after the base URL)
  client.print("/api/ticker/btc-usd"); // %2C == ,
  client.println(F(" HTTP/1.1"));

  //Headers
  client.print(F("Host: "));
  client.println(TEST_HOST);

  client.println(F("Cache-Control: no-cache"));

  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }
  //delay(100);
  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return;
  }

  // This is probably not needed for most, but I had issues
  // with the Tindie api where sometimes there were random
  // characters coming back before the body of the response.
  // This will cause no hard to leave it in
  // peek() will look at the character, but not take it off the queue
  while (client.available() && client.peek() != '{')
  {
    char c = 0;
    client.readBytes(&c, 1);
    Serial.print(c);
    Serial.println("BAD");
  }

  //  // While the client is still availble read each
  //  // byte and print to the serial monitor
  //  while (client.available()) {
  //    char c = 0;
  //    client.readBytes(&c, 1);
  //    Serial.print(c);
  //  }

  int valid = bitcoinPrice();
  if (valid < 0){
    return; 
  }else{
    Serial.print("Bitcoin most significant digit: ");
    Serial.println(valid);
    //5 to A, 4 to B and 0 to C 2 to D 14 to E 12 to F 13 to G
    chooseNumber(valid);
  }
}
void clear() {
  digitalWrite(5, LOW);
  digitalWrite(4, LOW);
  digitalWrite(0, LOW);
  digitalWrite(2, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  digitalWrite(14, LOW);
}
void chooseNumber(int input) {
  Serial.println(input);
  clear();
  switch(input){
    case 0:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(14, HIGH);
      digitalWrite(12, HIGH);
      //Serial.print(0);
      break;
    case 1:
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      //Serial.print(1);
      break;
    case 2:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(14, HIGH);
      digitalWrite(13, HIGH);
      //Serial.print(2);
      break;
    case 3:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(13, HIGH);
      //Serial.print(3);
      break;
    case 4:
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(12, HIGH);
      digitalWrite(13, HIGH);
      //Serial.print(4);
      break;
    case 5:
      digitalWrite(5, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(12, HIGH);
      digitalWrite(13, HIGH);
      //Serial.print(5);
      break;
    case 6:
      digitalWrite(5, HIGH);
      digitalWrite(13, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(14, HIGH);
      digitalWrite(12, HIGH);
      //Serial.print(6);
      break;
    case 7:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      //Serial.print(7);
      break;
    case 8:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(14, HIGH);
      digitalWrite(12, HIGH);
      digitalWrite(13, HIGH);
      //Serial.print(8);
      break;
    case 9:
      digitalWrite(5, HIGH);
      digitalWrite(4, HIGH);
      digitalWrite(0, HIGH);
      digitalWrite(2, HIGH);
      digitalWrite(12, HIGH);
      digitalWrite(13, HIGH);
      //Serial.print(9);
      break;
    }
  }

int getNumber() {
  String inputNumber = server.arg("number");
  char temp = inputNumber[0];
  int number = temp - '0';
  return number;
}
int bitcoinPrice() {
  // Stream& input;

  DynamicJsonDocument doc(384);
  
  DeserializationError error = deserializeJson(doc, client);
  
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return -1;
  }
  
  JsonObject ticker = doc["ticker"];
  const char* price = ticker["price"]; // "33908.95216874"
  char temp = price[strlen(price) - 10];
  int bitPrice = temp - '0';
  return bitPrice;
}
void loop() {
  // put your main code here, to run repeatedly:
  //makeHTTPRequest();
  server.handleClient();
}

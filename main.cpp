#include <Arduino.h>
#include <WiFi.h>
#include <TinyGPS++.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <math.h>

String public_ip;
String apikeyweather = "2339ca0fa8e98d6b258bffb2aae33321";
String geoapikey = "pk.0019b9fa96c63f1a39ae09a92ca67231";
const char *Host = "www.unwiredlabs.com";
String endpoint = "/v2/process.php";
String jsonString = "{\n";
TaskHandle_t ip_handle = NULL;
float lat, lon;

#define RX 18
#define TX 17
TinyGPSPlus gps;

/* Getting ip, location and weather from the internet */
void getIp()
{
  WiFiClient client;
  HTTPClient http;
  String payload = "";
  JSONVar myObject;

  const char *jsonURL = "http://api.ipify.org/?format=json";

  http.begin(client, jsonURL);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    myObject = JSON.parse(payload);
    if (JSON.typeof(myObject) != "undefined" && myObject.hasOwnProperty("ip"))
    {
      Serial.print("IP address: ");
      public_ip = (const char *)myObject["ip"];
      Serial.println(public_ip);
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void getLocation()
{
  // now build the jsonString...
  jsonString = "{\n";
  jsonString += "\"token\" : \"";
  jsonString += geoapikey;
  jsonString += "\",\n";
  jsonString += "\"ip\" : \"" + public_ip + "\",\n";
  jsonString += "\"fallbacks\": {\n";
  jsonString += "    \"ipf\": \"1\"\n";
  jsonString += "},\n";
  jsonString += "\"address\": 1 \n";
  jsonString += ("}\n");
  Serial.println(jsonString);

  WiFiClient client;
  std::string stemp;
  // std::vector<std::string> tokens;

  // Connect to the client and make the api call
  Serial.println("Requesting URL: http://" + (String)Host + endpoint);
  if (client.connect(Host, 80))
  {
    Serial.println("Connected");
    client.println("POST " + endpoint + " HTTP/1.1");
    client.println("Host: " + (String)Host);
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("User-Agent: Arduino/1.0");
    client.print("Content-Length: ");
    client.println(jsonString.length());
    client.println();
    client.print(jsonString);
    delay(500);
  }

  // Read and parse all the lines of the reply from server
  while (client.available())
  {
    String line = client.readStringUntil('\r');
    JSONVar myObject = JSON.parse(line);

    if (JSON.typeof(myObject) != "undefined" && myObject.hasOwnProperty("lat") && myObject.hasOwnProperty("lon"))
    {
      // Serial.print("JSON object = ");
      // Serial.println(myObject);
      Serial.print("My address: ");
      Serial.println(myObject["address"]);
      stemp = (const char *)myObject["address"];
      lat = (double)myObject["lat"];
      lon = (double)myObject["lon"];
      Serial.print("Latitude: ");
      Serial.println(lat);
      Serial.print("Longitude: ");
      Serial.println(lon);

      //      tokens = tokenize(stemp,',');
      //     city = tokens[0].c_str();
      //     city.trim();
      //     countryCode = tokens[3].c_str();
      //    countryCode.trim();
      //    zipCode = tokens[4].c_str();
      //     zipCode.trim();
    }
  }
  client.stop();
}

/*
void getWeather(){
String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + apikeyweather;
String jsonBuffer;
  jsonBuffer = httpGETRequest(serverPath.c_str());
  //Serial.println(jsonBuffer);
  JSONVar myObject = JSON.parse(jsonBuffer);
  String temp;
  if (JSON.typeof(myObject) != "undefined" && myObject.hasOwnProperty("main"))
  {
    Serial.print("JSON object = ");
    Serial.println(myObject);

    Serial.print("Timezone: ");
    Serial.println(myObject["timezone"]);
    ITimezone = (int) myObject["timezone"];
    Serial.println(ITimezone);

    Serial.print("Temperature: ");
    Serial.println(myObject["main"]["temp"]);
    Temperature = (double) myObject["main"]["temp"];
    Serial.println(Temperature);
    Serial.print("Pressure: ");
    Serial.println(myObject["main"]["pressure"]);
    Pressure = (int) myObject["main"]["pressure"];
    Serial.println(Pressure);
    Serial.print("Humidity: ");
    Serial.println(myObject["main"]["humidity"]);
    Humidity = (int) myObject["main"]["humidity"];
    Serial.println(Humidity);
    Serial.print("Wind Speed: ");
    Serial.println(myObject["wind"]["speed"]);
    Wind = (double) (myObject["wind"]["speed"]);
    Serial.println(Wind);
    weather_description = (const char *)myObject["weather"][0]["description"];
    Serial.println(weather_description);
    weather_icon = (const char *)myObject["weather"][0]["icon"];
  }
}
*/

void setTimezone(String timezone)
{
  Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
  setenv("TZ", timezone.c_str(), 1); //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone)
{
  struct tm timeinfo;
  unsigned long elapse = millis();
  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org"); // First connect to NTP server, with 0 TZ offset
  bool tsuccess = false;
  while (millis() - elapse < 5000)
  { // Timeout of 5 seconds
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("  Failed to obtain time");
      delay(100);
    }
    else
    {
      Serial.println("  Got the time from NTP");
      setTimezone(timezone);
      tsuccess = true;
      break;
    }
  }
  if (!tsuccess)
  {
    Serial.println("ntp timed out");
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time 1");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

void gps_stuff()
{
  if (Serial2.available() > 0)
  {
    if (gps.encode(Serial2.read()))
    {
      if (gps.location.isValid())
      {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat());

        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng());

        Serial.print(F("- altitude: "));

        if (gps.altitude.isValid())
        {
          Serial.println(gps.altitude.meters());
        }

        else
        {
          Serial.println(F("INVALID"));
        }
      }

      else
      {
        Serial.println(F("- location: INVALID"));
      }

      Serial.print(F("- speed: "));

      if (gps.speed.isValid())
      {
        Serial.print(gps.speed.kmph());
        Serial.println(F(" km/h"));
      }

      else
      {
        Serial.println(F("INVALID"));
      }

      Serial.print(F("- GPS date&time: "));

      if (gps.date.isValid() && gps.time.isValid())
      {
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
      }

      else
      {
        Serial.println(F("INVALID"));
      }

      Serial.println();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}

void get_ip(void *parameters)
{
  unsigned long now = 0;
  while (true)
  {
    if (millis() - now > 60 * 1000)
    {
      now = millis();
      getIp();
    }
    else
      vTaskDelay(5 * 1000 / portTICK_PERIOD_MS);
  }
}

void solar_calculations(struct tm timeinfo, String timezone){
  double timezone_double = timezone;
  double frac_year = ((2*M_PI)/365.0)*(timeinfo.tm_yday-1+((timeinfo.tm_hour-12)/24));
  double time_equation = 229.18*(0.000075 + 0.001868*cos(frac_year) - 0.032077*sin(frac_year) - 0.014615*cos(2*frac_year) - 0.040849*sin(2*frac_year));
  double declination_angle = 0.006918- 0.399912*cos(frac_year) + 0.070257*sin(frac_year) - 0.006758*cos(2*frac_year) + 0.000907*sin(2*frac_year) - 0.002697*cos(3*frac_year) + 0.00148*sin(3*frac_year);
  double time_offset = time_equation + 4 * lon - 60 * timezone;
  double tst = timeinfo.tm_hour * 60 + timeinfo.tm_min + timeinfo.tm_sec/60 + time_offset;
  double solar_hour_angle = acos(((cos(90.833))/(cos(lat)*cos(declination_angle)))-(tan(lat)*tan(declination_angle)));
  double sunrise_or_set = 720 - 4*(lon + solar_hour_angle) - time_equation;
  double solar_noon = 720 - 4*lon - time_equation;
}

void setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin("NETGEAR23", "rustictree335");
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RX, TX);
  Serial2.begin(9600); // Default baud of gps module
  Serial.println(F("Arduino - GPS module"));
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  initTime("EST5EDT,M3.2.0,M11.1.0");
  getIp();
  xTaskCreate(get_ip, "get_ip", 10 * 1024, NULL, tskIDLE_PRIORITY, &ip_handle);
  delay(5000);
  Serial.println("getLocation() function:");
  getLocation();
  solar_calculations();
}

void loop()
{
}

/*
__float128 x = 0; //use this for solar calculations
*/

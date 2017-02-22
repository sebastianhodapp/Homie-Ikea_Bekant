#include <Homie.h>
#include <Ultrasonic.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "hardware.h"
#include "FS.h"

// Defaults
const int DEFAULT_maxHeight     = 150;
const int DEFAULT_minHeight     = 50;
const int DEFAULT_sensorOffset  = 0;
const int DEFAULT_tempInterval  = 60000; //in ms
const int DEFAULT_sonicInterval = 500;   //in ms

int CURR_HEIGHT =  0;
int SET_HEIGHT  =  0;

const int TimeOut    =  58 * 125;

bool  activated       = false;
int   direction       = 0;
int   toggle_counter  = 0;

unsigned long previousMillis, previousMillis2 = 0;

static char celsiusTemp[7];
static char humidityTemp[7];
static char dtostrfbuffer[7];

HomieSetting<long> maxHeightSetting("maxHeight", "Maximum Table Height");
HomieSetting<long> minHeightSetting("minHeight", "Minimum Table Height");
HomieSetting<long> sensorOffsetSetting("sensorOffset", "Offset of Sensor compared to Table Height");
HomieSetting<long> tempIntervalSetting("tempInterval", "Interval for Temperature update");
HomieSetting<long> sonicIntervalSetting("sonicInterval", "Interval for Sonic Sensor reading");

HomieNode   positionNode("position", "position");
HomieNode   climateNode("climate", "climate");

Ultrasonic  ultrasonic(triggerPin,echoPin,TimeOut); // (Trig PIN,Echo PIN)
DHT         dht(dhtPin, DHTTYPE);


void loopHandler() {

  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis2) >= 500) {
    previousMillis2 = currentMillis;
    int tmp_height = ultrasonic.Ranging(CM); // + sensorOffsetSetting.get(); // Read sensor and correct reading by the sensor offset

    if (CURR_HEIGHT != tmp_height){
      CURR_HEIGHT = tmp_height;
      Homie.getLogger() << "CURR_HEIGHT: " << CURR_HEIGHT << ", SET_HEIGHT: " << SET_HEIGHT << ", toggle_counter: " << toggle_counter<< ", activated: " << activated<< endl;
    }

    if ((CURR_HEIGHT > (SET_HEIGHT-2)) && activated && (toggle_counter < max_reverse_toggles)){
      Homie.getLogger() << "Table too height. Moving down." << endl;
      positionNode.setProperty("currheight").send(String(CURR_HEIGHT));

      digitalWrite(downPin, HIGH);
      digitalWrite(upPin, LOW);
      if (direction > 0){
        toggle_counter++;
      }
      direction = -1;

    } else if ((CURR_HEIGHT < (SET_HEIGHT-2)) && activated && (toggle_counter < max_reverse_toggles)) {
      Homie.getLogger() << "Table too low. Moving up." << endl;
      positionNode.setProperty("currheight").send(String(CURR_HEIGHT));

      digitalWrite(downPin, LOW);
      digitalWrite(upPin, HIGH);
      if (direction < 0){
        toggle_counter++;
     }
     direction = 1;

    } else {
      digitalWrite(downPin, LOW);
      digitalWrite(upPin, LOW);
      activated = false;
      direction = 0;
      toggle_counter = 0;
    }
  }

  if ((currentMillis - previousMillis) >= tempIntervalSetting.get()) {
    previousMillis = currentMillis;

    float h = dht.readHumidity();
    float t = dht.readTemperature(); // Read temperature as Celsius (the default) (true) = fahrenheit

    // Reading & Updating climate
    if (isnan(h) || isnan(t)) {
      Homie.getLogger() << "DHT sensor read failed" << endl;
      strcpy(celsiusTemp,"Failed");
      strcpy(humidityTemp, "Failed");

    } else {
      float hic = dht.computeHeatIndex(t, h, false);
      dtostrf(hic, 6, 2, celsiusTemp);
      dtostrf(h, 6, 2, humidityTemp);

      Homie.getLogger() << "DHT Humidity: " << h << " %" << endl;
      climateNode.setProperty("humidity").send(dtostrf(h, 5, 2, dtostrfbuffer));

      Homie.getLogger() << "DHT Temperature: " << t << " °C" << endl;
      climateNode.setProperty("temperature").send(dtostrf(t, 5, 2, dtostrfbuffer));

      Homie.getLogger() << "DHT Heat Index: " << hic << endl;
      climateNode.setProperty("heat").send(dtostrf(hic, 5, 2, dtostrfbuffer));
    }
  }
} // end loopHandler

bool heightSetHandler(const HomieRange& range, const String& value){
  Homie.getLogger() << "Height set to " << value << endl;
  positionNode.setProperty("setheight").send(value);

  SET_HEIGHT = value.toInt();
  activated = true;
} // end heightSetHandler

void setup() {
  // Initialize
  Serial.begin(115200);
  Serial << endl << endl;

  // Set both directions to low
  pinMode(upPin, OUTPUT);
  pinMode(downPin, OUTPUT);
  digitalWrite(downPin, HIGH);
  digitalWrite(upPin, HIGH);

  // Initialize DHT Sensor
  dht.begin();
  Homie.getLogger() << "DHT sensor started"<< endl;

  // Set height initially to actual height
  SET_HEIGHT  = ultrasonic.Ranging(CM);
  CURR_HEIGHT = SET_HEIGHT;
  Homie.getLogger() << "Current table height: " << SET_HEIGHT << endl;

  //Homie.disableLogging(); // before Homie.setup()

  Homie_setFirmware("ikea-bekant", "1.0.0");
  Homie.setLoopFunction(loopHandler);

  // Table position
  positionNode.advertise("currheight");
  positionNode.advertise("maxheight");
  positionNode.advertise("minheight");
  positionNode.advertise("setheight").settable(heightSetHandler);
  positionNode.advertise("unit");
  positionNode.setProperty("unit").send("cm");

  // Room climate
  climateNode.advertise("temperature");
  climateNode.advertise("humidity");
  climateNode.advertise("heat");
  climateNode.advertise("unit");
  climateNode.setProperty("unit").send("c");


  HomieSetting<long> maxHeightSetting("maxHeight", "Maximum Table Height");
  HomieSetting<long> minHeightSetting("minHeight", "Minimum Table Height");
  HomieSetting<long> sensorOffsetSetting("sensorOffset", "Offset of Sensor compared to Table Height");
  HomieSetting<long> tempIntervalSetting("tempInterval", "Interval for Temperature update");
  HomieSetting<long> sonicIntervalSetting("sonicInterval", "Interval for Sonic Sensor Reading");

  maxHeightSetting.setDefaultValue(DEFAULT_maxHeight).setValidator([] (long candidate) {
    return candidate > 0;
  });
  minHeightSetting.setDefaultValue(DEFAULT_minHeight).setValidator([] (long candidate) {
    return candidate > 0;
  });
  sensorOffsetSetting.setDefaultValue(DEFAULT_maxHeight).setValidator([] (long candidate) {
    return candidate > 0 || candidate < 0 || candidate == 0;
  });
  tempIntervalSetting.setDefaultValue(DEFAULT_tempInterval).setValidator([] (long candidate) {
    return candidate > 0;
  });
  sonicIntervalSetting.setDefaultValue(DEFAULT_sonicInterval).setValidator([] (long candidate) {
    return candidate > 0;
  });

  Homie_setBrand("IKEA-bekant");
  Homie.setup();
} // end setup

void loop() {
  Homie.loop();
} //end loop

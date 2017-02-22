// Hardware Configuration
// Adjust pins to fit the hardware

// Ultrasonic Sensor
#define echoPin D7
#define triggerPin D6

// DHT Sensor
#define dhtPin D5
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22     // DHT 22  (AM2302), AM2321

// Optokoplers for Table control
#define upPin D1
#define downPin D2

#define max_reverse_toggles 3 // Depending on the speed of movement table could get in an endless loop between up/down. How often should the try to reach the exact height?

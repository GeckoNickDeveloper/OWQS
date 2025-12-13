/****************************************/
/*                CONFIGS               */
/****************************************/

// Import configs
#include "owqs_config.h"



/****************************************/
/*                DEFINES               */
/****************************************/

//// Set serial for monitor (to monitor)
#define SerialMon Serial
//// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1



/****************************************/
/*                IMPORTS               */
/****************************************/
// - GSM
#include <TinyGsmClient.h>

// - Temperature 
#include <OneWire.h>
#include <DallasTemperature.h>

// - InfluxDB
#include <InfluxDbClient.h>

// - Watchdog
#include "esp_task_wdt.h"



/****************************************/
/*               VARIABLES              */
/****************************************/
// - GSM
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

// - Temperature
OneWire oneWire(OWQS_GPIO_SENSOR_TEMPERATURE);  // OneWire bus
DallasTemperature temperatureSensor(&oneWire);  // DS18B20 sensor

// - Measurements
float temperature;                              // Temperature measurement (°C)
float pH;                                       // pH measurement
float turbidity;                                // Turbidity measurement



/****************************************/
/*                 Setup                */
/****************************************/

void setup() { app_main(); }
void loop() { /* Useless */ }



/****************************************/
/*          Utility Functions           */
/****************************************/

/***************************/
/*      Sleep Function     */
/***************************/

void owqs_light_sleep(unsigned int millis) {
  // Enable esp sleep timer
  esp_sleep_enable_timer_wakeup(millis * 1000);
  
  // Start light sleep
  esp_light_sleep_start();
}

void owqs_deep_sleep(unsigned int millis) {
  // Enable esp sleep timer
  esp_sleep_enable_timer_wakeup(millis * 1000);
  
  // Start deep sleep
  esp_deep_sleep_start();
}



/***************************/
/*    Network Functions    */
/***************************/

void owqs_net_connect_modem() {
  SerialMon.print("[OWQS] Connecting to APN: ");
  SerialMon.print(OWQS_SIM_APN);
  if (!modem.gprsConnect(OWQS_SIM_APN, OWQS_SIM_GRPS_USR, OWQS_SIM_GRPS_PWD)) {
    SerialMon.println(" fail");
    
    while(1);   // To trigger the watchdog
  } else {
    SerialMon.println(" OK");
  }
}

void owqs_net_disconnect_modem() {
  SerialMon.print("[OWQS] Disconnecting modem...");
  modem.gprsDisconnect();
}



void owqs_net_connect_client() {
  SerialMon.print("[OWQS] Connecting to server: ");
  SerialMon.print(OWQS_INFLUXDB_HOST);
  if (!client.connect(OWQS_INFLUXDB_HOST, OWQS_INFLUXDB_PORT)) {
    SerialMon.println(" fail");
    
    while(1);   // To trigger the watchdog
  } else {
    SerialMon.println(" OK");
  }
}

void owqs_net_disconnect_client() {
  SerialMon.print("[OWQS] Disconnecting client...");
  client.stop();
}



void owqs_net_send_data() {
  // Build data point
  Point sensors(OWQS_INFLUXDB_MEASUREMENT);

  sensors.clearTags();
  sensors.addTag(OWQS_INFLUXDB_TAG1_KEY, OWQS_INFLUXDB_TAG1_VAL);
  sensors.addTag(OWQS_INFLUXDB_TAG2_KEY, OWQS_INFLUXDB_TAG2_VAL);
  sensors.addTag(OWQS_INFLUXDB_TAG3_KEY, OWQS_INFLUXDB_TAG3_VAL);

  sensors.clearFields();
  sensors.addField("temperature", temperature);
  sensors.addField("pH", pH);
  sensors.addField("turbidity", turbidity);
  
  // Connect client
  owqs_net_connect_client();
  
  // Logging
  SerialMon.print("Writing: ");
  SerialMon.println(sensors.toLineProtocol());

  // Build POST request
  String payload = sensors.toLineProtocol() + " ";

  String httpRequest = "";
  httpRequest += String("POST ") + "/api/v2/write?org=" + OWQS_INFLUXDB_ORG + "&bucket=" + OWQS_INFLUXDB_BUCKET + " HTTP/1.1\r\n";
  httpRequest += String("Host: ") + OWQS_INFLUXDB_HOST + "\r\n";
  httpRequest += "Content-Type: text/plain; charset=utf-8\r\n";
  httpRequest += String("Authorization: Token ") + OWQS_INFLUXDB_TOKEN + "\r\n";
  httpRequest += "Accept: application/json\r\n";
  httpRequest += String("Content-Length: ") + payload.length();
  httpRequest += "\r\n";
  httpRequest += "\r\n";
  httpRequest += payload;
  httpRequest += "\r\n";

  // Logging
  SerialMon.println("________________________________");
  SerialMon.println(httpRequest);
  SerialMon.println("________________________________");

  // Send the data
  client.print(httpRequest);

  // Debugging
  // unsigned long timeout = millis();
  // while (client.connected() && millis() - timeout < 5000L) {
  //   // Print available data (HTTP response from server)
  //   while (client.available()) {
  //     char c = client.read();
  //     SerialMon.print(c);
  //     timeout = millis();
  //   }
  // }
}



/***************************/
/*    Sensors Functions    */
/***************************/

float owqs_sensors_read_temperature() { 
  temperatureSensor.requestTemperatures(); 
  return temperatureSensor.getTempCByIndex(0);
}

float owqs_sensors_read_ph(float temperature) {
  float voltage = analogRead(OWQS_GPIO_SENSOR_PH) * (3.3 / 4096.0); // in mV 
  
  // Slope computed as:
  // (pH14 - pH0) / (VpH14 - VpH0)
  //   = (14 - 0) / (0 - 3.0)
  //   = 14 / -3.0
  //   = -4.6666667
  float theoreticalSlope25 = -4.6666667f;

  // Temperature compensation (requires Kelvins)
  float temperatureCompensatedSlope = theoreticalSlope25 * ((temperature + 273.15) / 298.15);

  // pH computation
  return 7.0 + (voltage - 1.5) * temperatureCompensatedSlope;
}

float owqs_sensors_read_turbidity() {
  float voltage = analogRead(OWQS_GPIO_SENSOR_TURBIDITY) * (3.3 / 4096.0); // in V
  return voltage;
}



void owqs_sensors_acquire_all() {
  // Read temperature
  temperature = owqs_sensors_read_temperature();
  
  // Read pH with temperature compensation
  pH = owqs_sensors_read_ph(temperature);

  // Read turbidity
  turbidity = owqs_sensors_read_turbidity();
}



/***************************/
/*      Init Functions     */
/***************************/

void owqs_init_network() {
  SerialMon.println("[OWQS] Initializing network");

  // Set modem reset, enable, power pins
  pinMode(OWQS_GPIO_MODEM_PWKEY, OUTPUT);
  pinMode(OWQS_GPIO_MODEM_RST, OUTPUT);
  pinMode(OWQS_GPIO_MODEM_POWER_ON, OUTPUT);
  digitalWrite(OWQS_GPIO_MODEM_PWKEY, LOW);
  digitalWrite(OWQS_GPIO_MODEM_RST, HIGH);
  digitalWrite(OWQS_GPIO_MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, OWQS_GPIO_MODEM_RX, OWQS_GPIO_MODEM_TX);
  owqs_light_sleep(3000); // 3s sleep

  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()
  //// modem.restart();
  modem.init();
  //// use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(OWQS_SIM_PIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(OWQS_SIM_PIN);
  }



  // Connect the modem
  owqs_net_connect_modem();
}

void owqs_init_watchdog() {
  // WatchDog config
  esp_task_wdt_config_t config = {
    .timeout_ms = OWQS_TIMER_WATCHDOG_S * 1000,
    .trigger_panic = true,    // Trigger panic
  };

  // Configure watchdog
  esp_task_wdt_reconfigure(&config);
}

void owqs_init_sensors() {
  // Power up the sensors (3.3V)
  SerialMon.println("[OWQS] Initializing sensors...");
  pinMode(OWQS_GPIO_SENSORS_PWR, OUTPUT);
  digitalWrite(OWQS_GPIO_SENSORS_PWR, HIGH);

  // Start the DS18B20 sensor
  temperatureSensor.begin();

  // 2 minute sleep to fully power up the sensors (specifically pH)
  owqs_light_sleep(OWQS_TIMER_SENSORS_WARM_UP_S * 1000);
}



void owqs_init() {
  // Start the Serial Monitor
  SerialMon.begin(115200);
  SerialMon.println("[OWQS] Waking up...");

  // Init sensors
  owqs_init_sensors();
  
  // Init watchdog
  owqs_init_watchdog();

  // Init network
  owqs_init_network();
}



/***************************/
/*     Deinit Functions    */
/***************************/

void owqs_deinit_net() {
  SerialMon.println("[OWQS] Deinit network");
  // Disconnect client
  owqs_net_disconnect_client();
  // Disconnect modem
  owqs_net_disconnect_modem();

  // Power down the module
  digitalWrite(OWQS_GPIO_MODEM_PWKEY, LOW);
  digitalWrite(OWQS_GPIO_MODEM_RST, LOW);
  digitalWrite(OWQS_GPIO_MODEM_POWER_ON, LOW);
}

void owqs_deinit_sensors() {
  SerialMon.println("[OWQS] Deinit sensors");
  digitalWrite(OWQS_GPIO_SENSORS_PWR, LOW);
}

void owqs_deinit() {
  // Shutdown sensors
  owqs_deinit_sensors();

  // Shutdown modem
  owqs_deinit_net();



  // Enter deep sleep
  SerialMon.println("[OWQS] Entering in deep sleep...");
  owqs_deep_sleep(OWQS_TIMER_DEEP_SLEEP_S * 1000);
}



/****************************************/
/*                  App                 */
/****************************************/

void app_main() {
  // Init
  owqs_init();

  // Read sensors
  owqs_sensors_acquire_all();

  // Send data
  owqs_net_send_data();

  // System deinit
  owqs_deinit();
}
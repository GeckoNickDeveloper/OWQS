/****************************************/
/*                DEFINES               */
/****************************************/
// - System
#define SYS_PWR_PIN          19

// - GSM
//// TTGO T-Call pins
#define MODEM_PWKEY          4
#define MODEM_RST            5
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

//// Set serial for monitor (to monitor)
#define SerialMon Serial
//// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1

//// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800       // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024   // Set RX buffer to 1Kb

// - Temperature
#define OWQS_ONEWIRE_BUS 18         //  OneWire bus pin

// - pH
#define PH_ADC_PIN 34               //  pH sensor pin - TODO: substitute the pin

// - Turbidity
#define NTU_ADC_PIN 35              //  Turbidity sensor pin - TODO: substitute the pin

// - InfluxDB
//// Secrets

// - Deep Sleep
#define TIME_TO_SLEEP  1800         // Time ESP32 will go to sleep (in seconds) 1800 seconds = 30 minutes



/****************************************/
/*                IMPORTS               */
/****************************************/
// - GSM
#include <TinyGsmClient.h>

// - Temperature 
#include <OneWire.h>
#include <DallasTemperature.h>

// - pH
#include <DFRobot_PH.h>

// - Turbidity
// NONE

// - InfluxDB
#include <InfluxDbClient.h>
#include "secret.h"

// - Watchdog
#include "esp_task_wdt.h"


/****************************************/
/*               VARIABLES              */
/****************************************/
// - GSM
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

// Connection
const char apn[]          = "iot.1nce.net"; // APN
const char gprsUsr[]      = ""; // GPRS User
const char gprsPwd[]      = ""; // GPRS Password

// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = ""; 

// - Temperature
OneWire oneWire(OWQS_ONEWIRE_BUS);              // OneWire bus
DallasTemperature temperatureSensor(&oneWire);  // DS18B20 sensor

float temperature;                              // Temperature measurement (°C)

// - pH
DFRobot_PH phSensor;                            // Gavity pH sensor /* TODO: remove */
float pH;                                       // pH measurement

// - Turbidity 
float turbidity;                                // NTU measurement

// - InfluxDB
//// Measurement point



/****************************************/
/*                 Setup                */
/****************************************/

void setup() { app_main(); }
void loop() { /* Useless */ }


/****************************************/
/*          Utility Functions           */
/****************************************/

/***************************/
/*    Network Functions    */
/***************************/

void owqs_net_connect_modem() {
  SerialMon.print("[OWQS] Connecting to APN: ");
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUsr, gprsPwd)) {
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
  SerialMon.print(SCRT_INFLUXDB_HOST);
  if (!client.connect(SCRT_INFLUXDB_HOST, SCRT_INFLUXDB_PORT)) {
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
  Point sensors(SCRT_INFLUXDB_MEASUREMENT);

  sensors.clearTags();
  sensors.addTag(SCRT_INFLUXDB_TAG1_KEY, SCRT_INFLUXDB_TAG1_VAL);
  sensors.addTag(SCRT_INFLUXDB_TAG2_KEY, SCRT_INFLUXDB_TAG2_VAL);
  sensors.addTag(SCRT_INFLUXDB_TAG3_KEY, SCRT_INFLUXDB_TAG3_VAL);

  sensors.clearFields();
  sensors.addField("temperature", temperature);
  sensors.addField("pH", pH);
  sensors.addField("turbidity", ntu);
  
  // Connect client
  owqs_net_connect_client();
  
  // Logging
  SerialMon.print("Writing: ");
  SerialMon.println(sensors.toLineProtocol());

  // Build POST request
  String payload = sensors.toLineProtocol() + " ";

  String httpRequest = "";
  httpRequest += String("POST ") + "/api/v2/write?org=" + SCRT_INFLUXDB_ORG + "&bucket=" + SCRT_INFLUXDB_BUCKET + " HTTP/1.1\r\n";
  httpRequest += String("Host: ") + SCRT_INFLUXDB_HOST + "\r\n";// ":" + INFLUXDB_PORT + "\r\n";
  httpRequest += "Content-Type: text/plain; charset=utf-8\r\n";
  httpRequest += String("Authorization: Token ") + SCRT_INFLUXDB_TOKEN + "\r\n";
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
  float voltage = analogRead(PH_ADC_PIN) * (3300.0 / 4096.0); // in mV
  
  /* TODO */
  return 0.0;
}

float owqs_sensors_read_turbidity() {
  float voltage = analogRead(NTU_ADC_PIN) * (3.3 / 4096.0); // in V
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
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()
  //// modem.restart();
  modem.init();
  //// use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }



  // Connect the modem
  owqs_net_connect_modem();
}

void owqs_init_watchdog() {
  // WatchDog config
  esp_task_wdt_config_t config = {
    .timeout_ms = 120 * 1000,     // 2 minutes
    .trigger_panic = true,        // Trigger panic
  };

  // Configure watchdog
  esp_task_wdt_reconfigure(&config);
}

void owqs_init_sensors() {
  // Power up the sensors (3.3V)
  SerialMon.println("[OWQS] Initializing sensors...");
  pinMode(SYS_PWR_PIN, OUTPUT);
  digitalWrite(SYS_PWR_PIN, HIGH);

  // Start the DS18B20 sensor
  temperatureSensor.begin();

  // 1 minute sleep to fully power up the sensors (specifically pH)
  delay(60 * 1000);
}

void owqs_init_deepsleep() {
  // Enable deep sleep timer
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000);
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

  // Init deep sleep
  owqs_init_deepsleep();
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
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, LOW);
  digitalWrite(MODEM_POWER_ON, LOW);
}

void owqs_deinit_sensors() {
  SerialMon.println("[OWQS] Deinit sensors");
  digitalWrite(SYS_PWR_PIN, LOW);
}

void owqs_deinit() {
  // Shutdown sensors
  owqs_deinit_sensors();

  // Shutdown modem
  owqs_deinit_net();



  // Enter deep sleep
  SerialMon.println("[OWQS] Entering in deep sleep...");
  esp_deep_sleep_start();
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
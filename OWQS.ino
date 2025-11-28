/****************************************/
/*                DEFINES               */
/****************************************/
// - GSM
//// TTGO T-Call pins
#define MODEM_PWKEY          4
#define MODEM_RST            5
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26

#define I2C_SDA              21
#define I2C_SCL              22

//// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1

//// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800       // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024   // Set RX buffer to 1Kb

// - Temperature
#define OWQS_ONEWIRE_BUS 4          //  OneWire bus pin

// - pH
#define PH_ADC_PIN 34               //  pH sensor pin - TODO: substitute the pin

// - Turbidity
#define NTU_ADC_PIN 35              //  Turbidity sensor pin - TODO: substitute the pin



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
// TODO


/****************************************/
/*               VARIABLES              */
/****************************************/
// - GSM
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);


// - Temperature
OneWire oneWire(OWQS_ONEWIRE_BUS);            // OneWire bus
DallasTemperature tempSensor(&oneWire);       // DS18B20 sensor

float temperature;                            // Temperature measurement (°C)

// - pH
float pH_voltage;                             // pH ADC voltage measurement
float pH_value;                               // pH measurement


// - Torbidity 
float ntu_voltage;                             // NTU ADC voltage measurement



/****************************************/
/*                 Setup                */
/****************************************/

void setup() {
  // Start the Serial Monitor
  Serial.begin(115200);
  
  // Start the DS18B20 sensor
  tempSensor.begin();
}




/****************************************/
/*                  App                 */
/****************************************/

void loop() {
  tempSensor.requestTemperatures(); 
  float temperatureC = tempSensor.getTempCByIndex(0);
  float temperatureF = tempSensor.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  Serial.print(temperatureF);
  Serial.println("ºF");
  delay(5000);
}
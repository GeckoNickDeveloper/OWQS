#ifndef OWQS_CONFIG_H
#define OWQS_CONFIG_H

/****************************************/
/*                 GPIOs                */
/****************************************/

// Sensors' power-up
#define OWQS_GPIO_SENSORS_PWR           19

// Sensors
//// Temperature
#define OWQS_GPIO_SENSOR_TEMPERATURE    18

//// pH
#define OWQS_GPIO_SENSOR_PH             34

//// Turbidity
#define OWQS_GPIO_SENSOR_TURBIDITY      35



// SIM Module
//// ESP32 SIM800L v1.4
#define OWQS_GPIO_MODEM_PWKEY           4
#define OWQS_GPIO_MODEM_RST             5
#define OWQS_GPIO_MODEM_POWER_ON        23
#define OWQS_GPIO_MODEM_TX              27
#define OWQS_GPIO_MODEM_RX              26



/****************************************/
/*                Timers                */
/****************************************/

// Sensors max warm-up time (s)
#define OWQS_TIMER_SENSORS_WARM_UP_S    120

// Watchdog (s)
#define OWQS_TIMER_WATCHDOG_S           10

// Deep sleep (s)
#define OWQS_TIMER_DEEP_SLEEP_S         1800



/****************************************/
/*              SIM Module              */
/****************************************/

// TinyGSM Library config
#define TINY_GSM_MODEM_SIM800               // Modem is SIM800
#define TINY_GSM_RX_BUFFER          1024    // Set RX buffer to 1Kb



/****************************************/
/*                Secrets               */
/****************************************/

// Import secret defines
#include "owqs_secret.h"



// InfluxDB
//// InfluxDB target
#define OWQS_INFLUXDB_HOST              OWQS_SECRET_INFLUXDB_HOST
#define OWQS_INFLUXDB_PORT              OWQS_SECRET_INFLUXDB_PORT
#define OWQS_INFLUXDB_ORG               OWQS_SECRET_INFLUXDB_ORG
#define OWQS_INFLUXDB_BUCKET            OWQS_SECRET_INFLUXDB_BUCKET
#define OWQS_INFLUXDB_TOKEN             OWQS_SECRET_INFLUXDB_TOKEN

//// InfluxDB point
////// Measurement
#define OWQS_INFLUXDB_MEASUREMENT       OWQS_SECRET_INFLUXDB_MEASUREMENT

////// Tags
#define OWQS_INFLUXDB_TAG1_KEY          OWQS_SECRET_INFLUXDB_TAG1_KEY
#define OWQS_INFLUXDB_TAG1_VAL          OWQS_SECRET_INFLUXDB_TAG1_VAL

#define OWQS_INFLUXDB_TAG2_KEY          OWQS_SECRET_INFLUXDB_TAG2_KEY
#define OWQS_INFLUXDB_TAG2_VAL          OWQS_SECRET_INFLUXDB_TAG2_VAL

#define OWQS_INFLUXDB_TAG3_KEY          OWQS_SECRET_INFLUXDB_TAG3_KEY
#define OWQS_INFLUXDB_TAG3_VAL          OWQS_SECRET_INFLUXDB_TAG3_VAL



// SIM
#define OWQS_SIM_APN                    OWQS_SECRET_SIM_APN
#define OWQS_SIM_GRPS_USR               OWQS_SECRET_SIM_GRPS_USR
#define OWQS_SIM_GRPS_PWD               OWQS_SECRET_SIM_GRPS_PWD
#define OWQS_SIM_PIN                    OWQS_SECRET_SIM_PIN





#endif // OWQS_CONFIG_H
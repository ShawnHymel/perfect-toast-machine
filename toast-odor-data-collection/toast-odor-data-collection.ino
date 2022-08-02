/**
 * Toast odor data collection
 * 
 * Will start in IDLE. No collection is performed in this mode. Press the top 
 * left button to cycle between labels while collecting:
 * 
 * IDLE: no collection
 * BACKGROUND: toasting has not started
 * TOASTING: toasting in progress
 * BURNT: An assault to the olfactory system 
 * 
 * Cycle past BURNT to start again at IDLE.
 * 
 * WARNING: You really should let the gas sensors preheat for >24 hours before
 * they are accurate. However, we can get something reasonable after about 7 min
 * of preheating. After giving power to the gas sensors, wait at least 7 min.
 * 
 * Also: the gas sensors aren't really accurate (even after preheating). They
 * should be calibrated and compensated using temperature and humidity data (see
 * their respective datasheets). That being said, we just care about relative
 * data when attempting to make classification predictions, so this should be
 * good enough.
 * 
 * Collection script:
 *   https://github.com/edgeimpulse/example-data-collection-csv/blob/main/serial-data-collect-csv.py
 * 
 * Initially based on the AI nose project by Benjamin Cabé:
 *   https://github.com/kartben/artificial-nose
 * 
 * Sensors:
 *   https://wiki.seeedstudio.com/Grove-Multichannel-Gas-Sensor-V2/
 *   https://wiki.seeedstudio.com/Grove-Temperature_Humidity_Pressure_Gas_Sensor_BME680/
 *   https://wiki.seeedstudio.com/Grove-VOC_and_eCO2_Gas_Sensor-SGP30/
 *  
 * Install the following libraries:
 *   https://github.com/Seeed-Studio/Seeed_Multichannel_Gas_Sensor/archive/master.zip
 *   https://github.com/Seeed-Studio/Seeed_BME680/archive/refs/heads/master.zip
 *   https://github.com/Seeed-Studio/SGP30_Gas_Sensor/archive/refs/heads/master.zip
 *   
 * Author: Shawn Hymel
 * Date: July 30, 2022
 * License: 0BSD (https://opensource.org/licenses/0BSD)
 */

#include <Wire.h>

#include "Multichannel_Gas_GMXXX.h"
#include "seeed_bme680.h"
#include "sensirion_common.h"
#include "sgp30.h"
#include "TFT_eSPI.h"                                 // Comes with Wio Terminal package

// Settings
#define BTN_START           0                         // 1: press button to start, 0: loop
 #define BTN_PIN            WIO_KEY_C                 // Pin that connects to the button
#define SAMPLING_FREQ_HZ    4                         // Sampling frequency (Hz)
#define SAMPLING_PERIOD_MS  1000 / SAMPLING_FREQ_HZ   // Sampling period (ms)
#define NUM_SAMPLES         8                         // 8 samples at 4 Hz is 2 seconds
#define DEBOUNCE_DELAY      30                        // Delay for debounce (ms)
#define SAMPLE_DELAY        500                       // Delay between samples (ms)

// Constants
#define BME680_I2C_ADDR     uint8_t(0x76)             // I2C address of BME680
#define PA_IN_KPA           1000.0                    // Convert Pa to KPa

// Used to remember what mode we're in
#define NUM_MODES           4
#define MODE_IDLE           0
#define MODE_BACKGROUND     1
#define MODE_TOASTING       2
#define MODE_BURNT          3

// Global objects
GAS_GMXXX<TwoWire> gas;               // Multichannel gas sensor v2
Seeed_BME680 bme680(BME680_I2C_ADDR); // Environmental sensor
TFT_eSPI tft;                         // Wio Terminal LCD

void setup() {
  
  int16_t sgp_err;
  uint16_t sgp_eth;
  uint16_t sgp_h2;

  // Initialize button
  pinMode(BTN_PIN, INPUT_PULLUP);
  
  // Start serial
  Serial.begin(115200);

  // Configure LCD
  tft.begin();
  tft.setRotation(3);
  tft.setFreeFont(&FreeSansBoldOblique18pt7b);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("IDLE", 30, 100);

  // Initialize gas sensors
  gas.begin(Wire, 0x08);

  // Initialize environmental sensor
  while (!bme680.init()) {
    Serial.println("Trying to initialize BME680...");
    delay(1000);
  }

  // Initialize VOC and eCO2 sensor
  while (sgp_probe() != STATUS_OK) {
    Serial.println("Trying to initialize SGP30...");
    delay(1000);
  }

  // Perform initial read
  sgp_err = sgp_measure_signals_blocking_read(&sgp_eth, &sgp_h2);
  if (sgp_err != STATUS_OK) {
    Serial.println("Error: Could not read signal from SGP30");
    while (1);
  }
}

void loop() {

  float gm_no2_v;
  float gm_eth_v;
  float gm_voc_v;
  float gm_co_v;
  int16_t sgp_err;
  uint16_t sgp_tvoc;
  uint16_t sgp_co2;
  
  static uint8_t mode = MODE_IDLE;
  static uint8_t btn_state;
  static uint8_t last_btn_state = HIGH;
  static unsigned long last_dbnc_time = 0;
  int btn_reading;

  static unsigned long sample_timestamp = millis();
  static unsigned long start_timestamp = millis();

  // Debounce button - see if button state has changed
  btn_reading = digitalRead(BTN_PIN);
  if (btn_reading != last_btn_state) {
    last_dbnc_time = millis();
  }

  // Debounce button - wait some time before checking the button again
  if ((millis() - last_dbnc_time) > DEBOUNCE_DELAY) {
    if (btn_reading != btn_state) {
      btn_state = btn_reading;
      
      // Only transition to new mode if button is still pressed
      if (btn_state == LOW) {
        mode = (mode + 1);
        if (mode >= NUM_MODES) {
          mode = MODE_IDLE;
        }

        // Update LCD
        tft.fillScreen(TFT_BLACK);
        switch (mode) {
          case MODE_IDLE:
            tft.drawString("IDLE", 30, 100);
            Serial.println();
            break;
          case MODE_BACKGROUND:
            tft.drawString("BACKGROUND", 30, 100);
            Serial.println("timestamp,temp,humd,pres,co2,voc1,voc2,no2,eth,co,state");
            start_timestamp = millis();
            break;
          case MODE_TOASTING:
            tft.drawString("TOASTING", 30, 100);
            break;
          case MODE_BURNT:
            tft.drawString("BURNT", 30, 100);
            break;
          default:
            break;
        }
      }
    }
  }

  // Debounce button - seriously me, stop forgetting to put this in
  last_btn_state = btn_reading;

  // Only collect if not in idle
  if (mode > MODE_IDLE) {
    if ((millis() - sample_timestamp) >= SAMPLE_DELAY) {
      sample_timestamp = millis();

      // Read from GM-X02b sensors (multichannel gas)
      gm_no2_v = gas.calcVol(gas.getGM102B());
      gm_eth_v = gas.calcVol(gas.getGM302B());
      gm_voc_v = gas.calcVol(gas.getGM502B());
      gm_co_v = gas.calcVol(gas.getGM702B());
    
      // Read BME680 environmental sensor
      if (bme680.read_sensor_data()) {
        Serial.println("Error: Could not read from BME680");
        return;
      }
    
      // Read SGP30 sensor
      sgp_err = sgp_measure_iaq_blocking_read(&sgp_tvoc, &sgp_co2);
      if (sgp_err != STATUS_OK) {
        Serial.println("Error: Could not read from SGP30");
        return;
      }

      // Print CSV data with timestamp
      Serial.print(sample_timestamp - start_timestamp);
      Serial.print(",");
      Serial.print(bme680.sensor_result_value.temperature);
      Serial.print(",");
      Serial.print(bme680.sensor_result_value.humidity);
      Serial.print(",");
      Serial.print(bme680.sensor_result_value.pressure / PA_IN_KPA);
      Serial.print(",");
      Serial.print(sgp_co2);
      Serial.print(",");
      Serial.print(sgp_tvoc);
      Serial.print(",");
      Serial.print(gm_voc_v);
      Serial.print(",");
      Serial.print(gm_no2_v);
      Serial.print(",");
      Serial.print(gm_eth_v);
      Serial.print(",");
      Serial.print(gm_co_v);
      Serial.print(",");
      Serial.print(mode);
      Serial.println();
    }
  }
}

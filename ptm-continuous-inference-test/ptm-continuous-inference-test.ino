/**
 * Tests inference on a continuously rolling window.
 * 
 * Author: Shawn Hymel
 * Date: August 18, 2022
 * License: 0BSD
 */

// Arduino libraries
#include <Wire.h>

// Edge Impulse inferencing library
#include <perfect-toast-machine_inferencing.h>

// Sensor libraries
#include "Multichannel_Gas_GMXXX.h"
#include "seeed_bme680.h"
#include "sensirion_common.h"
#include "sgp30.h"

// LCD library: comes with Wio Terminal package
#include "TFT_eSPI.h"                                 

// Settings
#define BTN_START           0                         // 1: press button to start, 0: loop
#define BTN_PIN             WIO_KEY_C                 // Pin that connects to the button
#define SAMPLING_FREQ_HZ    4                         // Sampling frequency (Hz)
#define SAMPLING_PERIOD_MS  1000 / SAMPLING_FREQ_HZ   // Sampling period (ms)
#define NUM_SAMPLES         8                         // 8 samples at 4 Hz is 2 seconds
#define DEBOUNCE_DELAY      30                        // Delay for debounce (ms)
#define SAMPLE_DELAY        500                       // Delay between samples (ms)
#define NH3_PIN             A0                        // Pin for the ammonia sensor (MQ137)
#define ADC_MAX             1024                      // 10-bit ADC
#define ADC_VOLTAGE         3.3                       // ADC voltage
#define WINDOW_LEN          EI_CLASSIFIER_RAW_SAMPLE_COUNT  // Number of readings in each window
#define NUM_CHANNELS        EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME // Number of sensor channels

// Constants
#define BME680_I2C_ADDR     uint8_t(0x76)             // I2C address of BME680
#define PA_IN_KPA           1000.0                    // Convert Pa to KPa

// Used to remember what mode we're in
#define NUM_MODES           4
#define MODE_IDLE           0
#define MODE_BACKGROUND     1
#define MODE_TOASTING       2
#define MODE_BURNT          3

// Metrics for standardizing the raw data
// {temp, humd, co2, voc1, voc2, no2, eth, co, nh3}
static const float means[] = {75.502, 8.4883, 14913.9847, 32933.7305, 2.5958, 2.2113, 2.6475, 2.1576, 2.277};
static const float std_devs[] = {23.2281, 9.0292, 18230.8385, 24429.9224, 0.3656, 0.5479, 0.3673, 0.4927, 0.593};

// Global objects
GAS_GMXXX<TwoWire> gas;               // Multichannel gas sensor v2
Seeed_BME680 bme680(BME680_I2C_ADDR); // Environmental sensor
static TFT_eSPI tft;                  // Wio Terminal LCD
static TFT_eSprite spr = TFT_eSprite(&tft); // Sprite buffer
static signal_t signal;               // Wrapper for raw input buffer
static float input_buf[WINDOW_LEN * NUM_CHANNELS];  // Rolling window of standarized sensor data

void setup() {
  
  int16_t sgp_err;
  uint16_t sgp_eth;
  uint16_t sgp_h2;
  
  // Start serial
  Serial.begin(115200);

  // Configure LCD
  tft.begin();
  tft.setRotation(3);
  spr.createSprite(TFT_HEIGHT, TFT_WIDTH);
  spr.setTextColor(TFT_WHITE);
  spr.setFreeFont(&FreeSansBoldOblique24pt7b);
  spr.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);

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

  // Assign callback function to fill buffer used for preprocessing/inference
  signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  signal.get_data = &get_signal_data;
}

void loop() {

  float gm_no2_v;
  float gm_eth_v;
  float gm_voc_v;
  float gm_co_v;
  int16_t sgp_err;
  uint16_t sgp_tvoc;
  uint16_t sgp_co2;
  float nh3_v;
  int time_to_burnt;
  
  ei_impulse_result_t result;     // Used to store inference output
  EI_IMPULSE_ERROR res;           // Return code from inference
  
  float frame_buf[NUM_CHANNELS];  // Most resent sampling of sensors
  const int final_frame_idx = (WINDOW_LEN - 1) * NUM_CHANNELS;
  static unsigned long sample_timestamp = millis();

  char str_buf[20];
  
  if ((millis() - sample_timestamp) >= SAMPLE_DELAY) {
    sample_timestamp = millis();

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
    
    // Read from GM-X02b sensors (multichannel gas)
    gm_no2_v = gas.calcVol(gas.getGM102B());
    gm_eth_v = gas.calcVol(gas.getGM302B());
    gm_voc_v = gas.calcVol(gas.getGM502B());
    gm_co_v = gas.calcVol(gas.getGM702B());

    // Read MQ137 sensor (ammonia)
    nh3_v = (analogRead(NH3_PIN) * ADC_VOLTAGE) / ADC_MAX;

    // Assign readings to buffer
    frame_buf[0] = bme680.sensor_result_value.temperature;
    frame_buf[1] = bme680.sensor_result_value.humidity;
    frame_buf[2] = sgp_co2;
    frame_buf[3] = sgp_tvoc;
    frame_buf[4] = gm_voc_v;
    frame_buf[5] = gm_no2_v;
    frame_buf[6] = gm_eth_v;
    frame_buf[7] = gm_co_v;
    frame_buf[8] = nh3_v;

    // Roll window
    for (int i = 0; i < WINDOW_LEN - 1; i++) {
      for (int j = 0; j < NUM_CHANNELS; j++) {
        input_buf[(i * NUM_CHANNELS) + j] = input_buf[((i + 1) * NUM_CHANNELS) + j];
      }
    }

    // Standardize readings and put them into rolling window
    for (int j = 0; j < NUM_CHANNELS; j++) {
      input_buf[final_frame_idx + j] = (frame_buf[j] - means[j]) / std_devs[j];
    }

    // Print the whole buffer
//    for (int i = 0; i < WINDOW_LEN * NUM_CHANNELS; i++) {
//      Serial.print(input_buf[i]);
//      Serial.print(", ");
//    }
//    Serial.println();
    
    // Perform DSP pre-processing and inference
    res = run_classifier(&signal, &result, false);

    // Print return code and how long it took to perform inference
    ei_printf("run_classifier returned: %d\r\n", res);
    ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n", 
            result.timing.dsp, 
            result.timing.classification, 
            result.timing.anomaly);

    // Print inference/prediction results
    ei_printf("Predictions:\r\n");
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
        ei_printf("%.5f\r\n", result.classification[i].value);
    }

    // Round prediction to neared second
    time_to_burnt = (int)(result.classification[0].value + 0.5);

    // Update the LCD
    sprintf(str_buf, "%i", time_to_burnt);
    spr.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
    spr.drawString(str_buf, 115, 100);
    spr.pushSprite(0, 0);
  }
}

// Callback: fill a section of the out_ptr buffer when requested
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (input_buf + offset)[i];
    }

    return EIDSP_OK;
}

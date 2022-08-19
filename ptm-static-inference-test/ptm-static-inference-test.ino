/**
 * Tests inference on a static test sample copied from Edge Impulse.
 * 
 * Author: Shawn Hymel
 * Date: August 18, 2022
 * License: 0BSD (https://opensource.org/licenses/0BSD)
 */

#include <perfect-toast-machine_inferencing.h>

// Settings
static const int debug_nn = false;

// Raw features copied from test sample (Edge Impulse > Model testing)
static float input_buf[] = {
    1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510, 1.4038, -0.8046, 0.0786, 1.1126, 0.8823, 0.9417, 0.8742, -0.2865, 0.3510
};

// Wrapper for raw input buffer
static signal_t signal;

// Setup function that is called once as soon as the program starts
void setup() {

    // Start Serial
    Serial.begin(115200);

    // Print something to the terminal
    ei_printf("Static inference test\r\n");

    // Assign callback function to fill buffer used for preprocessing/inference
    signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
    signal.get_data = &get_signal_data;
}

// Loop function that is called repeatedly after setup()
void loop() {

    ei_impulse_result_t result; // Used to store inference output
    EI_IMPULSE_ERROR res;       // Return code from inference

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

    // Wait 100 ms before running inference again
    ei_sleep(100);
}

// Callback: fill a section of the out_ptr buffer when requested
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (input_buf + offset)[i];
    }

    return EIDSP_OK;
}

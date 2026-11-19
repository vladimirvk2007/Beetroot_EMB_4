#include "AudioTools.h"
#include "AudioTools/AudioLibs/MaximilianDSP.h"

// Audio output via I2S
I2SStream out;
Maximilian maximilian(out);

// Oscillator for tone generation
maxiOsc osc;

// Filter object (optional)
maxiFilter filter;

// Frequency of the tone (440 Hz = A note)
const double TONE_FREQUENCY = 440.0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting tone generator...");

  // Audio output configuration
  auto cfg = out.defaultConfig(TX_MODE);
  cfg.is_master = true;  // ESP32 generates the clock
  cfg.pin_bck = 15;      // Bit Clock (BCK)
  cfg.pin_ws = 17;       // Word Select / LRCK
  cfg.pin_data = 16;     // Data pin
  cfg.sample_rate = 32000;
  cfg.channels = 2;
  cfg.bits_per_sample = 16;
  cfg.buffer_size = 256;

  out.begin(cfg);
  maximilian.begin(cfg);

  Serial.println("Generating 440 Hz tone...");
}

void play(float *output) {
  // Generate simple sine wave at TONE_FREQUENCY
  double tone = osc.sinewave(TONE_FREQUENCY);

  // Optional: apply low-pass filter
  double filtered = filter.lores(tone, 2000, 0.5);

  // Output to both channels (mono) with volume control (0.3 = 30%)
  output[0] = filtered * 0.3;
  output[1] = output[0];
}

void loop() {
  // Audio processing is handled by maximilian.copy()
  maximilian.copy();
}

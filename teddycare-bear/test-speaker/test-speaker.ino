// sound.cpp

#include <sound.h>

bool parseWavHeader(File &file, WavHeader &header) {
  if (file.read() != 'R' || file.read() != 'I' || file.read() != 'F' || file.read() != 'F') {
    Serial.println("Invalid WAV file");
    return false;
  }

  file.seek(22);
  header.num_channels = file.read() | (file.read() << 8);
  header.sample_rate = file.read() | (file.read() << 8) | (file.read() << 16) | (file.read() << 24);
  file.seek(34);
  header.bits_per_sample = file.read() | (file.read() << 8);
  file.seek(44);  // Skip to the audio data
  return true;
}

void setup_sound() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 44100,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 1024,
      .use_apll = false,
      .tx_desc_auto_clear = true,
  };

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCLK,
      .ws_io_num = I2S_LRC,
      .data_out_num = I2S_DOUT,
      .data_in_num = -1,
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void play_sound(std::string text, fs::FS &SD) {
  std::string filename = "/audios/" + text + ".wav";
  File file = SD.open(filename.c_str());
  if (!file) {
    Serial.println("Failed to open file for reading");
    while (1) delay(1);
  }

  WavHeader header;
  if (!parseWavHeader(file, header)) {
    Serial.println("Invalid WAV file");
    return;
  }

  i2s_set_sample_rates(I2S_NUM_0, header.sample_rate);

  const size_t buffer_size = 512;
  uint8_t buffer[buffer_size];
  size_t bytes_read;
  size_t bytes_written;
  while (file.available() && (bytes_read = file.read(buffer, buffer_size)) > 0) {
    i2s_write(I2S_NUM_0, buffer, bytes_read, &bytes_written, portMAX_DELAY);
    if (bytes_read == 0) break;
  }

  file.close();
}
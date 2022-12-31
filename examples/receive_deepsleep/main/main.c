#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_intr_alloc.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/task.h>
#include <sx127x.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

sx127x *device = NULL;
static const char *TAG = "sx127x";

void rx_callback(sx127x *device) {
  uint8_t *data = NULL;
  uint8_t data_length = 0;
  esp_err_t code = sx127x_read_payload(device, &data, &data_length);
  if (code != ESP_OK) {
    ESP_LOGE(TAG, "can't read %d", code);
    return;
  }
  if (data_length == 0) {
    // no message received
    return;
  }
  uint8_t payload[514];
  const char SYMBOLS[] = "0123456789ABCDEF";
  for (size_t i = 0; i < data_length; i++) {
    uint8_t cur = data[i];
    payload[2 * i] = SYMBOLS[cur >> 4];
    payload[2 * i + 1] = SYMBOLS[cur & 0x0F];
  }
  payload[data_length * 2] = '\0';

  int16_t rssi;
  ESP_ERROR_CHECK(sx127x_get_packet_rssi(device, &rssi));
  float snr;
  ESP_ERROR_CHECK(sx127x_get_packet_snr(device, &snr));
  int32_t frequency_error;
  ESP_ERROR_CHECK(sx127x_get_frequency_error(device, &frequency_error));

  ESP_LOGI(TAG, "received: %d %s rssi: %d snr: %f freq_error: %ld", data_length, payload, rssi, snr, frequency_error);
}

void app_main() {
  ESP_LOGI(TAG, "starting up");
  spi_bus_config_t config = {
      .mosi_io_num = MOSI,
      .miso_io_num = MISO,
      .sclk_io_num = SCK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 0,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &config, 0));
  ESP_ERROR_CHECK(sx127x_create(HSPI_HOST, SS, 8196, &device));

  esp_sleep_wakeup_cause_t cpu0WakeupReason = esp_sleep_get_wakeup_cause();
  if (cpu0WakeupReason == ESP_SLEEP_WAKEUP_EXT0) {
    sx127x_set_rx_callback(rx_callback, device);
    // as it was called from interrupt routine
    sx127x_handle_interrupt_fromisr(device);
  } else {
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, device));
    ESP_ERROR_CHECK(sx127x_set_frequency(437200012, device));
    ESP_ERROR_CHECK(sx127x_reset_fifo(device));
    ESP_ERROR_CHECK(sx127x_set_lna_boost_hf(SX127x_LNA_BOOST_HF_ON, device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, device));
    ESP_ERROR_CHECK(sx127x_set_lna_gain(SX127x_LNA_GAIN_G4, device));
    ESP_ERROR_CHECK(sx127x_set_bandwidth(SX127x_BW_125000, device));
    ESP_ERROR_CHECK(sx127x_set_implicit_header(NULL, device));
    ESP_ERROR_CHECK(sx127x_set_modem_config_2(SX127x_SF_9, device));
    ESP_ERROR_CHECK(sx127x_set_syncword(18, device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, device));
  }

  ESP_ERROR_CHECK(rtc_gpio_set_direction((gpio_num_t)DIO0, RTC_GPIO_MODE_INPUT_ONLY));
  ESP_ERROR_CHECK(rtc_gpio_pulldown_en((gpio_num_t)DIO0));
  ESP_LOGI(TAG, "entering deep sleep");
  ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup((gpio_num_t)DIO0, 1));
  esp_deep_sleep_start();
}

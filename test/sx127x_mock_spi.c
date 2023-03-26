#include <check.h>
#include <string.h>
#include <sx127x_spi.h>

uint8_t expected_response[4096];
size_t expected_response_length = 0;
size_t expected_response_current = 0;
int expected_code = 0;

uint8_t actual_request[4096];
size_t actual_request_length = 0;
int expected_write_code = 0;

uint8_t *registers;

int sx127x_spi_read_registers(int reg, void *spi_device, size_t data_length, uint32_t *result) {
  *result = 0;
  for (int i = 0; i < data_length; i++) {
    *result = ((*result) << 8);
    *result = (*result) + registers[reg + i];
  }
  return expected_code;
}

int sx127x_spi_read_buffer(int reg, uint8_t *buffer, size_t buffer_length, void *spi_device) {
  ck_assert_int_eq(expected_response_current + 1 + buffer_length >= expected_response_length, 1);
  ck_assert_int_eq(expected_response[expected_response_current], reg);
  expected_response_current++;
  memcpy(buffer, expected_response + expected_response_current, buffer_length);
  expected_response_current += buffer_length;
  return expected_code;
}

int sx127x_spi_write_register(int reg, uint8_t *data, size_t data_length, void *spi_device) {
  for (size_t i = 0; i < data_length; i++) {
    registers[reg + i] = data[i];
  }
  return expected_write_code;
}

int sx127x_spi_write_buffer(int reg, uint8_t *buffer, size_t buffer_length, void *spi_device) {
  actual_request[actual_request_length] = reg;
  actual_request_length++;
  memcpy(actual_request + actual_request_length, buffer, buffer_length);
  actual_request_length += buffer_length;
  return expected_write_code;
}

void spi_mock_registers(uint8_t *expected, int code) {
  registers = expected;
  expected_code = code;
}

void spi_mock_fifo(uint8_t *expected, size_t expected_length, int code) {
  memcpy(expected_response, expected, expected_length);
  expected_response_length = expected_length;
  expected_response_current = 0;
  expected_code = code;
}

void spi_assert_write(uint8_t *expected, size_t expected_length) {
  ck_assert_int_eq(expected_length, actual_request_length);
  for (size_t i = 0; i < expected_length; i++) {
    ck_assert_int_eq(expected[i], actual_request[i]);
  }
}

void spi_mock_write(int code) {
  expected_write_code = code;
}

#include "dtr008v2io.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dtr008v2io {

static const char *const TAG = "dtr008v2io";

void dtr008v2ioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up dtr008v2io (bit-bang)...");

  if (this->oe_pin_ != nullptr) {
    this->oe_pin_->setup();
    this->oe_pin_->digital_write(false);  // OE = LOW (enabled)
  }
  if (this->latch_pin_ != nullptr) {
    this->latch_pin_->setup();
    this->latch_pin_->digital_write(false);
  }
  if (this->data_pin_ != nullptr) this->data_pin_->setup();
  if (this->clock_pin_ != nullptr) this->clock_pin_->setup();
  if (this->load_pin_ != nullptr) this->load_pin_->setup();

  this->transfer_gpio_();
}

void dtr008v2ioComponent::loop() {
  if (this->use_inputs_) {
    this->transfer_gpio_();
  }
}

void dtr008v2ioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "dtr008v2io (bit-bang):");
}

float dtr008v2ioComponent::get_setup_priority() const { return setup_priority::IO; }

void dtr008v2ioComponent::digital_write_(uint16_t pin, bool value) {
  if (pin >= segment_pins) return;
  uint8_t current = output_byte_;
  if (value) {
    this->output_byte_ |= (1 << (7 - pin));
  } else {
    this->output_byte_ &= ~(1 << (7 - pin));
  }
  if (current != output_byte_) {
    this->transfer_gpio_();
  }
}

bool dtr008v2ioComponent::digital_read_(uint16_t pin) {
  if (pin >= segment_pins) return false;
  return (bool)(this->input_byte_ & (1 << (7 - pin)));
}

void dtr008v2ioComponent::shift_out_(uint8_t value) {
  // przesuwamy 8 bitów MSB-first do 74HC595
  for (int i = 7; i >= 0; i--) {
    bool bit = (value >> i) & 0x1;
    this->data_pin_->digital_write(bit);
    this->clock_pin_->digital_write(true);
    this->clock_pin_->digital_write(false);
  }
}

uint8_t dtr008v2ioComponent::shift_in_() {
  uint8_t value = 0;
  if (this->load_pin_ != nullptr) {
    // snapshot wejść
    this->load_pin_->digital_write(false);
    delayMicroseconds(1);
    this->load_pin_->digital_write(true);
  }
  for (int i = 0; i < 8; i++) {
    this->clock_pin_->digital_write(true);
    bool bit = this->data_pin_->digital_read();  // QH z 74HC165
    this->clock_pin_->digital_write(false);
    value <<= 1;
    if (bit) value |= 1;
  }
  return value;
}

void dtr008v2ioComponent::transfer_gpio_() {
  // przesuwamy wyjścia
  this->shift_out_(this->output_byte_);

  // impuls latch tylko jeśli zmieniły się wyjścia
  if (this->latch_pin_ != nullptr) {
    this->latch_pin_->digital_write(true);
    delayMicroseconds(1);
    this->latch_pin_->digital_write(false);
  }

  // odczyt wejść jeśli aktywne
  if (this->use_inputs_) {
    this->input_byte_ = this->shift_in_();
  }
}

bool dtr008v2ioGPIOPin::digital_read() {
  return this->parent_->digital_read_(this->pin_) != this->inverted_;
}

void dtr008v2ioGPIOPin::digital_write(bool value) {
  this->parent_->digital_write_(this->pin_, value != this->inverted_);
}

std::string dtr008v2ioGPIOPin::dump_summary() const {
  return str_snprintf("%u via dtr008v2io", 16, this->pin_);
}

}  // namespace dtr008v2io
}  // namespace esphome

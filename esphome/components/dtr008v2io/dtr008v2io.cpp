#include "dtr008v2io.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dtr008v2io {

static const char *const TAG = "dtr008v2io";

void dtr008v2ioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up dtr008v2io (bit-bang, shared latch/load)...");

  if (this->oe_pin_) {
    this->oe_pin_->setup();
    this->oe_pin_->digital_write(false);  // OE = LOW (outputs enabled)
  }

  if (this->latch_load_pin_) {
    this->latch_load_pin_->setup();
    this->latch_load_pin_->digital_write(true);  // idle HIGH: 165 shift enabled, 595 not latching
  }

  if (this->data_out_pin_) this->data_out_pin_->setup();
  if (this->data_in_pin_) this->data_in_pin_->setup();
  if (this->clock_pin_) {
    this->clock_pin_->setup();
    this->clock_pin_->digital_write(false);
  }

  // initial sync
  this->transfer_gpio_();
}

void dtr008v2ioComponent::loop() {
  if (this->use_inputs_) {
    this->transfer_gpio_();
  }
}

void dtr008v2ioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "dtr008v2io (bit-bang): shared latch/load pin");
}

float dtr008v2ioComponent::get_setup_priority() const { return setup_priority::IO; }

void dtr008v2ioComponent::digital_write_(uint16_t pin, bool value) {
  if (pin >= segment_pins) return;
  uint8_t prev = this->output_byte_;
  if (value) {
    this->output_byte_ |= (1 << (7 - pin));
  } else {
    this->output_byte_ &= ~(1 << (7 - pin));
  }
  if (prev != this->output_byte_) {
    this->transfer_gpio_();
  }
}

bool dtr008v2ioComponent::digital_read_(uint16_t pin) {
  if (pin >= segment_pins) return false;
  return (bool)(this->input_byte_ & (1 << (7 - pin)));
}

void dtr008v2ioComponent::shift_out_(uint8_t value) {
  // Shift to 74HC595, MSB first
  for (int i = 7; i >= 0; i--) {
    bool bit = (value >> i) & 0x1;
    this->data_out_pin_->digital_write(bit);
    this->clock_pin_->digital_write(true);
    this->clock_pin_->digital_write(false);
  }
}

uint8_t dtr008v2ioComponent::shift_in_(uint8_t clocks) {
  uint8_t v = 0;

  // Snapshot inputs: /PL LOW -> HIGH
  this->latch_load_pin_->digital_write(false);
  // short hold time for /PL; keep clock low
  // 200–500 ns is enough; use 1 µs for margin
  delayMicroseconds(1);
  this->latch_load_pin_->digital_write(true);

  // Now shift QH while CLOCK pulses; sample on rising or falling edge (choose consistent)
  for (uint8_t i = 0; i < clocks; i++) {
    this->clock_pin_->digital_write(true);
    bool bit = this->data_in_pin_->digital_read();
    this->clock_pin_->digital_write(false);
    v = (v << 1) | (bit ? 1 : 0);
  }
  return v;
}

void dtr008v2ioComponent::transfer_gpio_() {
  // 1) Read inputs snapshot first (if enabled)
  uint8_t read_in = 0;
  if (this->use_inputs_) {
    read_in = this->shift_in_(8);
  }

  // 2) Shift out new outputs while latch_load is HIGH
  this->shift_out_(this->output_byte_);

  // 3) Commit outputs: LATCH pulse (requires a LOW->HIGH edge)
  // Note: This LOW will reload 165, but we've already finished reading it above.
  this->latch_load_pin_->digital_write(false);
  // short pulse width; ensure SHCP is low during this transition
  delayMicroseconds(1);
  this->latch_load_pin_->digital_write(true);

  // store inputs after read
  if (this->use_inputs_) {
    this->input_byte_ = read_in;
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

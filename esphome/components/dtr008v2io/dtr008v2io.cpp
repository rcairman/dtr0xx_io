#include "dtr008v2io.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dtr008v2io {

static const char *const TAG = "dtr008v2io";

void dtr008v2ioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up dtr008v2io for %d pins...", segment_pins);

  this->oe_pin_->setup();
  this->oe_pin_->digital_write(true);  // disable outputs

  this->spi_setup();
  this->transfer_gpio_();
}

void dtr008v2ioComponent::loop() {
  if (this->use_inputs_)
    this->transfer_gpio_();
}

void dtr008v2ioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "dtr008v2io: %d inputs/outputs", segment_pins);
}

float dtr008v2ioComponent::get_setup_priority() const {
  return setup_priority::IO;
}

void dtr008v2ioComponent::digital_write_(uint16_t pin, bool value) {
  if (pin >= segment_pins) {
    ESP_LOGE(TAG, "Pin %u out of range (0-%u)", pin, segment_pins - 1);
    return;
  }

  uint32_t mask = (1UL << (segment_pins - 1 - pin));

  if (value)
    output_bits_ |= mask;
  else
    output_bits_ &= ~mask;
}

bool dtr008v2ioComponent::digital_read_(uint16_t pin) {
  if (pin >= segment_pins) {
    ESP_LOGE(TAG, "Pin %u out of range (0-%u)", pin, segment_pins - 1);
    return false;
  }

  return (bool)(input_bits_ & (1UL << (segment_pins - 1 - pin)));
}

void dtr008v2ioComponent::transfer_gpio_() {
  uint32_t new_input = 0;

  this->enable();

  for (int i = segment_count - 1; i >= 0; i--) {
    uint8_t out = uint8_t((output_bits_ >> (i * 8)) & 0xFF);
    uint8_t in = this->transfer_byte(out);
    new_input |= (uint32_t(in) << (i * 8));
  }

  this->disable();

  this->input_bits_ = new_input;
  this->oe_pin_->digital_write(false);  // enable outputs
}

bool dtr008v2ioGPIOPin::digital_read() {
  return this->parent_->digital_read_(this->pin_) != this->inverted_;
}

void dtr008v2ioGPIOPin::digital_write(bool value) {
  this->parent_->digital_write_(this->pin_, value != this->inverted_);
}

std::string dtr008v2ioGPIOPin::dump_summary() const {
  return str_snprintf("%u via dtr008v2io", 32, this->pin_);
}

}  // namespace dtr008v2io
}  // namespace esphome

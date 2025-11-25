#include "dtr008v2io.h"
#include "esphome/core/log.h"

namespace esphome {
namespace dtr008v2io {

static const char *const TAG = "dtr008v2io";

void dtr008v2ioComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up dtr008v2io...");
  // If OE pin provided, set it up and force outputs enabled (OE = LOW)
  if (this->oe_pin_ != nullptr) {
    this->oe_pin_->setup();
    // Keep outputs enabled permanently (active low OE)
    // That matches manufacturer's approach: OE always active (LOW).
    this->oe_pin_->digital_write(false);
    ESP_LOGCONFIG(TAG, "  OE pin configured (held LOW)");
  } else {
    ESP_LOGCONFIG(TAG, "  OE pin not configured (assume hardware tied LOW)");
  }

  if (this->latch_pin_ != nullptr) {
    this->latch_pin_->setup();
    this->latch_pin_->digital_write(false);
    ESP_LOGCONFIG(TAG, "  LATCH pin configured");
  } else {
    ESP_LOGE(TAG, "No LATCH pin configured! Outputs may glitch.");
  }

  this->spi_setup();

  // Perform an initial transfer to sync input_byte_ state
  this->transfer_gpio_();
}

void dtr008v2ioComponent::loop() {
  if (this->use_inputs_) {
    this->transfer_gpio_();
  }
}

void dtr008v2ioComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "dtr008v2io:");
  if (this->oe_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  OE pin configured (held LOW)");
  }
  if (this->latch_pin_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  LATCH pin configured");
  }
}

float dtr008v2ioComponent::get_setup_priority() const { return setup_priority::IO; }

void dtr008v2ioComponent::digital_write_(uint16_t pin, bool value) {
  if (pin >= segment_pins) {
    ESP_LOGE(TAG, "Invalid output pin %u (0..%d)!", pin, segment_pins - 1);
    return;
  }
  uint8_t current = output_byte_;
  // Store bits MSB-first like original implementation
  if (value) {
    this->output_byte_ |= (1 << (7 - pin));
  } else {
    this->output_byte_ &= ~(1 << (7 - pin));
  }
  // If not using inputs, transfer immediately on change
  if (!this->use_inputs_ && (current != output_byte_)) {
    this->transfer_gpio_();
  }
}

bool dtr008v2ioComponent::digital_read_(uint16_t pin) {
  if (pin >= segment_pins) {
    ESP_LOGE(TAG, "Invalid input pin %u (0..%d)!", pin, segment_pins - 1);
    return false;
  }
  return (bool)(this->input_byte_ & (1 << (7 - pin)));
}

void dtr008v2ioComponent::transfer_gpio_() {
  // Transfer sequence:
  // - enable() lowers CS (this->enable() calls spi_device_acquire and pulls CS low)
  // - transfer_byte sends the output_byte_ (MSB first)
  // - the chain of 595/165 shifts and the last byte returned is read into input_byte_
  // - disable() raises CS (latch) and releases spi device
  //
  // IMPORTANT: we DO NOT toggle OE here. OE is held LOW (outputs enabled) permanently.
  //taskENTER_CRITICAL();

  // ensure latch is low before transmission
  if (this->latch_pin_ != nullptr) {
    this->latch_pin_->digital_write(false);
  }
  // transfer_byte handles a single byte exchange; for 8-channel setup it's sufficient
  this->enable();
  this->input_byte_ = this->transfer_byte(this->output_byte_);
  this->disable();

  //taskEXIT_CRITICAL();

  // Pulse latch after transmission
  if (this->latch_pin_ != nullptr) {
 //   delayMicroseconds(2);
    this->latch_pin_->digital_write(true);
    delayMicroseconds(2);
    this->latch_pin_->digital_write(false);
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

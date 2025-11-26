#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace dtr008v2io {

class dtr008v2ioComponent : public Component {
 public:
  static constexpr uint8_t segment_pins = 8;
  dtr008v2ioComponent() = default;

  void setup() override;
  void loop() override;
  float get_setup_priority() const override;
  void dump_config() override;

  void set_oe_pin(GPIOPin *pin) { this->oe_pin_ = pin; }
  void set_latch_pin(GPIOPin *pin) { this->latch_pin_ = pin; }
  void set_data_pin(GPIOPin *pin) { this->data_pin_ = pin; }
  void set_clock_pin(GPIOPin *pin) { this->clock_pin_ = pin; }
  void set_load_pin(GPIOPin *pin) { this->load_pin_ = pin; }
  void set_use_inputs() { this->use_inputs_ = true; }

 protected:
  friend class dtr008v2ioGPIOPin;
  bool digital_read_(uint16_t pin);
  void digital_write_(uint16_t pin, bool value);
  void transfer_gpio_();
  void shift_out_(uint8_t value);
  uint8_t shift_in_();

  GPIOPin *oe_pin_{nullptr};
  GPIOPin *latch_pin_{nullptr};
  GPIOPin *data_pin_{nullptr};
  GPIOPin *clock_pin_{nullptr};
  GPIOPin *load_pin_{nullptr};
  uint8_t input_byte_{0};
  uint8_t output_byte_{0};
  bool use_inputs_{false};
};

class dtr008v2ioGPIOPin : public GPIOPin, public Parented<dtr008v2ioComponent> {
 public:
  void setup() override {}
  void pin_mode(gpio::Flags flags) override {}
  bool digital_read() override;
  void digital_write(bool value) override;
  std::string dump_summary() const override;

  void set_pin(uint16_t pin) { pin_ = pin; }
  void set_inverted(bool inverted) { inverted_ = inverted; }
  void set_flags(gpio::Flags flags) { this->flags_ = flags; }
  gpio::Flags get_flags() const override { return this->flags_; }

 protected:
  uint16_t pin_{0};
  bool inverted_{false};
  gpio::Flags flags_{gpio::FLAG_INPUT};
};

}  // namespace dtr008v2io
}  // namespace esphome

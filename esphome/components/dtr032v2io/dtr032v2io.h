#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace dtr032v2io {

class dtr032v2ioComponent : public Component,
                            public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                                  spi::CLOCK_POLARITY_LOW,
                                                  spi::CLOCK_PHASE_LEADING,
                                                  spi::DATA_RATE_4MHZ> {
 public:
  // DT-R032 = 4 segments x 8 pins = 32 I/O
  static constexpr uint8_t segment_count = 4;
  static constexpr uint8_t segment_pins = segment_count * 8;

  dtr032v2ioComponent() = default;

  void setup() override;
  void loop() override;
  float get_setup_priority() const override;
  void dump_config() override;

  void set_oe_pin(GPIOPin *pin) { this->oe_pin_ = pin; }
  void set_use_inputs() { this->use_inputs_ = true; }

 protected:
  friend class dtr032v2ioGPIOPin;
  bool digital_read_(uint16_t pin);
  void digital_write_(uint16_t pin, bool value);
  void transfer_gpio_();

  GPIOPin *oe_pin_{nullptr};
  uint32_t input_bits_{0};
  uint32_t output_bits_{0};
  bool use_inputs_{true};
};

class dtr032v2ioGPIOPin : public GPIOPin, public Parented<dtr032v2ioComponent> {
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
  uint16_t pin_{};
  bool inverted_{false};
  gpio::Flags flags_{gpio::FLAG_INPUT};
};

}  // namespace dtr032v2io
}  // namespace esphome

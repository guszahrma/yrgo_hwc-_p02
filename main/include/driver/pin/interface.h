#pragma once

/**
 * @brief Pin architecture overview
 *
 * Each device defines its own pin header under driver/pin/:
 *
 *   driver/pin/stub.h     — virtual pins for stub/test usage
 *   driver/pin/esp32s3.h  — ESP32-S3 physical pins
 *
 * Each device header provides:
 *   enum class AdcPin  : uint8_t  — ADC-capable pins for that device
 *   enum class GpioPin : uint8_t  — GPIO-capable pins for that device
 *   uint8_t     toNumber(AdcPin)   — returns physical pin number
 *   uint8_t     toNumber(GpioPin)  — returns physical pin number
 *   const char* toString(AdcPin)   — name as string
 *   const char* toString(GpioPin)  — name as string
 *
 * Compile-time safety: ADC drivers only accept AdcPin, GPIO drivers only accept GpioPin.
 * Runtime safety:      driver::pin::PhysicalPinManager (manager.h) tracks usage across
 *                      all drivers to prevent the same physical pin being claimed twice.
 */
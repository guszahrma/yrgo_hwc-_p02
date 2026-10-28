/**
 * @file the generic WiFi interface
 */
#pragma once

namespace comm::wifi
{
/**
 * @brief abstract wifi driver
 */
class Interface
{
public:
    virtual ~Interface() noexcept = default;

    /**
     * @brief Initialize the WiFi subsystem.
     */
    virtual void init() noexcept = 0;

    /**
     * @brief Connect to the configured network.
     * @return True if connection was established, false otherwise.
     */
    virtual bool connect() noexcept = 0;
};
} // namespace comm::wifi

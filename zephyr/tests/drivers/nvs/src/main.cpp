/**
 * @brief Ztest for nvs::Zephyr on native_sim, where storage_partition is in a simulated flash.
 */
#include <string>

#include <zephyr/ztest.h>

#include "driver/nvs/zephyr.h"

ZTEST_SUITE(nvs_zephyr, nullptr, nullptr, nullptr, nullptr, nullptr);

ZTEST(nvs_zephyr, test_string_can_be_read_back)
{
    driver::nvs::Zephyr nvs{"test"};
    std::string value{};

    zassert_true(nvs.setString("key", "value"));
    zassert_true(nvs.getString("key", value));
    zassert_equal(std::string{"value"}, value);
}

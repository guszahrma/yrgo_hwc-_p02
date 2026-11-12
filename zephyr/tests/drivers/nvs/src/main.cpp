/**
 * @brief Ztest for nvs::Zephyr on native_sim, where storage_partition is in a simulated flash.
 */
#include <cstdint>
#include <string>

#include <zephyr/ztest.h>

#include "driver/nvs/zephyr.h"

namespace
{
/** The namespace most tests use. */
constexpr const char* TestNamespace{"test"};

/** A second namespace, to check that namespaces are kept apart. */
constexpr const char* OtherNamespace{"other"};

/**
 * @brief Erase both namespaces before each test.
 *
 *        All tests in a run share the simulated flash, so each test starts from empty storage.
 *
 * @param[in] fixture Not used.
 */
void eraseBothNamespaces(void* fixture)
{
    (void)fixture;
    driver::nvs::Zephyr{TestNamespace}.eraseAll();
    driver::nvs::Zephyr{OtherNamespace}.eraseAll();
}
} // namespace

ZTEST_SUITE(nvs_zephyr, nullptr, nullptr, eraseBothNamespaces, nullptr, nullptr);

ZTEST(nvs_zephyr, test_string_can_be_read_back)
{
    driver::nvs::Zephyr nvs{TestNamespace};
    std::string value{};

    zassert_true(nvs.setString("key", "value"));
    zassert_true(nvs.getString("key", value));
    zassert_equal(std::string{"value"}, value);
}

ZTEST(nvs_zephyr, test_string_survives_a_new_driver)
{
    // A new driver reads the same flash, as after a restart.
    {
        driver::nvs::Zephyr nvs{TestNamespace};
        zassert_true(nvs.setString("blink_period", "300"));
    }

    driver::nvs::Zephyr nvs{TestNamespace};
    std::string value{};
    zassert_true(nvs.getString("blink_period", value));
    zassert_equal(std::string{"300"}, value);
}

ZTEST(nvs_zephyr, test_a_second_write_replaces_the_first)
{
    driver::nvs::Zephyr nvs{TestNamespace};
    std::string value{};

    zassert_true(nvs.setString("key", "first"));
    zassert_true(nvs.setString("key", "second"));
    zassert_true(nvs.getString("key", value));
    zassert_equal(std::string{"second"}, value);
}

ZTEST(nvs_zephyr, test_missing_key_is_not_found)
{
    driver::nvs::Zephyr nvs{TestNamespace};
    std::string value{"unchanged"};

    zassert_false(nvs.getString("missing", value));
}

ZTEST(nvs_zephyr, test_namespaces_are_kept_apart)
{
    driver::nvs::Zephyr test{TestNamespace};
    driver::nvs::Zephyr other{OtherNamespace};
    std::string value{};

    zassert_true(test.setString("key", "in test"));
    zassert_true(other.setString("key", "in other"));

    zassert_true(test.getString("key", value));
    zassert_equal(std::string{"in test"}, value);
    zassert_true(other.getString("key", value));
    zassert_equal(std::string{"in other"}, value);
}

ZTEST(nvs_zephyr, test_integer_can_be_read_back)
{
    driver::nvs::Zephyr nvs{TestNamespace};
    std::uint16_t value{};

    zassert_true(nvs.set<std::uint16_t>("period", 500U));
    zassert_true(nvs.get<std::uint16_t>("period", value));
    zassert_equal(500U, value);
}

ZTEST(nvs_zephyr, test_erase_key_removes_only_that_key)
{
    driver::nvs::Zephyr nvs{TestNamespace};
    std::string value{};

    zassert_true(nvs.setString("first", "1"));
    zassert_true(nvs.setString("second", "2"));
    zassert_true(nvs.eraseKey("first"));

    zassert_false(nvs.getString("first", value));
    zassert_true(nvs.getString("second", value));
    zassert_equal(std::string{"2"}, value);
}

ZTEST(nvs_zephyr, test_erase_all_keeps_other_namespaces)
{
    driver::nvs::Zephyr test{TestNamespace};
    driver::nvs::Zephyr other{OtherNamespace};
    std::string value{};

    zassert_true(test.setString("first", "1"));
    zassert_true(test.setString("second", "2"));
    zassert_true(other.setString("key", "kept"));
    zassert_true(test.eraseAll());

    zassert_false(test.getString("first", value));
    zassert_false(test.getString("second", value));
    zassert_true(other.getString("key", value));
    zassert_equal(std::string{"kept"}, value);
}

ZTEST(nvs_zephyr, test_key_longer_than_15_characters_is_rejected)
{
    driver::nvs::Zephyr nvs{TestNamespace};
    std::string value{};

    // 15 characters is the limit (as ESP-IDF's NVS), 16 is one too many.
    zassert_true(nvs.setString("abcdefghijklmno", "15"));
    zassert_false(nvs.setString("abcdefghijklmnop", "16"));
    zassert_false(nvs.getString("abcdefghijklmnop", value));
}

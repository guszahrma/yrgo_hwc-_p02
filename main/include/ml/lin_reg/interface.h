/**
 * @file Linear regression model interface
 */
#pragma once

namespace ml::lin_reg
{
class Interface
{
public:
    /**
     * @brief Destroy the Interface object
     */
    virtual ~Interface() noexcept = default;

    /**
     * @brief Predict based on the given input.
     * 
     * @param[in] input Input to predict with. 
     * @return Predicted output.
     */
    [[nodiscard]] virtual double predict(double input) const noexcept = 0;
};
} // namespace ml::lin_reg
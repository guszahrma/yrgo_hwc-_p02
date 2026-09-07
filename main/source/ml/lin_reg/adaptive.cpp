/**
 * @file Adaptive linear regression model.
 */
#include "ml/lin_reg/adaptive.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>

namespace
{
void initRandom() noexcept
{
    static bool initialized{false};
    if (initialized) { return; }
    std::srand(std::time(nullptr));
    initialized = true;
}
} // namespace

namespace ml::lin_reg
{

Adaptive::Adaptive(const Matrix1d& trainIn, const Matrix1d& trainOut) noexcept
    : myTrainOrder{}
    , myTrainIn{trainIn}
    , myTrainOut{trainOut}
    , myBias{}
    , myWeight{}
{
    const auto setCount = std::min(trainIn.size(), trainOut.size());
    if (0U == setCount)
    {
        std::fprintf(stderr, "Cannot create regression model without training data!\n");
        std::terminate();
    }

    myTrainOrder.resize(setCount);
    for (std::uint32_t i{}; i < setCount; ++i)
    {
        myTrainOrder[i] = i;
    }
}

double Adaptive::predict(const double input) const noexcept
{
    return myWeight * input + myBias;
}

void Adaptive::optimize(const double input, const double output, const double learningRate) noexcept
{
    const auto prediction = predict(input);
    const auto error      = output - prediction;
    myBias += error * learningRate;
    myWeight += error * learningRate * input;
}

double Adaptive::computePrecision() const noexcept
{
    const auto setCount = myTrainOrder.size();
    if (0U == setCount) { return 0.0; }

    double totalError{};
    for (std::size_t i{}; i < setCount; ++i)
    {
        const auto error = myTrainOut[i] - predict(myTrainIn[i]);
        totalError += std::abs(error);
    }

    const auto mae = totalError / static_cast<double>(setCount);
    return 1.0 - mae;
}

void Adaptive::shuffle() noexcept
{
    initRandom();

    for (std::uint32_t i{}; i < myTrainOrder.size(); ++i)
    {
        const auto r    = static_cast<std::uint32_t>(std::rand() % myTrainOrder.size());
        const auto temp = myTrainOrder[i];
        myTrainOrder[i] = myTrainOrder[r];
        myTrainOrder[r] = temp;
    }
}

bool Adaptive::train(const std::size_t epochCount, const double initialLearningRate,
                     const double precisionThreshold) noexcept
{
    if (0U == epochCount) { return false; }
    if ((0.0 >= initialLearningRate) || (1.0 <= initialLearningRate)) { return false; }
    if ((0.0 >= precisionThreshold) || (1.0 <= precisionThreshold)) { return false; }

    auto learningRate      = initialLearningRate;
    auto previousPrecision = computePrecision();

    constexpr std::size_t precisionCheckInterval{10U};

    for (std::size_t epoch{}; epoch < epochCount; ++epoch)
    {
        const auto savedBias   = myBias;
        const auto savedWeight = myWeight;

        shuffle();

        for (const auto index : myTrainOrder)
        {
            optimize(myTrainIn[index], myTrainOut[index], learningRate);
        }

        const auto current = computePrecision();

        if (current < previousPrecision)
        {
            myBias   = savedBias;
            myWeight = savedWeight;
            learningRate *= 0.5;
        }
        else
        {
            learningRate *= 1.05;
            if (learningRate >= 1.0) { learningRate = 0.99; }
            previousPrecision = current;
        }

        if (0U == (epoch % precisionCheckInterval))
        {
            if (current >= precisionThreshold)
            {
                std::printf("Reached %g %% precision after %zu epochs!\n",
                            current * 100.0, epoch + 1U);
                return true;
            }
        }
    }

    std::printf("Final precision: %g %% after %zu epochs\n",
                computePrecision() * 100.0, epochCount);
    return true;
}

} // namespace ml::lin_reg
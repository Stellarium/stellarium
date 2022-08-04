#include "LogarithmicSpinBox.hpp"
#include <cmath>

LogarithmicSpinBox::LogarithmicSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
}

void LogarithmicSpinBox::stepBy(const int steps)
{
    auto value = this->value();

    const auto numIterations = std::abs(steps);
    // Doing it one-by-one, to prevent overstepping the step change boundary
    for(int n=0; n < numIterations; ++n)
    {
        const double minOrder = -decimals();
        const auto order = std::max(minOrder, std::floor(std::log10(value)) - 1);
        const auto step = std::pow(10., order);
        value = steps>0 ? value+step : value-step;
    }

    setValue(value);
}

#ifndef LOGARITHMIC_SPINBOX_20210618
#define LOGARITHMIC_SPINBOX_20210618

#include <QDoubleSpinBox>

class LogarithmicSpinBox : public QDoubleSpinBox
{
public:
    LogarithmicSpinBox(QWidget* parent = nullptr);
    void stepBy(int steps) override;
};

#endif

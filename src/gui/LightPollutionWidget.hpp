#ifndef LIGHT_POLLUTION_WIDGET_20210606
#define LIGHT_POLLUTION_WIDGET_20210606

#include <memory>
#include <QWidget>
#include "ui_lightPollutionWidget.h"

class LightPollutionWidget : public QWidget
{
    Q_OBJECT
public:
    LightPollutionWidget(QWidget* parent = nullptr);
    void setup();
    void retranslate();

signals:
    void luminanceChanged(double luminance);
    void databaseUseChanged(bool enabled);

private:
    enum class Mode
    {
        // Must be in the same order as in the combobox
        AutoFromDB,
        Manual,
        ManualFromSQM,
    };
    enum class Unit
    {
        // Must be in the same order as in the combobox
        cdm2,  //!< cd/m²
        mcdm2, //!< mcd/m²
        ucdm2, //!< μcd/m²
        mpsas, //!< mag/arcsec²
    };

private:
    Unit unit() const;
    Mode mode() const;
    void populate();
    void updateSliderValue();
    void updateSpinBoxValue();
    void updateBortleScaleToolTip();
    void setMode(Mode mode);
    void setUnit(Unit unit);
    void setLuminance(double luminance);
    void onSpinboxEdited();
    void onSliderMoved(int value);
    void onUseDBPropertyChanged(const QVariant& useVar);
    void onLuminancePropertyChanged(const QVariant& lumVar);

private:
    double luminanceValue_ = 0;
    std::unique_ptr<Ui_lightPollutionWidget> ui;
};

#endif

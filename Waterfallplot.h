#ifndef WATERFALLPLOT_H
#define WATERFALLPLOT_H

#include <QWidget>

#include "WaterfallData.h"

class QwtPlot;
class QwtPlotCurve;
class QwtPlotSpectrogram;

class QLabel; //FUUUU QT

class Waterfallplot : public QWidget
{
public:
    Waterfallplot(double dXMin, double dXMax, // X bounds, fixed once for all
                  const size_t historyExtent, // Will define Y width (number of layers)
                  const size_t layerPoints,   // FFT/Data points in a single layer
                  QWidget* parent);

    // view
    void replot(bool forceRepaint = false);
    void setWaterfallVisibility(const bool bVisible);
    void setTitle(const QString& qstrNewTitle);
    void setXLabel(const QString& qstrTitle);
    void setYLabel(const QString& qstrTitle);
    QwtPlot* getCurvePlot() const { return m_plotCurve; }
    QwtPlot* getSpectrogramPlot() const { return m_plotSpectrogram; }
    // color bar must be handled in another column (of a QGridLayout)
    // to keep waterfall perfectly aligned with a curve plot !
    //void setZLabel(const QString& qstrTitle);

    // data
    bool addData(const float* const dataPtr, const size_t dataLen);
    void setRange(double dLower, double dUpper);
    void getRange(double& rangeMin, double& rangeMax) const;
    void getDataRange(double& rangeMin, double& rangeMax) const;
    void clear();
    time_t getLayerDate(const double y) const;

protected:
    QwtPlot*const             m_plotCurve;
    QwtPlot*const             m_plotSpectrogram;
    QwtPlotCurve* const       m_curve;
    QwtPlotSpectrogram* const m_spectrogram;

    // later, the type can be parametrized when instanciating Waterfallplot
    // m_pData will be owned (freed) by m_spectrogram
    // Sadly, to avoid compile problems with a templated class, the implementation
    // needs to be moved in this header file !
    WaterfallData<float>* m_data;

    bool m_bColorBarInitialized = false;

    double* m_curveXAxisData;
    double* m_curveYAxisData;

    // TODO......
    mutable bool m_inScaleSync;

private:
    //Q_DISABLE_COPY(Waterfallplot)
};

#endif // WATERFALLPLOT_

#ifndef WATERFALLPLOT_H
#define WATERFALLPLOT_H

#include <QObject>

#include <qwt_plot_spectrogram.h>

#include "WaterfallData.h"

class QwtPlot;

class Waterfallplot : public QObject
{
public:
    Waterfallplot(double dXMin, double dXMax,      // X bounds, fixed once for all
                  const size_t historyExtent,      // Will define Y width (number of layers)
                  const size_t layerPoints,        // FFT/Data points in a single layer
                  QwtPlot*const plot,              // QwtPlot in which the waterfall will be painted
                  QObject*const parent = nullptr); // Parent for garbage collection

    // view
    void replot();
    void setVisible(const bool bVisible);
    void setTitle(const QString& qstrNewTitle);
    void setXLabel(const QString& qstrTitle);
    void setYLabel(const QString& qstrTitle);
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
    QwtPlot*const       m_plot;
    QwtPlotSpectrogram& m_spectrogram;

    // later, the type can be parametrized when instanciating Waterfallplot
    // m_pData will be owned (freed) by m_spectrogram
    // Sadly, to avoid compile problems with a templated class, the implementation
    // needs to be moved in this header file !
    WaterfallData<float>* m_data;

    bool m_bColorBarInitialized = false;

private:
    Q_DISABLE_COPY(Waterfallplot)
};

#endif // WATERFALLPLOT_

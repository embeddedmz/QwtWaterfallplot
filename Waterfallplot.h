#ifndef WATERFALLPLOT_H
#define WATERFALLPLOT_H

#include <QWidget>

#include "WaterfallData.h"

class QwtPlot;
class QwtPlotCurve;
class QwtPlotSpectrogram;
class QwtPlotZoomer;

class Waterfallplot : public QWidget
{
public:
    Waterfallplot(QWidget* parent);

    void setDataDimensions(double dXMin, double dXMax, // X bounds, fixed once for all
                           const size_t historyExtent, // Will define Y width (number of layers)
                           const size_t layerPoints);  // FFT/Data points in a single layer)
    void getDataDimensions(double& dXMin,
                           double& dXMax,
                           size_t& historyExtent,
                           size_t& layerPoints) const;

    QwtPlot* getPlot() const;

    // view
    void replot(bool forceRepaint = false);
    void setWaterfallVisibility(const bool bVisible);
    void setTitle(const QString& qstrNewTitle);
    void setXLabel(const QString& qstrTitle);
    void setYLabel(const QString& qstrTitle);
    void setZLabel(const QString& qstrTitle);
    QwtPlot* getCurvePlot() const { return m_plotCurve; }
    QwtPlot* getSpectrogramPlot() const { return m_plotSpectrogram; }
    // color bar must be handled in another column (of a QGridLayout)
    // to keep waterfall perfectly aligned with a curve plot !
    //void setZLabel(const QString& qstrTitle);

    // data
    bool addData(const double* const dataPtr, const size_t dataLen, const time_t timestamp);
    void setRange(double dLower, double dUpper);
    void getRange(double& rangeMin, double& rangeMax) const;
    void getDataRange(double& rangeMin, double& rangeMax) const;
    void clear();
    time_t getLayerDate(const double y) const;

    double getOffset() const { return (m_data) ? m_data->getOffset() : 0; }

protected slots:
    void autoRescale(const QRectF& rect);

protected:
    QwtPlot*const             m_plotCurve;
    QwtPlot*const             m_plotSpectrogram;
    QwtPlotCurve* const       m_curve;
    QwtPlotSpectrogram* const m_spectrogram;
    QwtPlotZoomer*            m_zoomer;

    // later, the type can be parametrized when instanciating Waterfallplot
    // m_pData will be owned (freed) by m_spectrogram
    // Sadly, to avoid compile problems with a templated class, the implementation
    // needs to be moved in this header file !
    WaterfallData<double>* m_data = nullptr;

    bool m_bColorBarInitialized = false;

    double* m_curveXAxisData;
    double* m_curveYAxisData;

    mutable bool m_inScaleSync = false;

protected slots:
   void scaleDivChanged();

protected:
    void updateLayout();

private:
    //Q_DISABLE_COPY(Waterfallplot)

    void alignAxis(int axisId);
};

#endif // WATERFALLPLOT_

#ifndef WATERFALLPLOT_H
#define WATERFALLPLOT_H

#include <QWidget>

#include "WaterfallData.h"

class QwtPlot;
class QwtPlotCurve;
class QwtPlotMarker;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotSpectrogram;
class QwtPlotZoomer;

class Waterfallplot : public QWidget
{
public:
    Waterfallplot(QWidget* parent);
    ~Waterfallplot() override;

    void setDataDimensions(double dXMin, double dXMax, // X bounds, fixed once for all
                           const size_t historyExtent, // Will define Y width (number of layers)
                           const size_t layerPoints);  // FFT/Data points in a single layer)
    void getDataDimensions(double& dXMin,
                           double& dXMax,
                           size_t& historyExtent,
                           size_t& layerPoints) const;

    bool setMarker(const double x, const double y);

    QwtPlot* getPlot() const;

    // view
    void replot(bool forceRepaint = false);
    void setWaterfallVisibility(const bool bVisible);
    void setTitle(const QString& qstrNewTitle);
    void setXLabel(const QString& qstrTitle);
    void setYLabel(const QString& qstrTitle);
    void setZLabel(const QString& qstrTitle);
    QwtPlot* getHorizontalCurvePlot() const { return m_plotHorCurve; }
    QwtPlot* getVerticalCurvePlot() const { return m_plotVertCurve; }
    QwtPlot* getSpectrogramPlot() const { return m_plotSpectrogram; }

    // data
    bool addData(const double* const dataPtr, const size_t dataLen, const time_t timestamp);
    void setRange(double dLower, double dUpper);
    void getRange(double& rangeMin, double& rangeMax) const;
    void getDataRange(double& rangeMin, double& rangeMax) const;
    void clear();
    time_t getLayerDate(const double y) const;

    double getOffset() const { return (m_data) ? m_data->getOffset() : 0; }

public slots:
    void setPickerEnabled(const bool enabled);

protected slots:
    void autoRescale(const QRectF& rect);

    void selectedPoint(const QPointF& pt);

protected:
    QwtPlot*const             m_plotHorCurve;
    QwtPlot*const             m_plotVertCurve;
    QwtPlot*const             m_plotSpectrogram;
    QwtPlotCurve* const       m_horCurve;
    QwtPlotCurve* const       m_vertCurve;
    QwtPlotPicker* const      m_picker;
    QwtPlotPanner* const      m_panner;
    QwtPlotSpectrogram* const m_spectrogram;
    QwtPlotZoomer*            m_zoomer;

    // later, the type can be parametrized when instanciating Waterfallplot
    // m_pData will be owned (freed) by m_spectrogram
    // Sadly, to avoid compile problems with a templated class, the implementation
    // needs to be moved in this header file !
    WaterfallData<double>* m_data = nullptr;

    bool m_bColorBarInitialized = false;

    double* m_horCurveXAxisData = nullptr;
    double* m_horCurveYAxisData = nullptr;

    double* m_vertCurveXAxisData = nullptr;
    double* m_vertCurveYAxisData = nullptr;

    mutable bool m_inScaleSync = false;

    double m_markerX = 0;
    double m_markerY = 0;

protected slots:
   void scaleDivChanged();

protected:
    void updateLayout();

    void updateCurvesData();
    void freeCurvesData();

private:
    //Q_DISABLE_COPY(Waterfallplot)

    void alignAxis(int axisId);
    void alignAxisForColorBar();
};

#endif // WATERFALLPLOT_

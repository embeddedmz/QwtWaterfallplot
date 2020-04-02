#ifndef WATERFALLPLOT_H
#define WATERFALLPLOT_H

#include <QWidget>

#include "ColorMaps.h"
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
    Waterfallplot(QWidget* parent, const ColorMaps::ControlPoints& ctrlPts = ColorMaps::Jet());
    ~Waterfallplot() override;

    void setDataDimensions(double dXMin, double dXMax, // X bounds, fixed once for all
                           const size_t historyExtent, // Will define Y width (number of layers)
                           const size_t layerPoints);  // FFT/Data points in a single layer)
    void getDataDimensions(double& dXMin,
                           double& dXMax,
                           size_t& historyExtent,
                           size_t& layerPoints) const;

    bool setMarker(const double x, const double y);

    // view
    void replot(bool forceRepaint = false);
    void setWaterfallVisibility(const bool bVisible);
    void setTitle(const QString& qstrNewTitle);
    void setXLabel(const QString& qstrTitle, const int fontPointSize = 12);
    void setYLabel(const QString& qstrTitle, const int fontPointSize = 12);
    void setZLabel(const QString& qstrTitle, const int fontPointSize = 12);
    bool setColorMap(const ColorMaps::ControlPoints& colorMap);
    ColorMaps::ControlPoints getColorMap() const;
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
    QwtPlot* const            m_plotHorCurve = nullptr;
    QwtPlot* const            m_plotVertCurve = nullptr;
    QwtPlot* const            m_plotSpectrogram = nullptr;
    QwtPlotCurve*             m_horCurve = nullptr;
    QwtPlotCurve*             m_vertCurve = nullptr;
    QwtPlotPicker* const      m_picker = nullptr;
    QwtPlotPanner* const      m_panner = nullptr;
    QwtPlotSpectrogram* const m_spectrogram = nullptr;
    QwtPlotZoomer* const      m_zoomer = nullptr;
    QwtPlotMarker* const      m_horCurveMarker = nullptr;
    QwtPlotMarker* const      m_vertCurveMarker = nullptr;

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

    ColorMaps::ControlPoints m_ctrlPts;

protected slots:
   void scaleDivChanged();

protected:
    void updateLayout();

    void allocateCurvesData();
    void freeCurvesData();
    void setupCurves();
    void updateCurvesData();

private:
    //Q_DISABLE_COPY(Waterfallplot)

    void alignAxis(int axisId);
    void alignAxisForColorBar();
};

#endif // WATERFALLPLOT_

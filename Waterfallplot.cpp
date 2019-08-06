#include "Waterfallplot.h"

// Qt includes
#include <QApplication>
#include <QDateTime>
//#include <QGridLayout>
#include <QSizePolicy>
#include <QVBoxLayout>

// Qwt includes
#include <qwt_color_map.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_spectrogram.h>

// C++ STL and its standard lib includes
#include <algorithm>

namespace
{
// a color map taken from ParaView
class CoolToWarmColorMapRGB : public QwtLinearColorMap
{
public:
    CoolToWarmColorMapRGB() :
        QwtLinearColorMap(QColor(0.23137*255, 0.298039*255, 0.75294*255),
                          QColor(0.70588*255, 0.015686*255, 0.14901*255),
                          QwtColorMap::RGB)
    {
        addColorStop(0.5, QColor(0.86499*255, 0.864999*255, 0.86499*255));
    }
};

class WaterfallTimeScaleDraw: public QwtScaleDraw
{
   const Waterfallplot& m_waterfallPlot;
   mutable QDateTime m_dateTime;

public:
    WaterfallTimeScaleDraw(const Waterfallplot& waterfall) :
        m_waterfallPlot(waterfall)
    {
    }

    // make it public !
    using QwtScaleDraw::invalidateCache;

    virtual QwtText label(double v) const
    {
        time_t ret = m_waterfallPlot.getLayerDate(v);
        if (ret > 0)
        {
            m_dateTime.setTime_t(ret);
            // need something else other than time_t to have 'zzz'
            //return m_dateTime.toString("hh:mm:ss:zzz");
            return m_dateTime.toString("hh:mm:ss");
        }
        return QwtText();
    }
};
}

#include <QLabel>
Waterfallplot::Waterfallplot(double dXMin, double dXMax,
                             const size_t historyExtent,
                             const size_t layerPoints,
                             QWidget* parent) :
    QWidget(parent),
    m_plotCurve(new QwtPlot),
    m_plotSpectrogram(new QwtPlot),
    m_curve(new QwtPlotCurve),
    m_spectrogram(new QwtPlotSpectrogram),
    m_data(new WaterfallData<float>(dXMin, dXMax, historyExtent, layerPoints)),
    m_curveXAxisData(new double[layerPoints]),
    m_curveYAxisData(new double[layerPoints])
{
    m_plotCurve->setAutoReplot(false);
    m_plotSpectrogram->setAutoReplot(false);

    m_curve->attach(m_plotCurve);
    m_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    //m_curve->setTitle("");
    //m_curve->setPen(...); // pen : color + width
    m_curve->setStyle(QwtPlotCurve::Lines);
    //m_curve->setSymbol(new QwtSymbol(QwtSymbol::Style...,Qt::NoBrush, QPen..., QSize(5, 5)));

    m_spectrogram->setData(m_data);
    m_spectrogram->setRenderThreadCount(0); // use system specific thread count
    m_spectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
    m_spectrogram->attach(m_plotSpectrogram);

    // color map...
    CoolToWarmColorMapRGB* const colorLut = new CoolToWarmColorMapRGB;
    m_spectrogram->setColorMap(colorLut);

    // Auto rescale
    m_plotCurve->setAxisAutoScale(QwtPlot::xBottom,       true);
    m_plotCurve->setAxisAutoScale(QwtPlot::yLeft,         true);
    m_plotSpectrogram->setAxisAutoScale(QwtPlot::xBottom, true);
    m_plotSpectrogram->setAxisAutoScale(QwtPlot::yLeft,   true);

    // the canvas should be perfectly aligned to the boundaries of your curve.
    m_plotSpectrogram->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotSpectrogram->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotSpectrogram->plotLayout()->setAlignCanvasToScales(true);

    m_plotCurve->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotCurve->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotCurve->plotLayout()->setAlignCanvasToScales(true);

    m_plotSpectrogram->setAutoFillBackground(true);
    m_plotSpectrogram->setPalette(Qt::white);
    m_plotSpectrogram->setCanvasBackground(Qt::white);

    m_plotCurve->setAutoFillBackground(true);
    m_plotCurve->setPalette(Qt::white);
    m_plotCurve->setCanvasBackground(Qt::white);

    QwtPlotCanvas* const spectroCanvas = dynamic_cast<QwtPlotCanvas*>(m_plotSpectrogram->canvas());
    spectroCanvas->setFrameStyle(QFrame::NoFrame);

    QwtPlotCanvas* const curveCanvas = dynamic_cast<QwtPlotCanvas*>(m_plotCurve->canvas());
    curveCanvas->setFrameStyle(QFrame::NoFrame);

    // Y axis labels should represent the insert time of a layer (fft)
    m_plotSpectrogram->setAxisScaleDraw(QwtPlot::yLeft, new WaterfallTimeScaleDraw(*this));
    //m_plot->setAxisLabelRotation(QwtPlot::yLeft, -50.0);
    //m_plot->setAxisLabelAlignment(QwtPlot::yLeft, Qt::AlignLeft | Qt::AlignBottom);

    // test color bar...
    m_plotSpectrogram->enableAxis(QwtPlot::yRight);
    QwtScaleWidget* axis = m_plotSpectrogram->axisWidget(QwtPlot::yRight);
    axis->setColorBarEnabled(true);
    axis->setColorBarWidth(20);

    /*QGridLayout* const gridLayout = new QGridLayout(this);
    //gridLayout->addWidget(m_plotCurve, 0, 0);
    //gridLayout->addWidget(m_plotSpectrogram, 0, 0);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(5);*/

    QVBoxLayout* layout = new QVBoxLayout(this);
    //layout->setSpacing(6);
    //layout->setContentsMargins(11, 11, 11, 11);
    //m_plotCurve->setFixedHeight(200);
    layout->addWidget(m_plotCurve, 30);
    layout->addWidget(m_plotSpectrogram, 70);

    QSizePolicy policy;
    policy.setVerticalPolicy(QSizePolicy::Ignored);
    policy.setHorizontalPolicy(QSizePolicy::Ignored);
    m_plotCurve->setSizePolicy(policy);
    m_plotSpectrogram->setSizePolicy(policy);
    setSizePolicy(policy);

    // TODO : add align stuff (plotmatrix)

    // generate curve X axis data
    const double dx = (dXMax - dXMin) / layerPoints; // x spacing
    m_curveXAxisData[0] = dXMin;
    for (size_t x = 1u; x < layerPoints; ++x)
    {
        m_curveXAxisData[x] = m_curveXAxisData[x - 1] + dx;
    }
}

void Waterfallplot::replot(bool forceRepaint /*= false*/)
{
    if (!m_plotSpectrogram->isVisible())
    {
        // temporary solution for older Qwt versions
        QApplication::postEvent(m_plotCurve, new QEvent(QEvent::LayoutRequest));
        QApplication::postEvent(m_plotSpectrogram, new QEvent(QEvent::LayoutRequest));
    }

    m_plotCurve->replot();
    m_plotSpectrogram->replot();

    if (forceRepaint)
    {
        m_plotCurve->repaint();
        m_plotSpectrogram->repaint();
    }
}

void Waterfallplot::setWaterfallVisibility(const bool bVisible)
{
    // useful ? complete ?
    m_spectrogram->setVisible(bVisible);
}

bool Waterfallplot::addData(const float* const dataPtr, const size_t dataLen)
{
    const bool bRet = m_data->addData(dataPtr, dataLen);
    if (bRet)
    {
        // refresh curve's data
        const float* wfData = m_data->getData();
        const size_t layerPts   = m_data->getLayerPoints();
        const size_t maxHistory = m_data->getMaxHistoryLength();
        std::copy(wfData + layerPts * (maxHistory - 1),
                  wfData + layerPts * maxHistory,
                  m_curveYAxisData);
        m_curve->setRawSamples(m_curveXAxisData, m_curveYAxisData, layerPts);

        // refresh spectrogram content and Y axis labels
        m_spectrogram->invalidateCache();
        auto const yLeftAxis = static_cast<WaterfallTimeScaleDraw*>(
                    m_plotSpectrogram->axisScaleDraw(QwtPlot::yLeft));
        yLeftAxis->invalidateCache();
    }
    return bRet;
}

void Waterfallplot::setRange(double dLower, double dUpper)
{
    if (dLower > dUpper)
    {
        std::swap(dLower, dUpper);
    }

    // the color bar must be handled in another column (of a QGridLayout)
    // to keep waterfall perfectly aligned with a curve plot !
    if (m_plotSpectrogram->axisEnabled(QwtPlot::yRight))
    {
        m_plotSpectrogram->setAxisScale(QwtPlot::yRight, dLower, dUpper);

        QwtScaleWidget* axis = m_plotSpectrogram->axisWidget(QwtPlot::yRight);
        if (axis->isColorBarEnabled())
        {
            // Waiting a proper method to get a reference to the QwtInterval
            // instead of resetting a new color map to the axis !
            QwtColorMap* colorMap;
            if (m_bColorBarInitialized)
            {
                colorMap = const_cast<QwtColorMap*>(axis->colorMap());
            }
            else
            {
                colorMap = new CoolToWarmColorMapRGB;
                m_bColorBarInitialized = true;
            }
            axis->setColorMap(QwtInterval(dLower, dUpper), colorMap);
        }
    }

    m_data->setRange(dLower, dUpper);
    m_spectrogram->invalidateCache();
}

void Waterfallplot::getRange(double& rangeMin, double& rangeMax) const
{
    m_data->getRange(rangeMin, rangeMax);
}

void Waterfallplot::getDataRange(double& rangeMin, double& rangeMax) const
{
    m_data->getDataRange(rangeMin, rangeMax);
}

void Waterfallplot::clear()
{
    m_data->clear();
}

time_t Waterfallplot::getLayerDate(const double y) const
{
    return m_data->getLayerDate(y);
}

void Waterfallplot::setTitle(const QString& qstrNewTitle)
{
    m_plotSpectrogram->setTitle(qstrNewTitle);
}

void Waterfallplot::setXLabel(const QString& qstrTitle)
{
    m_plotSpectrogram->setAxisTitle(QwtPlot::xBottom, qstrTitle);
}

void Waterfallplot::setYLabel(const QString& qstrTitle)
{
    m_plotSpectrogram->setAxisTitle(QwtPlot::yLeft, qstrTitle);
}

// color bar must be handled in another column (of a QGridLayout)
// to keep waterfall perfectly aligned with a curve plot !
/*void Waterfallplot::setZLabel(const QString& qstrTitle)
{
    m_plot->setAxisTitle(QwtPlot::yRight, qstrTitle);
}*/

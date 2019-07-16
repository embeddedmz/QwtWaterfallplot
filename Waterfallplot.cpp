#include "Waterfallplot.h"

#include <QApplication>
#include <QDateTime>

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>

#include <algorithm>

#include <qwt_color_map.h>
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

Waterfallplot::Waterfallplot(double dXMin, double dXMax,
                             const size_t historyExtent,
                             const size_t layerPoints,
                             QwtPlot*const plot,
                             QObject*const parent /*= nullptr*/) :
    QObject(parent),
    m_plot(plot),
    m_spectrogram(*new QwtPlotSpectrogram),
    m_data(new WaterfallData<float>(dXMin, dXMax, historyExtent, layerPoints))
{
    m_plot->setAutoReplot(false);

    m_spectrogram.setData(m_data);

    m_spectrogram.setRenderThreadCount(0); // use system specific thread count
    m_spectrogram.setCachePolicy(QwtPlotRasterItem::PaintCache);

    // color map...
    CoolToWarmColorMapRGB* const colorLut = new CoolToWarmColorMapRGB;
    m_spectrogram.setColorMap(colorLut);

    m_spectrogram.attach(plot);

    // Auto rescale
    m_plot->setAxisAutoScale(QwtPlot::xBottom, true);
    m_plot->setAxisAutoScale(QwtPlot::yLeft,   true);

    // the canvas should be perfectly aligned to the boundaries of your curve.
    m_plot->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plot->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plot->plotLayout()->setAlignCanvasToScales(true);

    m_plot->setAutoFillBackground(true);
    m_plot->setPalette(Qt::white);
    m_plot->setCanvasBackground(Qt::white);

    QwtPlotCanvas* const canvas = dynamic_cast<QwtPlotCanvas*>(m_plot->canvas());
    canvas->setFrameStyle(QFrame::NoFrame);

    // Y axis labels should represent the insert time of a layer (fft)
    m_plot->setAxisScaleDraw(QwtPlot::yLeft, new WaterfallTimeScaleDraw(*this));
    //m_plot->setAxisLabelRotation(QwtPlot::yLeft, -50.0);
    //m_plot->setAxisLabelAlignment(QwtPlot::yLeft, Qt::AlignLeft | Qt::AlignBottom);

    // test color bar...
    m_plot->enableAxis(QwtPlot::yRight);
    QwtScaleWidget* axis = m_plot->axisWidget(QwtPlot::yRight);
    axis->setColorBarEnabled(true);
    axis->setColorBarWidth(20);
}

void Waterfallplot::replot()
{
    if (!m_plot->isVisible())
    {
        // temporary solution for older Qwt versions
        QApplication::postEvent(m_plot, new QEvent(QEvent::LayoutRequest));
    }
    m_plot->replot();
}

void Waterfallplot::setVisible(const bool bVisible)
{
    return m_spectrogram.setVisible(bVisible);
}

bool Waterfallplot::addData(const float* const dataPtr, const size_t dataLen)
{
    const bool bRet = m_data->addData(dataPtr, dataLen);
    if (bRet)
    {
        m_spectrogram.invalidateCache();
        static_cast<WaterfallTimeScaleDraw*>(m_plot->axisScaleDraw(QwtPlot::yLeft))->invalidateCache();
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
    if (m_plot->axisEnabled(QwtPlot::yRight))
    {
        m_plot->setAxisScale(QwtPlot::yRight, dLower, dUpper);

        QwtScaleWidget* axis = m_plot->axisWidget(QwtPlot::yRight);
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
    m_spectrogram.invalidateCache();
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
    m_plot->setTitle(qstrNewTitle);
}

void Waterfallplot::setXLabel(const QString& qstrTitle)
{
    m_plot->setAxisTitle(QwtPlot::xBottom, qstrTitle);
}

void Waterfallplot::setYLabel(const QString& qstrTitle)
{
    m_plot->setAxisTitle(QwtPlot::yLeft, qstrTitle);
}

// color bar must be handled in another column (of a QGridLayout)
// to keep waterfall perfectly aligned with a curve plot !
/*void Waterfallplot::setZLabel(const QString& qstrTitle)
{
    m_plot->setAxisTitle(QwtPlot::yRight, qstrTitle);
}*/

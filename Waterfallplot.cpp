#include "Waterfallplot.h"

// Qt includes
#include <QApplication>
#include <QDateTime>
#include <QGridLayout>
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
#include <qwt_plot_zoomer.h>

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

//TODO: give it a nicer name
class LinearColorMapRGB : public QwtLinearColorMap
{
public:
    LinearColorMapRGB() :
        QwtLinearColorMap(Qt::darkBlue, Qt::darkRed, QwtColorMap::RGB)
      // Qt bug (according to Uwe): using QwtColorMap::Indexed will crash the app
      // QwtColorMap::RGB in version >= 6.2 is discretized unlike the previous versions !
    {
        addColorStop(0.2, Qt::blue);
        addColorStop(0.4, Qt::cyan);
        addColorStop(0.6, Qt::yellow);
        addColorStop(0.8, Qt::red);
    }
};

class MyZoomer: public QwtPlotZoomer
{
    QwtPlotSpectrogram* m_spectro;
    const Waterfallplot& m_waterfallPlot; // to retrieve time
    mutable QDateTime m_dateTime;

public:
    MyZoomer( QWidget* canvas, QwtPlotSpectrogram* spectro, const Waterfallplot& waterfall ):
        QwtPlotZoomer( canvas ),
        m_spectro( spectro ),
        m_waterfallPlot( waterfall )
    {
        Q_ASSERT(m_spectro);
        setTrackerMode( AlwaysOn );
    }

    virtual QwtText trackerTextF( const QPointF& pos ) const
    {
        QColor bg( Qt::white );
        bg.setAlpha( 200 );

        const double distVal = pos.x();
        QwtText text;
        if (m_spectro->data())
        {
            QString date;
            const double histVal = pos.y();
            time_t timeVal = m_waterfallPlot.getLayerDate(histVal);
            if (timeVal > 0)
            {
                m_dateTime.setTime_t(timeVal);
                date = m_dateTime.toString("dd.MM.yy - hh:mm:ss");
            }

            const double tempVal = m_spectro->data()->value(pos.x(), pos.y());
            text = QString("%1m, %2: %3°C").arg(distVal).arg(date).arg(tempVal);
        }
        else
        {
            text = QString("%1m: -°C").arg(distVal);
        }

        text.setBackgroundBrush( QBrush( bg ) );
        return text;
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
        time_t ret = m_waterfallPlot.getLayerDate(v - m_waterfallPlot.getOffset());
        if (ret > 0)
        {
            m_dateTime.setTime_t(ret);
            // need something else other than time_t to have 'zzz'
            //return m_dateTime.toString("hh:mm:ss:zzz");
            return m_dateTime.toString("dd.MM.yy\nhh:mm:ss");
        }
        return QwtText();
    }
};
}

Waterfallplot::Waterfallplot(QWidget* parent) :
    QWidget(parent),
    m_plotCurve(new QwtPlot),
    m_plotSpectrogram(new QwtPlot),
    m_curve(new QwtPlotCurve),
    m_spectrogram(new QwtPlotSpectrogram),
    m_curveXAxisData(nullptr),
    m_curveYAxisData(nullptr),
    m_zoomer(new MyZoomer(m_plotSpectrogram->canvas(), m_spectrogram, *this))
{
    m_plotCurve->setAutoReplot(false);
    m_plotSpectrogram->setAutoReplot(false);

//#if 0
    m_curve->attach(m_plotCurve);
    m_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    //m_curve->setTitle("");
    //m_curve->setPen(...); // pen : color + width
    m_curve->setStyle(QwtPlotCurve::Lines);
    //m_curve->setSymbol(new QwtSymbol(QwtSymbol::Style...,Qt::NoBrush, QPen..., QSize(5, 5)));
//#endif

    m_spectrogram->setRenderThreadCount(0); // use system specific thread count
    m_spectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
    m_spectrogram->attach(m_plotSpectrogram);

    // color map...
    //CoolToWarmColorMapRGB* const colorLut = new CoolToWarmColorMapRGB;
    LinearColorMapRGB* const colorLut = new LinearColorMapRGB;
    m_spectrogram->setColorMap(colorLut);

    // We need to enable yRight axis in order to align it with the spectrogram's one
    m_plotCurve->enableAxis(QwtPlot::yRight);
    m_plotCurve->axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtScaleDraw::Ticks, false);
    m_plotCurve->axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtScaleDraw::Labels, false);

    // Auto rescale
    m_plotCurve->setAxisAutoScale(QwtPlot::xBottom,       true);
    m_plotCurve->setAxisAutoScale(QwtPlot::yLeft,         true);
    m_plotCurve->setAxisAutoScale(QwtPlot::yRight,        true);
    m_plotSpectrogram->setAxisAutoScale(QwtPlot::xBottom, true);
    m_plotSpectrogram->setAxisAutoScale(QwtPlot::yLeft,   true);

    // the canvas should be perfectly aligned to the boundaries of your curve.
    m_plotSpectrogram->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotSpectrogram->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotSpectrogram->plotLayout()->setAlignCanvasToScales(true);

    m_plotCurve->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotCurve->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotCurve->axisScaleEngine(QwtPlot::yRight)->setAttribute(QwtScaleEngine::Floating, true);
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

    // Zoomer - brought to you from the experimentations with G1X Brillouin plot !
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    m_zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);
    const QColor c( Qt::darkBlue );
    //const QColor c(Qt::lightGray);
    m_zoomer->setRubberBandPen(c);
    m_zoomer->setTrackerPen(c);

    QObject::connect(m_zoomer, &QwtPlotZoomer::zoomed, this, &Waterfallplot::autoRescale);

    /*m_plotCurve->canvas()->setToolTip(
    "Zooming:\n"
    "- Mouse left button: zoom in an area by drawing a rectangle.\n"
    "- Mouse right button: previous zoomed in area.\n"
    "- Ctrl + mouse right button : zoom out to spectrogram full size.");*/

//#if 0
    connect(m_plotCurve->axisWidget(QwtPlot::xBottom), &QwtScaleWidget::scaleDivChanged,
            this,                          &Waterfallplot::scaleDivChanged/*, Qt::QueuedConnection*/);
    connect(m_plotSpectrogram->axisWidget(QwtPlot::xBottom), &QwtScaleWidget::scaleDivChanged,
            this,                                &Waterfallplot::scaleDivChanged/*,Qt::QueuedConnection*/);
//#endif
}

// From G1x Brillouin plot...
void Waterfallplot::autoRescale(const QRectF& rect)
{
    Q_UNUSED(rect)

    if (m_zoomer->zoomRectIndex() == 0)
    {
        // Rescale axis to data range
        m_plotSpectrogram->setAxisAutoScale(QwtPlot::xBottom, true);
        m_plotSpectrogram->setAxisAutoScale(QwtPlot::yLeft, true);

        m_zoomer->setZoomBase();
    }
}

void Waterfallplot::setDataDimensions(double dXMin, double dXMax,
                                      const size_t historyExtent,
                                      const size_t layerPoints)
{
    // TODO : free...

    // NB: m_data is just for convenience !
    m_data = new WaterfallData<double>(dXMin, dXMax, historyExtent, layerPoints);
    m_spectrogram->setData(m_data); // NB: owner of the data is m_spectrogram !

    // TODO....

    m_curveXAxisData = new double[layerPoints];
    m_curveYAxisData = new double[layerPoints];

    // generate curve X axis data
    const double dx = (dXMax - dXMin) / layerPoints; // x spacing
    m_curveXAxisData[0] = dXMin;
    for (size_t x = 1u; x < layerPoints; ++x)
    {
        m_curveXAxisData[x] = m_curveXAxisData[x - 1] + dx;
    }
}

void Waterfallplot::getDataDimensions(double& dXMin,
                                      double& dXMax,
                                      size_t& historyExtent,
                                      size_t& layerPoints) const
{
    if (m_data)
    {
        dXMin = m_data->getXMin();
        dXMax = m_data->getXMax();
        historyExtent = m_data->getMaxHistoryLength();
        layerPoints = m_data->getLayerPoints();
    }
    else
    {
        dXMin = 0;
        dXMax = 0;
        historyExtent = 0;
        layerPoints = 0;
    }
}

QwtPlot* Waterfallplot::getPlot() const
{
    return m_plotSpectrogram;
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

bool Waterfallplot::addData(const double* const dataPtr, const size_t dataLen, const time_t timestamp)
{
    if (!m_data)
    {
        return false;
    }

    const bool bRet = m_data->addData(dataPtr, dataLen, timestamp);
    if (bRet)
    {
        // refresh curve's data
        const double* wfData = m_data->getData();
        const size_t layerPts   = m_data->getLayerPoints();
        const size_t maxHistory = m_data->getMaxHistoryLength();

        if (m_curveXAxisData && m_curveYAxisData)
        {
            std::copy(wfData + layerPts * (maxHistory - 1),
                      wfData + layerPts * maxHistory,
                      m_curveYAxisData);
            m_curve->setRawSamples(m_curveXAxisData, m_curveYAxisData, layerPts);
        }

        // refresh spectrogram content and Y axis labels
        //m_spectrogram->invalidateCache();

        auto const yLeftAxis = static_cast<WaterfallTimeScaleDraw*>(
                    m_plotSpectrogram->axisScaleDraw(QwtPlot::yLeft));
        yLeftAxis->invalidateCache();

        const double currentOffset = getOffset();
        m_plotSpectrogram->setAxisScale(QwtPlot::yLeft, currentOffset, maxHistory + currentOffset);
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
                //colorMap = new CoolToWarmColorMapRGB;
                colorMap = new LinearColorMapRGB;
                m_bColorBarInitialized = true;
            }
            axis->setColorMap(QwtInterval(dLower, dUpper), colorMap);
        }
    }

    if (m_data)
    {
        m_data->setRange(dLower, dUpper);
    }

    m_spectrogram->invalidateCache();
}

void Waterfallplot::getRange(double& rangeMin, double& rangeMax) const
{
    if (m_data)
    {
        m_data->getRange(rangeMin, rangeMax);
    }
    else
    {
        rangeMin = 0;
        rangeMax = 1;
    }
}

void Waterfallplot::getDataRange(double& rangeMin, double& rangeMax) const
{
    if (m_data)
    {
        m_data->getDataRange(rangeMin, rangeMax);
    }
    else
    {
        rangeMin = 0;
        rangeMax = 1;
    }
}

void Waterfallplot::clear()
{
    if (m_data)
    {
        m_data->clear();
    }
}

time_t Waterfallplot::getLayerDate(const double y) const
{
    return m_data ? m_data->getLayerDate(y) : 0;
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

void Waterfallplot::setZLabel(const QString& qstrTitle)
{
    m_plotSpectrogram->setAxisTitle(QwtPlot::yRight, qstrTitle);
}

// color bar must be handled in another column (of a QGridLayout)
// to keep waterfall perfectly aligned with a curve plot !
/*void Waterfallplot::setZLabel(const QString& qstrTitle)
{
    m_plot->setAxisTitle(QwtPlot::yRight, qstrTitle);
}*/

void Waterfallplot::scaleDivChanged()
{
    // apparently, m_inScaleSync is a hack that can be replaced by
    // blocking signals on a widget but that could be cumbersome
    // or not possible as the Qwt API doesn't provide any mean to do that
    if (m_inScaleSync)
    {
        return;
    }
    m_inScaleSync = true;
    
    QwtPlot* updatedPlot;
    if (m_plotCurve->axisWidget(QwtPlot::xBottom) == sender())
    {
        updatedPlot = m_plotCurve;
    }
    else if (m_plotSpectrogram->axisWidget(QwtPlot::xBottom) == sender())
    {
        updatedPlot = m_plotSpectrogram;
    }
    else
    {
        updatedPlot = nullptr;
    }
    
    if (updatedPlot)
    {
        QwtPlot* plotToUpdate = (updatedPlot == m_plotCurve) ? m_plotSpectrogram : m_plotCurve;
        plotToUpdate->setAxisScaleDiv(QwtPlot::xBottom,
                                      updatedPlot->axisScaleDiv(QwtPlot::xBottom));
        updateLayout();
    }
    
    m_inScaleSync = false;
}

void Waterfallplot::alignAxis(int axisId)
{
    // 1. Align Vertical Axis (only left or right)
    double maxExtent = 0;

    {
        QwtScaleWidget* scaleWidget = m_plotCurve->axisWidget(axisId);

        QwtScaleDraw* sd = scaleWidget->scaleDraw();
        sd->setMinimumExtent(0.0);

        const double extent = sd->extent(scaleWidget->font());
        if (extent > maxExtent)
        {
            maxExtent = extent;
        }
    }

    {
        QwtScaleWidget* scaleWidget = m_plotSpectrogram->axisWidget(axisId);

        QwtScaleDraw* sd = scaleWidget->scaleDraw();
        sd->setMinimumExtent( 0.0 );

        const double extent = sd->extent(scaleWidget->font());
        if (extent > maxExtent)
        {
            maxExtent = extent;
        }
    }

    {
        QwtScaleWidget* scaleWidget = m_plotCurve->axisWidget(axisId);
        scaleWidget->scaleDraw()->setMinimumExtent(maxExtent);
    }

    {
        QwtScaleWidget* scaleWidget = m_plotSpectrogram->axisWidget(axisId);
        scaleWidget->scaleDraw()->setMinimumExtent(maxExtent);
    }
}

void Waterfallplot::alignAxisForColorBar()
{
    auto s1 = m_plotSpectrogram->axisWidget(QwtPlot::yRight);
    auto s2 = m_plotCurve->axisWidget(QwtPlot::yRight);

    s2->scaleDraw()->setMinimumExtent( 0.0 );

    qreal extent = s1->scaleDraw()->extent(s1->font());
    //extent -= s2->scaleDraw()->extent(s2->font());
    extent += s1->colorBarWidth() + s1->spacing();

    s2->scaleDraw()->setMinimumExtent(extent);
}

void Waterfallplot::updateLayout()
{
    // 1. Align Vertical Axis (only left or right)
    alignAxis(QwtPlot::yLeft);
    alignAxisForColorBar();
    
    // 2. Replot
    m_plotCurve->replot();
    m_plotSpectrogram->replot();
}

#include "Waterfallplot.h"

// Qt includes
#include <QApplication>
#include <QDateTime>
#include <QGridLayout>
#include <QSizePolicy>
#include <QVBoxLayout>

// Qwt includes
#include <qwt_color_map.h>
#include <qwt_picker_machine.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_panner.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_plot_zoomer.h>

// C++ STL and its standard lib includes
#include <algorithm>

namespace
{

QwtColorMap* controlPointsToQwtColorMap(const ColorMaps::ControlPoints& ctrlPts)
{
    using namespace ColorMaps;

    if (ctrlPts.size() < 2 ||
        std::get<0>(ctrlPts.front()) != 0. ||
        std::get<0>(ctrlPts.back())  != 1. ||
        !std::is_sorted(ctrlPts.cbegin(), ctrlPts.cend(),
        [](const ControlPoint& x, const ControlPoint& y)
        {
            // strict weak ordering
            return std::get<0>(x) < std::get<0>(y);
        }))
    {
        return nullptr;
    }

    QColor from, to;
    from.setRgbF(std::get<1>(ctrlPts.front()), std::get<2>(ctrlPts.front()), std::get<3>(ctrlPts.front()));
    to.setRgbF(std::get<1>(ctrlPts.back()), std::get<2>(ctrlPts.back()), std::get<3>(ctrlPts.back()));

    QwtLinearColorMap* lcm = new QwtLinearColorMap(from, to, QwtColorMap::RGB);

    for (size_t i = 1; i < ctrlPts.size() - 1; ++i)
    {
        QColor cs;
        cs.setRgbF(std::get<1>(ctrlPts[i]), std::get<2>(ctrlPts[i]), std::get<3>(ctrlPts[i]));
        lcm->addColorStop(std::get<0>(ctrlPts[i]), cs);
    }

    return lcm;
}

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
            time_t timeVal = m_waterfallPlot.getLayerDate(histVal - m_waterfallPlot.getOffset());
            if (timeVal > 0)
            {
                m_dateTime.setTime_t(timeVal);
                date = m_dateTime.toString("dd.MM.yy - hh:mm:ss");
            }

            const double tempVal = m_spectro->data()->value(pos.x(), pos.y());
            text = QString("%1%2, %3: %4%5")
                    .arg(distVal)
                    .arg(m_waterfallPlot.m_xUnit)
                    .arg(date)
                    .arg(tempVal)
                    .arg(m_waterfallPlot.m_zUnit);
        }
        else
        {
            text = QString("%1%2: -%3")
                    .arg(distVal)
                    .arg(m_waterfallPlot.m_xUnit)
                    .arg(m_waterfallPlot.m_zUnit);
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

Waterfallplot::Waterfallplot(QWidget* parent, const ColorMaps::ControlPoints& ctrlPts /*= ColorMaps::Jet()*/) :
    QWidget(parent),
    m_plotHorCurve(new QwtPlot),
    m_plotVertCurve(new QwtPlot),
    m_plotSpectrogram(new QwtPlot),
    m_picker(new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
        QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, m_plotSpectrogram->canvas())),
    m_panner(new QwtPlotPanner(m_plotSpectrogram->canvas())),
    m_spectrogram(new QwtPlotSpectrogram),
    m_zoomer(new MyZoomer(m_plotSpectrogram->canvas(), m_spectrogram, *this)),
    m_horCurveMarker(new QwtPlotMarker),
    m_vertCurveMarker(new QwtPlotMarker),
    m_ctrlPts(ctrlPts)
{
    //m_plotHorCurve->setFixedHeight(200);
    //m_plotVertCurve->setFixedWidth(400);

    m_plotHorCurve->setAutoReplot(false);
    m_plotVertCurve->setAutoReplot(false);
    m_plotSpectrogram->setAutoReplot(false);

    m_spectrogram->setRenderThreadCount(0); // use system specific thread count
    m_spectrogram->setCachePolicy(QwtPlotRasterItem::PaintCache);
    m_spectrogram->attach(m_plotSpectrogram);

    // Setup color map
    setColorMap(m_ctrlPts);

    // We need to enable yRight axis in order to align it with the spectrogram's one
    m_plotHorCurve->enableAxis(QwtPlot::yRight);
    m_plotHorCurve->axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtScaleDraw::Ticks, false);
    m_plotHorCurve->axisWidget(QwtPlot::yRight)->scaleDraw()->enableComponent(QwtScaleDraw::Labels, false);
    QPalette palette = m_plotHorCurve->axisWidget(QwtPlot::yRight)->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    m_plotHorCurve->axisWidget(QwtPlot::yRight)->setPalette(palette);

    // Auto rescale
    m_plotHorCurve->setAxisAutoScale(QwtPlot::xBottom,    true);
    m_plotHorCurve->setAxisAutoScale(QwtPlot::yLeft,      false);
    m_plotHorCurve->setAxisAutoScale(QwtPlot::yRight,     true);
    m_plotVertCurve->setAxisAutoScale(QwtPlot::xBottom,   false);
    m_plotVertCurve->setAxisAutoScale(QwtPlot::yLeft,     true);
    m_plotVertCurve->setAxisAutoScale(QwtPlot::yRight,    true);
    m_plotSpectrogram->setAxisAutoScale(QwtPlot::xBottom, true);
    m_plotSpectrogram->setAxisAutoScale(QwtPlot::yLeft,   true);

    // the canvas should be perfectly aligned to the boundaries of your curve.
    m_plotSpectrogram->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotSpectrogram->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotSpectrogram->plotLayout()->setAlignCanvasToScales(true);

    m_plotHorCurve->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotHorCurve->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotHorCurve->axisScaleEngine(QwtPlot::yRight)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotHorCurve->plotLayout()->setAlignCanvasToScales(true);

    m_plotVertCurve->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotVertCurve->axisScaleEngine(QwtPlot::yLeft)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotVertCurve->axisScaleEngine(QwtPlot::yRight)->setAttribute(QwtScaleEngine::Floating, true);
    m_plotVertCurve->plotLayout()->setAlignCanvasToScales(true);

    m_plotSpectrogram->setAutoFillBackground(true);
    m_plotSpectrogram->setPalette(Qt::white);
    m_plotSpectrogram->setCanvasBackground(Qt::white);

    m_plotHorCurve->setAutoFillBackground(true);
    m_plotHorCurve->setPalette(Qt::white);
    m_plotHorCurve->setCanvasBackground(Qt::white);

    m_plotVertCurve->setAutoFillBackground(true);
    m_plotVertCurve->setPalette(Qt::white);
    m_plotVertCurve->setCanvasBackground(Qt::white);

    QwtPlotCanvas* const spectroCanvas = dynamic_cast<QwtPlotCanvas*>(m_plotSpectrogram->canvas());
    spectroCanvas->setFrameStyle(QFrame::NoFrame);

    QwtPlotCanvas* const horCurveCanvas = dynamic_cast<QwtPlotCanvas*>(m_plotHorCurve->canvas());
    horCurveCanvas->setFrameStyle(QFrame::NoFrame);

    QwtPlotCanvas* const vertCurveCanvas = dynamic_cast<QwtPlotCanvas*>(m_plotVertCurve->canvas());
    vertCurveCanvas->setFrameStyle(QFrame::NoFrame);

    // Y axis labels should represent the insert time of a layer (fft)
    m_plotSpectrogram->setAxisScaleDraw(QwtPlot::yLeft, new WaterfallTimeScaleDraw(*this));
    //m_plot->setAxisLabelRotation(QwtPlot::yLeft, -50.0);
    //m_plot->setAxisLabelAlignment(QwtPlot::yLeft, Qt::AlignLeft | Qt::AlignBottom);

    // test color bar...
    m_plotSpectrogram->enableAxis(QwtPlot::yRight);
    QwtScaleWidget* axis = m_plotSpectrogram->axisWidget(QwtPlot::yRight);
    axis->setColorBarEnabled(true);
    axis->setColorBarWidth(20);

#if 1
    QGridLayout* const gridLayout = new QGridLayout(this);
    gridLayout->addWidget(m_plotHorCurve, 0, 1);
    gridLayout->addWidget(m_plotSpectrogram, 1, 1);
    gridLayout->addWidget(m_plotVertCurve, 1, 0);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(5);

    gridLayout->setRowStretch(0, 1);
    gridLayout->setRowStretch(1, 3);

    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 4);

#else
    QVBoxLayout* layout = new QVBoxLayout(this);
    //layout->setSpacing(6);
    //layout->setContentsMargins(11, 11, 11, 11);
    //m_plotCurve->setFixedHeight(200);
    layout->addWidget(m_plotHorCurve, 20);
    layout->addWidget(m_plotSpectrogram, 60);
    layout->addWidget(m_plotVertCurve, 20);
#endif

    QSizePolicy policy;
    policy.setVerticalPolicy(QSizePolicy::Ignored);
    policy.setHorizontalPolicy(QSizePolicy::Ignored);
    m_plotHorCurve->setSizePolicy(policy);
    m_plotVertCurve->setSizePolicy(policy);
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

    m_picker->setStateMachine( new QwtPickerDragPointMachine() );
    m_picker->setRubberBandPen( QColor( Qt::green ) );
    m_picker->setRubberBand( QwtPicker::CrossRubberBand );
    //m_picker->setTrackerPen( QColor( Qt::white ) );
    m_picker->setTrackerMode(QwtPicker::AlwaysOff);
    m_picker->setEnabled(false);
    connect(m_picker, static_cast<void(QwtPlotPicker::*)(const QPointF&)>(&QwtPlotPicker::selected),
            this, &Waterfallplot::selectedPoint);
    connect(m_picker, static_cast<void(QwtPlotPicker::*)(const QPointF&)>(&QwtPlotPicker::moved),
            this, &Waterfallplot::selectedPoint);

    m_panner->setMouseButton(Qt::MidButton);

    connect(m_plotHorCurve->axisWidget(QwtPlot::xBottom), &QwtScaleWidget::scaleDivChanged,
            this,                                         &Waterfallplot::scaleDivChanged, Qt::QueuedConnection);
    connect(m_plotSpectrogram->axisWidget(QwtPlot::xBottom), &QwtScaleWidget::scaleDivChanged,
            this,                                            &Waterfallplot::scaleDivChanged, Qt::QueuedConnection);
    connect(m_plotSpectrogram->axisWidget(QwtPlot::yLeft), &QwtScaleWidget::scaleDivChanged,
            this,                                          &Waterfallplot::scaleDivChanged, Qt::QueuedConnection);

    //m_plotHorCurve->setTitle("Some plot");
    /* you shouldn't put a title as when the window shrinks in size, m_plotVertCurve and m_plotSpectrogram Y axis
     * will misalign, currently, in Qwt there's no way to avoid titles to misalign axis */
    m_plotVertCurve->setTitle(" ");

    {
        QwtPlotGrid* horCurveGrid = new QwtPlotGrid;
        horCurveGrid->enableXMin( true );
        horCurveGrid->setMinorPen( QPen( Qt::gray, 0 , Qt::DotLine ) );
        horCurveGrid->setMajorPen( QPen( Qt::gray, 0 , Qt::DotLine ) );
        horCurveGrid->attach(m_plotHorCurve);

        QwtPlotGrid* vertCurveGrid = new QwtPlotGrid;
        vertCurveGrid->enableXMin( true );
        vertCurveGrid->setMinorPen( QPen( Qt::gray, 0 , Qt::DotLine ) );
        vertCurveGrid->setMajorPen( QPen( Qt::gray, 0 , Qt::DotLine ) );
        vertCurveGrid->attach(m_plotVertCurve);
    }

    {
        m_horCurveMarker->setLineStyle(QwtPlotMarker::VLine);
        m_horCurveMarker->setLinePen(Qt::red, 0, Qt::SolidLine);
        m_horCurveMarker->attach(m_plotHorCurve);

        m_vertCurveMarker->setLineStyle( QwtPlotMarker::HLine );
        m_vertCurveMarker->setLinePen(Qt::red, 0, Qt::SolidLine );
        m_vertCurveMarker->attach(m_plotVertCurve);
    }
}

Waterfallplot::~Waterfallplot()
{
    freeCurvesData();
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

        m_zoomActive = false;
    }
    else
    {
        m_zoomActive = true;
    }
}

void Waterfallplot::setDataDimensions(double dXMin, double dXMax,
                                      const size_t historyExtent,
                                      const size_t layerPoints)
{
    // NB: m_data is just for convenience !
    m_data = new WaterfallData<double>(dXMin, dXMax, historyExtent, layerPoints);
    m_spectrogram->setData(m_data); // NB: owner of the data is m_spectrogram !

    setupCurves();
    freeCurvesData();
    allocateCurvesData();

    // After changing data dimensions, we need to reset curves markers
    // to show the last received data on  the horizontal axis and the history
    // of the middle point
    m_markerX = (dXMax - dXMin) / 2;
    m_markerY = historyExtent - 1;

    m_horCurveMarker->setValue(m_markerX, 0.0);
    m_vertCurveMarker->setValue(0.0, m_markerY);

    // scale x
    m_plotHorCurve->setAxisScale(QwtPlot::xBottom, dXMin, dXMax);
    m_plotSpectrogram->setAxisScale(QwtPlot::xBottom, dXMin, dXMax);
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

void Waterfallplot::replot(bool forceRepaint /*= false*/)
{
    if (!m_plotSpectrogram->isVisible())
    {
        // temporary solution for older Qwt versions
        QApplication::postEvent(m_plotHorCurve, new QEvent(QEvent::LayoutRequest));
        QApplication::postEvent(m_plotVertCurve, new QEvent(QEvent::LayoutRequest));
        QApplication::postEvent(m_plotSpectrogram, new QEvent(QEvent::LayoutRequest));
    }
    
    updateLayout();

    /*m_plotHorCurve->replot();
    m_plotVertCurve->replot();
    m_plotSpectrogram->replot();*/
    
    if (forceRepaint)
    {
        m_plotHorCurve->repaint();
        m_plotVertCurve->repaint();
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
        updateCurvesData();

        // refresh spectrogram content and Y axis labels
        //m_spectrogram->invalidateCache();

        auto const ySpectroLeftAxis = static_cast<WaterfallTimeScaleDraw*>(
                    m_plotSpectrogram->axisScaleDraw(QwtPlot::yLeft));
        ySpectroLeftAxis->invalidateCache();

        auto const yHistoLeftAxis = static_cast<WaterfallTimeScaleDraw*>(
                    m_plotVertCurve->axisScaleDraw(QwtPlot::yLeft));
        yHistoLeftAxis->invalidateCache();

        const double currentOffset = getOffset();
        const size_t maxHistory = m_data->getMaxHistoryLength();

        const QwtScaleDiv& yDiv = m_plotSpectrogram->axisScaleDiv(QwtPlot::yLeft);
        const double yMin = (m_zoomActive) ? yDiv.lowerBound() + 1 : currentOffset;
        const double yMax = (m_zoomActive) ? yDiv.upperBound() + 1 : maxHistory + currentOffset;

        m_plotSpectrogram->setAxisScale(QwtPlot::yLeft, yMin, yMax);
        m_plotVertCurve->setAxisScale(QwtPlot::yLeft, yMin, yMax);

        m_vertCurveMarker->setValue(0.0, m_markerY + currentOffset);
    }
    return bRet;
}

void Waterfallplot::setRange(double dLower, double dUpper)
{
    if (dLower > dUpper)
    {
        std::swap(dLower, dUpper);
    }

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
                colorMap = controlPointsToQwtColorMap(m_ctrlPts);
                m_bColorBarInitialized = true;
            }
            axis->setColorMap(QwtInterval(dLower, dUpper), colorMap);
        }
    }

    // set vertical plot's X axis and horizontal plot's Y axis scales to the color bar min/max
    m_plotHorCurve->setAxisScale(QwtPlot::yLeft, dLower, dUpper);
    m_plotVertCurve->setAxisScale(QwtPlot::xBottom, dLower, dUpper);

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

    setupCurves();
    freeCurvesData();
    allocateCurvesData();
}

time_t Waterfallplot::getLayerDate(const double y) const
{
    return m_data ? m_data->getLayerDate(y) : 0;
}

void Waterfallplot::setTitle(const QString& qstrNewTitle)
{
    m_plotSpectrogram->setTitle(qstrNewTitle);
}

void Waterfallplot::setXLabel(const QString& qstrTitle, const int fontPointSize /*= 12*/)
{
    QFont font;
    font.setPointSize(fontPointSize);

    QwtText title;
    title.setText(qstrTitle);
    title.setFont(font);

    m_plotSpectrogram->setAxisTitle(QwtPlot::xBottom, title);
}

void Waterfallplot::setYLabel(const QString& qstrTitle, const int fontPointSize /*= 12*/)
{
    QFont font;
    font.setPointSize(fontPointSize);

    QwtText title;
    title.setText(qstrTitle);
    title.setFont(font);

    m_plotSpectrogram->setAxisTitle(QwtPlot::yLeft, title);
}

void Waterfallplot::setZLabel(const QString& qstrTitle, const int fontPointSize /*= 12*/)
{
    QFont font;
    font.setPointSize(fontPointSize);

    QwtText title;
    title.setText(qstrTitle);
    title.setFont(font);

    m_plotSpectrogram->setAxisTitle(QwtPlot::yRight, title);
    m_plotHorCurve->setAxisTitle(QwtPlot::yLeft, title);
    m_plotHorCurve->setAxisTitle(QwtPlot::yRight, title);
    m_plotVertCurve->setAxisTitle(QwtPlot::xBottom, title);
}

void Waterfallplot::setXTooltipUnit(const QString& xUnit)
{
    m_xUnit = xUnit;
}

void Waterfallplot::setZTooltipUnit(const QString& zUnit)
{
    m_zUnit = zUnit;
}

bool Waterfallplot::setColorMap(const ColorMaps::ControlPoints& colorMap)
{
    QwtColorMap* spectrogramColorMap = controlPointsToQwtColorMap(colorMap);
    if (!spectrogramColorMap)
    {
        return false;
    }
    m_ctrlPts = colorMap;
    m_spectrogram->setColorMap(spectrogramColorMap);

    if (m_plotSpectrogram->axisEnabled(QwtPlot::yRight))
    {
        QwtScaleWidget* axis = m_plotSpectrogram->axisWidget(QwtPlot::yRight);
        if (axis->isColorBarEnabled())
        {
            double dLower;
            double dUpper;
            getRange(dLower, dUpper);

            axis->setColorMap(QwtInterval(dLower, dUpper), controlPointsToQwtColorMap(m_ctrlPts));
        }
    }

    m_spectrogram->invalidateCache();

    return true;
}

ColorMaps::ControlPoints Waterfallplot::getColorMap() const
{
    return m_ctrlPts;
}

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
    int axisId;
    if (m_plotHorCurve->axisWidget(QwtPlot::xBottom) == sender())
    {
        updatedPlot = m_plotHorCurve;
        axisId = QwtPlot::xBottom;
    }
    else if (m_plotSpectrogram->axisWidget(QwtPlot::xBottom) == sender())
    {
        updatedPlot = m_plotSpectrogram;
        axisId = QwtPlot::xBottom;
    }
    else if (m_plotSpectrogram->axisWidget(QwtPlot::yLeft) == sender())
    {
        updatedPlot = m_plotSpectrogram;
        axisId = QwtPlot::yLeft;
    }
    else
    {
        updatedPlot = nullptr;
    }
    
    if (updatedPlot)
    {
        QwtPlot* plotToUpdate;
        if (axisId == QwtPlot::xBottom)
        {
            plotToUpdate = (updatedPlot == m_plotHorCurve) ? m_plotSpectrogram : m_plotHorCurve;
        }
        else
        {
            plotToUpdate = m_plotVertCurve;
        }

        plotToUpdate->setAxisScaleDiv(axisId, updatedPlot->axisScaleDiv(axisId));
        updateLayout();
    }
    
    m_inScaleSync = false;
}

void Waterfallplot::alignAxis(int axisId)
{
    // 1. Align Vertical Axis (only left or right)
    double maxExtent = 0;

    {
        QwtScaleWidget* scaleWidget = m_plotHorCurve->axisWidget(axisId);

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
        QwtScaleWidget* scaleWidget = m_plotHorCurve->axisWidget(axisId);
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
    auto s2 = m_plotHorCurve->axisWidget(QwtPlot::yRight);

    s2->scaleDraw()->setMinimumExtent( 0.0 );

    qreal extent = s1->scaleDraw()->extent(s1->font());
    extent -= s2->scaleDraw()->extent(s2->font());
    extent += s1->colorBarWidth() + s1->spacing();

    s2->scaleDraw()->setMinimumExtent(extent);
}

void Waterfallplot::updateLayout()
{
    // 1. Align Vertical Axis (only left or right)
    alignAxis(QwtPlot::yLeft);
    alignAxisForColorBar();
    
    // 2. Replot
    m_plotHorCurve->replot();
    m_plotVertCurve->replot();
    m_plotSpectrogram->replot();
}

void Waterfallplot::updateCurvesData()
{
    // refresh curve's data
    const size_t currentHistory = m_data->getHistoryLength();
    const size_t layerPts   = m_data->getLayerPoints();
    const size_t maxHistory = m_data->getMaxHistoryLength();
    const double* wfData = m_data->getData();

    const size_t markerY = m_markerY;
    if (markerY >= maxHistory)
    {
        return;
    }

    if (m_horCurveXAxisData && m_horCurveYAxisData)
    {
        std::copy(wfData + layerPts * markerY,
                  wfData + layerPts * (markerY + 1),
                  m_horCurveYAxisData);
        m_horCurve->setRawSamples(m_horCurveXAxisData, m_horCurveYAxisData, layerPts);
    }

    const double offset = m_data->getOffset();

    if (currentHistory > 0 && m_vertCurveXAxisData && m_vertCurveYAxisData)
    {
        size_t dataIndex = 0;
        for (size_t layer = maxHistory - currentHistory; layer < maxHistory; ++layer, ++dataIndex)
        {
            const double z = m_data->value(m_markerX, layer + size_t(offset));
            const double t = double(layer) + offset;
            m_vertCurveXAxisData[dataIndex] = z;
            m_vertCurveYAxisData[dataIndex] = t;
        }

        m_vertCurve->setRawSamples(m_vertCurveXAxisData, m_vertCurveYAxisData, currentHistory);

        //auto resultPair = std::minmax_element(m_vertCurveXAxisData, m_vertCurveXAxisData + currentHistory);
        //const double rangeMin = *resultPair.first;
        //const double rangeMax = *resultPair.second;

        //m_plotVertCurve->setAxisScale(QwtPlot::xBottom, rangeMin, rangeMax);
    }
}

void Waterfallplot::allocateCurvesData()
{
    if (m_horCurveXAxisData || m_horCurveYAxisData || m_vertCurveXAxisData ||
        m_vertCurveYAxisData || !m_data)
    {
        return;
    }

    const size_t layerPoints = m_data->getLayerPoints();
    const double dXMin = m_data->getXMin();
    const double dXMax = m_data->getXMax();
    const size_t historyExtent = m_data->getMaxHistoryLength();

    m_horCurveXAxisData  = new double[layerPoints];
    m_horCurveYAxisData  = new double[layerPoints];
    m_vertCurveXAxisData = new double[historyExtent];
    m_vertCurveYAxisData = new double[historyExtent];

    // generate curve X axis data
    const double dx = (dXMax - dXMin) / layerPoints; // x spacing
    m_horCurveXAxisData[0] = dXMin;
    for (size_t x = 1u; x < layerPoints; ++x)
    {
        m_horCurveXAxisData[x] = m_horCurveXAxisData[x - 1] + dx;
    }

    // Reset marker to the default position
    m_markerX = (dXMax - dXMin) / 2;
    m_markerY = historyExtent - 1;
}

void Waterfallplot::freeCurvesData()
{
    if (m_horCurveXAxisData)
    {
        delete [] m_horCurveXAxisData;
        m_horCurveXAxisData = nullptr;
    }
    if (m_horCurveYAxisData)
    {
        delete [] m_horCurveYAxisData;
        m_horCurveYAxisData = nullptr;
    }
    if (m_vertCurveXAxisData)
    {
        delete [] m_vertCurveXAxisData;
        m_vertCurveXAxisData = nullptr;
    }
    if (m_vertCurveYAxisData)
    {
        delete [] m_vertCurveYAxisData;
        m_vertCurveYAxisData = nullptr;
    }
}

void Waterfallplot::setPickerEnabled(const bool enabled)
{
    m_panner->setEnabled(!enabled);

    m_picker->setEnabled(enabled);

    m_zoomer->setEnabled(!enabled);
    //m_zoomer->zoom(0);

    // clear plots ?
}

bool Waterfallplot::setMarker(const double x, const double y)
{
    if (!m_data)
    {
        return false;
    }

    const QwtInterval xInterval = m_data->interval(Qt::XAxis);
    const QwtInterval yInterval = m_data->interval(Qt::YAxis);
    if (!(xInterval.contains(x) && yInterval.contains(y)))
    {
        return false;
    }

    m_markerX = x;

    const double offset = m_data->getOffset();
    m_markerY = y - offset;

    // update curves's markers positions
    m_horCurveMarker->setValue(m_markerX, 0.0);
    m_vertCurveMarker->setValue(0.0, y);

    updateCurvesData();

    m_plotHorCurve->replot();
    m_plotVertCurve->replot();

    return true;
}

void Waterfallplot::selectedPoint(const QPointF& pt)
{
    setMarker(pt.x(), pt.y());
}

void Waterfallplot::setupCurves()
{
    m_plotHorCurve->detachItems(QwtPlotItem::Rtti_PlotCurve, true);
    m_plotVertCurve->detachItems(QwtPlotItem::Rtti_PlotCurve, true);

    m_horCurve = new QwtPlotCurve;
    m_vertCurve = new QwtPlotCurve;

    /* Horizontal Curve */
    m_horCurve->attach(m_plotHorCurve);
    m_horCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    //m_curve->setTitle("");
    //m_curve->setPen(...); // pen : color + width
    m_horCurve->setStyle(QwtPlotCurve::Lines);
    //m_curve->setSymbol(new QwtSymbol(QwtSymbol::Style...,Qt::NoBrush, QPen..., QSize(5, 5)));

    /* Vertical Curve */
    m_vertCurve->attach(m_plotVertCurve);
    m_vertCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    m_vertCurve->setStyle(QwtPlotCurve::Lines);

    m_plotVertCurve->setAxisScaleDraw(QwtPlot::yLeft, new WaterfallTimeScaleDraw(*this));
}

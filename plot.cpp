#include "plot.h"
#include "Waterfallplot.h"

#include <qprinter.h>
#include <qprintdialog.h>

#include <qwt_plot_renderer.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>

#include <QThread>

Plot::Plot( QWidget *parent ):
    QwtPlot( parent )
{
    m_waterfall = new Waterfallplot(0, 500, 64, 126, this);

    srand(time(nullptr));
}

#ifndef QT_NO_PRINTER
void Plot::printPlot()
{
    QPrinter printer( QPrinter::HighResolution );
    printer.setOrientation( QPrinter::Landscape );
    printer.setOutputFileName( "waterfall.pdf" );

    QPrintDialog dialog( &printer );
    if ( dialog.exec() )
    {
        QwtPlotRenderer renderer;

        if ( printer.colorMode() == QPrinter::GrayScale )
        {
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardBackground );
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasBackground );
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasFrame );
            renderer.setLayoutFlag( QwtPlotRenderer::FrameWithScales );
        }

        renderer.renderTo( this, printer );
    }
}
#endif

void Plot::playData()
{
    for (auto i = 0u; i < 32; ++i)
    {
        std::vector<float> dummyData(126);
        std::generate(dummyData.begin(), dummyData.end(), []{ return (std::rand() % 256); });

        const bool bRet = m_waterfall->addData(dummyData.data(), dummyData.size());
        assert(bRet);

        // set the range only once (data range)
        static bool s_setRangeOnlyOnce = true;
        if (s_setRangeOnlyOnce)
        {
            double dataRng[2];
            m_waterfall->getDataRange(dataRng[0], dataRng[1]);
            m_waterfall->setRange(dataRng[0], dataRng[1]);
            s_setRangeOnlyOnce = false;
        }

        replot();
        repaint(); // force repaint

        QThread::msleep(10);
    }
}

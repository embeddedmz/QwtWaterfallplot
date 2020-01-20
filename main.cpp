#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <vector>

#include <qapplication.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qprinter.h>
#include <qprintdialog.h>
#include <QThread>
#include <QVBoxLayout>

#include <qwt_color_map.h>
#include <qwt_plot_renderer.h>

#include "Waterfallplot.h"

class MainWindow: public QMainWindow
{
public:
    MainWindow(QWidget * = NULL);

public slots:
#ifndef QT_NO_PRINTER
    void printPlot();
#endif
    void playData();

private:
    Waterfallplot* m_waterfall = nullptr;
};

MainWindow::MainWindow( QWidget *parent ):
    QMainWindow( parent )
{
    srand(time(nullptr));

    QWidget *centralWidget = new QWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->setSpacing(6);
    layout->setContentsMargins(11, 11, 11, 11);

    m_waterfall = new Waterfallplot(nullptr);

    layout->addWidget(m_waterfall);

    setCentralWidget(centralWidget);

    // Toolbar
    QToolBar *toolBar = new QToolBar( this );
#ifndef QT_NO_PRINTER
    QToolButton *btnPrint = new QToolButton( toolBar );
    btnPrint->setText( "Print" );
    btnPrint->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->addWidget( btnPrint );
    connect( btnPrint, SIGNAL( clicked() ), this, SLOT( printPlot() ) );

    toolBar->addSeparator();
#endif

    QToolButton* btnPlayData = new QToolButton(toolBar);
    btnPlayData->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    btnPlayData->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnPlayData->setToolTip("Play 32 random data (layers) on the waterfall view.");
    toolBar->addWidget(btnPlayData);
    QObject::connect(btnPlayData, &QToolButton::clicked, this, &MainWindow::playData);

    addToolBar(toolBar);
}

int main( int argc, char **argv )
{
    QApplication a( argc, argv );
    a.setStyle( "Windows" );

    MainWindow mainWindow;
    mainWindow.resize( 600, 400 );
    mainWindow.show();

    return a.exec();
}

#ifndef QT_NO_PRINTER
void MainWindow::printPlot()
{
    QPrinter printer( QPrinter::HighResolution );
    printer.setOrientation( QPrinter::Landscape );
    printer.setOutputFileName( "waterfall.pdf" );

    QPrintDialog dialog( &printer );
    if (dialog.exec())
    {
        QwtPlotRenderer renderer;

        if ( printer.colorMode() == QPrinter::GrayScale )
        {
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardBackground );
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasBackground );
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasFrame );
            renderer.setLayoutFlag( QwtPlotRenderer::FrameWithScales );
        }
        // how to render curve + spectro in the same file ?
        renderer.renderTo( m_waterfall->getSpectrogramPlot(), printer );
    }
}
#endif

void MainWindow::playData()
{
    for (auto i = 0u; i < 32; ++i)
    {
        std::vector<double> dummyData(126);
        std::generate(dummyData.begin(), dummyData.end(), []{ return (std::rand() % 256); });

        double xMin, xMax;
        size_t historyLength, layerPoints;
        m_waterfall->getDataDimensions(xMin, xMax, historyLength, layerPoints);

        if (xMin != 0 || xMax != 500 ||
            126 != layerPoints ||
            64 != historyLength)
        {
            // log/notify for debug purposes...
            m_waterfall->setDataDimensions(0, 500, 64, dummyData.size());
        }

        const bool bRet = m_waterfall->addData(dummyData.data(), dummyData.size(), std::time(nullptr));
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

        m_waterfall->replot(true); // true: force repaint

        QThread::msleep(10);
    }
}

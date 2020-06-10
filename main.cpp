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
#include <QThread>
#include <QVBoxLayout>

#include <qwt_color_map.h>
#include <qwt_plot_renderer.h>

#include "ExportDialog.h"
#include "Waterfallplot.h"

class MainWindow: public QMainWindow
{
public:
    MainWindow(QWidget * = NULL);

public slots:
    void changeColorMap();
    void exportPlots();
    void playData();
    void clearWaterfall();

private:
    Waterfallplot* m_waterfall = nullptr;
};

MainWindow::MainWindow( QWidget *parent ) :
    QMainWindow( parent )
{
    srand(time(nullptr));

    QWidget *centralWidget = new QWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->setSpacing(6);
    layout->setContentsMargins(11, 11, 11, 11);

    m_waterfall = new Waterfallplot(nullptr);

    m_waterfall->setTitle("Waterfall Demo");
    m_waterfall->setXLabel("Distance (m)", 10);
    m_waterfall->setXTooltipUnit("m");
    m_waterfall->setZTooltipUnit("°C");
    m_waterfall->setYLabel("Time", 10);
    m_waterfall->setZLabel("Temperature (°C)", 10);

    layout->addWidget(m_waterfall);

    setCentralWidget(centralWidget);

    // Toolbar
    QToolBar *toolBar = new QToolBar( this );
    QToolButton *btnExport = new QToolButton( toolBar );
    btnExport->setText( "Export" );
    btnExport->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->addWidget( btnExport );
    connect( btnExport, &QToolButton::clicked, this, &MainWindow::exportPlots );

    QToolButton *btnChangeColorMap = new QToolButton( toolBar );
    btnChangeColorMap->setText( "Change color map" );
    btnChangeColorMap->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->addWidget( btnChangeColorMap );
    connect( btnChangeColorMap, &QToolButton::clicked, this, &MainWindow::changeColorMap );

    toolBar->addSeparator();

    QToolButton* btnPlayData = new QToolButton(toolBar);
    btnPlayData->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    btnPlayData->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnPlayData->setToolTip("Play 32 random data (layers) on the waterfall view.");
    toolBar->addWidget(btnPlayData);
    QObject::connect(btnPlayData, &QToolButton::clicked, this, &MainWindow::playData);

    QToolButton* btnPicker = new QToolButton(toolBar);
    btnPicker->setText( "Pick" );
    //btnPicker->setIcon( QPixmap( zoom_xpm ) );
    btnPicker->setCheckable( true );
    btnPicker->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->addWidget(btnPicker);
    QObject::connect(btnPicker, &QToolButton::toggled, m_waterfall, &Waterfallplot::setPickerEnabled);

    QToolButton* btnClear = new QToolButton(toolBar);
    btnClear->setText("Clear");
    btnClear->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->addWidget(btnClear);
    QObject::connect(btnClear, &QToolButton::clicked, this, &MainWindow::clearWaterfall);

    addToolBar(toolBar);
}

int main( int argc, char **argv )
{
    QApplication a( argc, argv );
    a.setStyle( "Windows" );

    MainWindow mainWindow;
    mainWindow.resize( 1152, 768 );
    mainWindow.show();

    return a.exec();
}

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

void MainWindow::clearWaterfall()
{
    m_waterfall->clear();
    m_waterfall->replot(true); // true: force repaint
}

void MainWindow::exportPlots()
{
    ExportDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        QwtPlotRenderer renderer;

        if (dialog.getExportWaterfallCurve())
        {
            renderer.exportTo(m_waterfall->getSpectrogramPlot(), "waterfall.pdf");
        }

        if (dialog.getExportHorizontalCurve())
        {
            renderer.exportTo(m_waterfall->getHorizontalCurvePlot(), "horizontal_plot.pdf");
        }

        if (dialog.getExportVerticalCurve())
        {
            renderer.exportTo(m_waterfall->getVerticalCurvePlot(), "vertical_plot.pdf");
        }
    }
}

void MainWindow::changeColorMap()
{
    static bool s_useBBRColorMap = true;
    if (s_useBBRColorMap)
    {

        m_waterfall->setColorMap(ColorMaps::BlackBodyRadiation());
        m_waterfall->replot();

        s_useBBRColorMap = false;
    }
    else
    {
        m_waterfall->setColorMap(ColorMaps::Jet());
        m_waterfall->replot();

        s_useBBRColorMap = true;
    }
}

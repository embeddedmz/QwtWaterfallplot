#include <qwt_plot.h>

//#include <QTimer>

class Waterfallplot;

class Plot: public QwtPlot
{
    Q_OBJECT

public:
    Plot( QWidget * = NULL );

public slots:
#ifndef QT_NO_PRINTER
    void printPlot();
#endif
    void playData();

private:
    Waterfallplot* m_waterfall = nullptr;

    //QTimer m_renderTimer;
};

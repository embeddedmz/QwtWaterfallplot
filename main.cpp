#include <qapplication.h>
#include <qmainwindow.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include "plot.h"
#include "qwt_color_map.h"

class MainWindow: public QMainWindow
{
public:
    MainWindow( QWidget * = NULL );

private:
    Plot *d_plot;
};

MainWindow::MainWindow( QWidget *parent ):
    QMainWindow( parent )
{
    d_plot = new Plot( this );

    setCentralWidget( d_plot );

    QToolBar *toolBar = new QToolBar( this );

#ifndef QT_NO_PRINTER
    QToolButton *btnPrint = new QToolButton( toolBar );
    btnPrint->setText( "Print" );
    btnPrint->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
    toolBar->addWidget( btnPrint );
    connect( btnPrint, SIGNAL( clicked() ), d_plot, SLOT( printPlot() ) );

    toolBar->addSeparator();
#endif

    QToolButton* btnPlayData = new QToolButton(toolBar);
    btnPlayData->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    btnPlayData->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btnPlayData->setToolTip("Play 32 random data (layers) on the waterfall view.");
    toolBar->addWidget(btnPlayData);
    QObject::connect(btnPlayData, &QToolButton::clicked, d_plot, &Plot::playData);

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

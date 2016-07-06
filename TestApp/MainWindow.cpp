#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "MapGraphicsView.h"
#include "MapGraphicsScene.h"
#include "tileSources/GridTileSource.h"
#include "tileSources/OSMTileSource.h"
#include "tileSources/FileSystemTileSource.h"
#include "tileSources/CompositeTileSource.h"
#include "tileSources/GoogleTileSource.h"
#include "guts/CompositeTileSourceConfigurationWidget.h"
#include "CircleObject.h"
#include "PolygonObject.h"
#include "WeatherManager.h"

#include <QSharedPointer>
#include <QtDebug>
#include <QThread>
#include <QComboBox>
#include <QSpinBox>
#include <QImage>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_current_roration(0)
{
    ui->setupUi(this);

    m_spin_zoomlevel = new QSpinBox;
    m_spin_zoomlevel->setRange(0, 18);
    m_spin_rotation = new QSpinBox;
    m_spin_rotation->setRange(-360, 360);

    ui->toolBar->addWidget(new QLabel(tr("ZoomLevel:")));
    ui->toolBar->addWidget(m_spin_zoomlevel);
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(new QLabel(tr("Rotation:")));
    ui->toolBar->addWidget(m_spin_rotation);

    //Setup the MapGraphics scene and view
    MapGraphicsScene * scene = new MapGraphicsScene(this);
    MapGraphicsView * view = new MapGraphicsView(scene,this);

    m_view = view;

    //The view will be our central widget
    this->setCentralWidget(view);

    //Setup some tile sources
    QSharedPointer<FileSystemTileSource> fsTiles(new FileSystemTileSource("C:/Users/examyes/Desktop/map/"), &QObject::deleteLater);
    QSharedPointer<CompositeTileSource> composite(new CompositeTileSource(), &QObject::deleteLater);

    composite->addSourceTop(fsTiles);
    view->setTileSource(composite);

    //Create a widget in the dock that lets us configure tile source layers
    CompositeTileSourceConfigurationWidget * tileConfigWidget = new CompositeTileSourceConfigurationWidget(composite.toWeakRef(),
                                                                                         this->ui->dockWidget);
    this->ui->dockWidget->setWidget(tileConfigWidget);
    delete this->ui->dockWidgetContents;

    this->ui->menuWindow->addAction(this->ui->dockWidget->toggleViewAction());
    this->ui->dockWidget->toggleViewAction()->setText("&Layers");

    view->setZoomLevel(13);
    view->centerOn(120.320667362213, 31.428553009033);
    view->enableMousePositionReport(true);

    m_spin_zoomlevel->setValue(view->zoomLevel());

    connect(view, SIGNAL(mousePositionChanged(QPointF,QPointF)),
            this, SLOT(onMousePositionChanged(QPointF,QPointF)));

    connect(view, SIGNAL(zoomLevelChanged(quint8)),
            this, SLOT(onZoomLevelChanged(quint8)));

    connect(m_spin_zoomlevel, SIGNAL(valueChanged(int)),
            this, SLOT(setZoomLevel(int)));

    connect(m_spin_rotation, SIGNAL(valueChanged(int)),
            this, SLOT(setRotation(int)));

    // WeatherManager * weatherMan = new WeatherManager(scene, this);
    // Q_UNUSED(weatherMan)
}

MainWindow::~MainWindow()
{
    delete ui;
}

//private slot
void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::onMousePositionChanged(QPointF viewPos, QPointF ll)
{
    QString message = QString("View Position: (%1, %2), Geo Position: (%3, %4)")
            .arg(viewPos.x())
            .arg(viewPos.y())
            .arg(ll.x(), 0, 'f', 14)
            .arg(ll.y(), 0, 'f', 14);
    ui->statusBar->showMessage(message);
}

void MainWindow::setZoomLevel(int i)
{
    m_view->setZoomLevel(i);
}

void MainWindow::onZoomLevelChanged(quint8 nZoom)
{
    m_spin_zoomlevel->setValue(nZoom);
}

void MainWindow::setRotation(int i)
{
    m_view->rotate(-m_current_roration);
    m_view->rotate(i);

    m_current_roration = i;
}

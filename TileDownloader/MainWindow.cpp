#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cmath>

#include <QApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QDir>
#include <QFileDialog>
#include <QThread>
#include <QDesktopServices>

#include "guts/MapGraphicsNetwork.h"

#include "TileDownloader.h"

#include <QDebug>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_downloader(new TileDownloader)
{
    ui->setupUi(this);

    ui->spin_zoomlevel->setRange(0, 18);
    ui->spin_zoomlevel->setValue(3);
    ui->line_path_to_save->setReadOnly(true);

    ui->line_left_longitude->setText("-180");
    ui->line_top_latitude->setText("85.05113");
    ui->line_right_longitude->setText("180");
    ui->line_bottom_latitude->setText("-85.05113");

    ui->line_path_to_save->setText(qApp->applicationDirPath());

    m_downloader_thread = new QThread(this);
    m_downloader->moveToThread(m_downloader_thread);
    m_downloader_thread->start();

    connect(ui->btn_browse, SIGNAL(clicked(bool)),
            this, SLOT(onBrowseFolder()));

    connect(this, SIGNAL(startDownload(QPointF,QPointF,int,QString,QString)),
            m_downloader, SLOT(download(QPointF,QPointF,int,QString,QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_Exit_triggered()
{
    qApp->quit();
}

QPointF MainWindow::getGeoStart(bool *ok)
{
    Q_ASSERT(0 != ok);
    bool convert_ok = false;

    qreal lon = ui->line_left_longitude->text().toDouble(&convert_ok);
    if (!convert_ok)
    {
        *ok = false;
        return QPointF();
    }

    qreal lat = ui->line_top_latitude->text().toDouble(&convert_ok);
    if (!convert_ok)
    {
        *ok = false;
        return QPointF();
    }

    *ok = true;
    return QPointF(lon, lat);
}

QPointF MainWindow::getGeoStop(bool *ok)
{
    Q_ASSERT(0 != ok);
    bool convert_ok = false;

    qreal lon = ui->line_right_longitude->text().toDouble(&convert_ok);
    if (!convert_ok)
    {
        *ok = false;
        return QPointF();
    }

    qreal lat = ui->line_bottom_latitude->text().toDouble(&convert_ok);
    if (!convert_ok)
    {
        *ok = false;
        return QPointF();
    }

    *ok = true;
    return QPointF(lon, lat);
}

void MainWindow::dbg(const QString &message)
{
    ui->text_browser_log->append(message);
}


void MainWindow::on_pushButton_clicked()
{    
    int z = ui->spin_zoomlevel->value();

    bool ok = false;
    QPointF geo_start = getGeoStart(&ok);
    if (!ok) return;

    QPointF geo_stop = getGeoStop(&ok);
    if (!ok) return;

    QString url_pattern = "http://mt1.google.cn/vt/lyrs=y&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3";

    emit startDownload(geo_start, geo_stop, z, url_pattern,
                           ui->line_path_to_save->text());
}

void MainWindow::onBrowseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this);

    if (!dir.isNull())
        ui->line_path_to_save->setText(dir);
}

void MainWindow::on_btn_open_folder_clicked()
{
    QString target_path = QString("file:///%1").arg(ui->line_path_to_save->text());
    QDesktopServices::openUrl(QUrl(target_path, QUrl::TolerantMode));
}

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
#include <QSettings>

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
    , m_log_buffer_size(0)
{
    ui->setupUi(this);

    ui->spin_zoomlevel->setRange(0, 18);
    ui->spin_zoomlevel->setValue(3);
    ui->line_path_to_save->setReadOnly(true);

    ui->line_left_longitude->setText("-180");
    ui->line_top_latitude->setText("85.05113");
    ui->line_right_longitude->setText("180");
    ui->line_bottom_latitude->setText("-85.05113");

    ui->check_skip->setChecked(true);
    ui->spin_log_buffer_size->setRange(10, 1000000);
    ui->spin_log_buffer_size->setValue(1000);

    addMapType("Google Map Tiles", "http://mt2.google.cn/vt/lyrs=m&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");
    addMapType("Google Skeleton Map Light Tiles", "http://mt2.google.cn/vt/lyrs=h&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");
    addMapType("Google Skeleton Map Dark Tiles", "http://mt0.google.cn/vt/lyrs=r&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");
    addMapType("Google Terrain Tiles", "http://mt2.google.cn/vt/lyrs=t&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");
    addMapType("Google Terrain Map Tiles", "http://mt2.google.cn/vt/lyrs=p&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");
    addMapType("Google Satellite Tiles", "http://mt3.google.cn/vt/lyrs=s&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");
    addMapType("Google Hybrid Satellite Map Tiles", "http://mt1.google.cn/vt/lyrs=y&hl=zh-CN&gl=cn&x=%1&y=%2&z=%3");

    ui->combo_map_type->setCurrentIndex(6);

    QSettings settings;
    QString saved_path = settings.value("MAP_SAVE_PATH").toString();
    if (saved_path.isNull())
        ui->line_path_to_save->setText(qApp->applicationDirPath());
    else
        ui->line_path_to_save->setText(saved_path);


    m_downloader_thread = new QThread(this);
    m_downloader->moveToThread(m_downloader_thread);
    m_downloader_thread->start();

    connect(ui->btn_browse, SIGNAL(clicked(bool)),
            this, SLOT(onBrowseFolder()));

    connect(this, SIGNAL(startDownload(QPointF,QPointF,int,QString,QString,bool)),
            m_downloader, SLOT(download(QPointF,QPointF,int,QString,QString,bool)));

    connect(m_downloader, SIGNAL(dbg(QString)),
            this, SLOT(dbg(QString)));
    connect(m_downloader, SIGNAL(showMessage(QString)),
            this, SLOT(showMessage(QString)));
    connect(m_downloader, SIGNAL(downloadFinished()),
            this, SLOT(onDownloadFinished()));
}

MainWindow::~MainWindow()
{
    delete ui;
    m_downloader_thread->requestInterruption();
    m_downloader_thread->quit();
    m_downloader_thread->terminate();
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

void MainWindow::addMapType(const QString &name, const QString &url_pattern)
{
    ui->combo_map_type->addItem(name, url_pattern);
}

void MainWindow::dbg(const QString &message)
{
    if (m_log_buffer_size > ui->spin_log_buffer_size->value())
    {
        ui->text_browser_log->clear();
        m_log_buffer_size = 0;
    }

    ui->text_browser_log->append(message);
    ++m_log_buffer_size;
}

void MainWindow::showMessage(const QString &message)
{
    this->statusBar()->showMessage(message);
}

void MainWindow::onDownloadFinished()
{
    dbg("[Task Finished]");
    ui->btn_start->setEnabled(true);
}

void MainWindow::onBrowseFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this);

    if (!dir.isNull())
    {
        ui->line_path_to_save->setText(dir);
        QSettings settings;
        settings.setValue("MAP_SAVE_PATH", dir);
    }
}

void MainWindow::on_btn_open_folder_clicked()
{
    QString target_path = QString("file:///%1").arg(ui->line_path_to_save->text());
    QDesktopServices::openUrl(QUrl(target_path, QUrl::TolerantMode));
}

void MainWindow::on_btn_start_clicked()
{
    int z = ui->spin_zoomlevel->value();

    bool ok = false;
    QPointF geo_start = getGeoStart(&ok);
    if (!ok) return;

    QPointF geo_stop = getGeoStop(&ok);
    if (!ok) return;

    QString url_pattern = ui->combo_map_type->currentData().toString();

    ui->btn_start->setEnabled(false);
    ui->text_browser_log->setFocus();
    ui->statusBar->clearMessage();

    emit startDownload(geo_start, geo_stop, z, url_pattern,
                       ui->line_path_to_save->text(), !ui->check_skip->isChecked());
}

void MainWindow::on_btn_clear_log_buffer_clicked()
{
    ui->text_browser_log->clear();
    m_log_buffer_size = 0;
}

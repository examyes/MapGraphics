#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <cmath>

#include <QApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringBuilder>

#include "guts/MapGraphicsNetwork.h"

#include <QDebug>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qDebug() << latlon2xy(31.65651936531, 120.357402896881, 13);
    qDebug() << latlon2xy(31.438766860961, 120.586399126052, 13);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_Exit_triggered()
{
    qApp->quit();
}

QPoint MainWindow::latlon2xy(qreal lat, qreal lon, int zoom)
{
    // reference: http://blog.csdn.net/youngkingyj/article/details/23365849
    qreal x = pow(2.0, zoom-1) * (lon / 180 + 1);
    qreal y = pow(2.0, zoom-1) * (1 - log(tan(lat*deg2rad) + 1/cos(lat*deg2rad)) / PI);

    return QPoint(int(x), int(y));
}

QString MainWindow::createCacheID(quint32 x, quint32 y, quint8 z)
{
    //We use % because it's more efficient to concatenate with QStringBuilder
    QString toRet = QString::number(x) % "," % QString::number(y) % "," % QString::number(z);
    return toRet;
}

bool MainWindow::cacheID2xyz(const QString &string, quint32 *x, quint32 *y, quint32 *z)
{
    QStringList list = string.split(',');
    if (list.size() != 3)
    {
        qWarning() << "Bad cacheID" << string << "cannot convert";
        return false;
    }

    bool ok = true;
    *x = list.at(0).toUInt(&ok);
    if (!ok)
        return false;
    *y = list.at(1).toUInt(&ok);
    if (!ok)
        return false;
    *z = list.at(2).toUInt(&ok);
    return ok;
}


void MainWindow::on_pushButton_clicked()
{
    int zoom = 1;
    QPoint start = latlon2xy(55.83, 67.40, zoom);
    QPoint stop = latlon2xy(-0.176, 135, zoom);

    qDebug() << "Tiles Count = " << (stop.x() - start.x()) * (stop.y() - start.y());

    MapGraphicsNetwork * network = MapGraphicsNetwork::getInstance();
    QString host = "http://mt1.google.cn/vt/lyrs=y&hl=zh-CN&gl=cn&";
    QString url = "x=%1&y=%2&z=%3";

    for (int x = start.x(); x < stop.x(); x++)
    {
        for (int y = start.y(); y < stop.y(); y++)
        {
            //Use the unique cacheID to see if this tile has already been requested
            const QString cacheID = createCacheID(x, y, zoom);
            if (_pendingRequests.contains(cacheID))
                continue;
            _pendingRequests.insert(cacheID);

            //Build the request
            const QString fetchURL = url.arg(QString::number(x),
                                             QString::number(y),
                                             QString::number(zoom));
            QNetworkRequest request(QUrl(host + fetchURL));

            //Send the request and setupd a signal to ensure we're notified when it finishes
            QNetworkReply * reply = network->get(request);
            _pendingReplies.insert(reply, cacheID);

            connect(reply, SIGNAL(finished()),
                    this, SLOT(handleNetworkRequestFinished()));
        }
    }
}

void MainWindow::handleNetworkRequestFinished()
{
    static int download_count = 0;
    ++download_count;

    if (download_count > 1000)
    {
        ui->text_browser_log->clear();
        download_count = 0;
    }

    QObject * sender = QObject::sender();
    QNetworkReply * reply = qobject_cast<QNetworkReply *>(sender);

    if (reply == 0)
    {
        qWarning() << "QNetworkReply cast failure";
        return;
    }

    /*
      We can do this here and use reply later in the function because the reply
      won't be deleted until execution returns to the event loop.
    */
    reply->deleteLater();

    if (!_pendingReplies.contains(reply))
    {
        qWarning() << "Unknown QNetworkReply";
        return;
    }

    //get the cacheID
    const QString cacheID = _pendingReplies.take(reply);
    _pendingRequests.remove(cacheID);

    //If there was a network error, ignore the reply
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Network Error:" << reply->errorString();
        return;
    }

    //Convert the cacheID back into x,y,z tile coordinates
    quint32 x,y,z;
    if (!cacheID2xyz(cacheID,&x,&y,&z))
    {
        qWarning() << "Failed to convert cacheID" << cacheID << "back to xyz";
        return;
    }

    QByteArray bytes = reply->readAll();
    QImage * image = new QImage();

    if (!image->loadFromData(bytes))
    {
        delete image;
        qWarning() << "Failed to make QImage from network bytes";
        return;
    }

    QString filename = QString("C:/Users/examyes/Desktop/map/%3_%1_%2_s.jpg")
            .arg(x).arg(y).arg(z);
    image->save(filename);

    delete image;

    ui->text_browser_log->append("[Complete] " + filename);

    if (_pendingRequests.isEmpty())
        ui->text_browser_log->append("[Task Finished!]");


}

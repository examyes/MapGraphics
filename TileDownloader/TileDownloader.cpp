#include "TileDownloader.h"

#include <cmath>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringBuilder>
#include <QDir>
#include <QUrl>
#include <QImage>

#include "guts/MapGraphicsNetwork.h"

#include <QDebug>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

TileDownloader::TileDownloader(QObject *parent) :
    QObject(parent)
{

}

QPoint TileDownloader::latlon2xy(qreal lon, qreal lat, int zoom)
{
    // reference: http://blog.csdn.net/youngkingyj/article/details/23365849
    qreal x = pow(2.0, zoom-1) * (lon / 180 + 1);
    qreal y = pow(2.0, zoom-1) * (1 - log(tan(lat*deg2rad) + 1/cos(lat*deg2rad)) / PI);

    return QPoint(int(x), int(y));
}

QString TileDownloader::createCacheID(quint32 x, quint32 y, quint8 z)
{
    //We use % because it's more efficient to concatenate with QStringBuilder
    QString toRet = QString::number(x) % "," % QString::number(y) % "," % QString::number(z);
    return toRet;
}

bool TileDownloader::cacheID2xyz(const QString &string, quint32 *x, quint32 *y, quint32 *z)
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

void TileDownloader::download(QPointF geo_start, QPointF geo_stop, int zoom_level, const QString &url_pattern, const QString &path_to_save)
{
    m_path_to_save = path_to_save;

    int z = zoom_level;

    QPoint start = latlon2xy(geo_start.x(), geo_start.y() ,z);
    QPoint stop = latlon2xy(geo_stop.x(), geo_stop.y(), z);

    qDebug() << "Tiles Count = " << (stop.x() - start.x()) * (stop.y() - start.y());

    MapGraphicsNetwork * network = MapGraphicsNetwork::getInstance();

    for (int x = start.x(); x < stop.x(); x++)
    {
        for (int y = start.y(); y < stop.y(); y++)
        {
            //Use the unique cacheID to see if this tile has already been requested
            const QString cacheID = createCacheID(x, y, z);
            if (m_pending_requests.contains(cacheID))
                continue;
            m_pending_requests.insert(cacheID);

            //Build the request
            const QString fetchURL = url_pattern.arg(QString::number(x),
                                             QString::number(y),
                                             QString::number(z));
            QNetworkRequest request(QUrl(fetchURL + ""));

            //Send the request and setupd a signal to ensure we're notified when it finishes
            QNetworkReply * reply = network->get(request);
            m_pending_replies.insert(reply, cacheID);

            connect(reply, SIGNAL(finished()),
                    this, SLOT(handleNetworkRequestFinished()));
        }
    }
}

void TileDownloader::handleNetworkRequestFinished()
{
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

    if (!m_pending_replies.contains(reply))
    {
        qWarning() << "Unknown QNetworkReply";
        return;
    }

    //get the cacheID
    const QString cacheID = m_pending_replies.take(reply);
    m_pending_requests.remove(cacheID);

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

    QString filename = m_path_to_save + QString("/%3_%1_%2_s.jpg").arg(x).arg(y).arg(z);
    image->save(filename);

    delete image;

    qDebug() << "[Complete] " + filename;

    if (m_pending_requests.isEmpty())
        qDebug() << "[Task Finished!]";
}


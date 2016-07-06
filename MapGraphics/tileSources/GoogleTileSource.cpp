#include "GoogleTileSource.h"

#include "guts/MapGraphicsNetwork.h"

#include <cmath>
#include <QPainter>
#include <QStringBuilder>
#include <QtDebug>
#include <QNetworkReply>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

GoogleTileSource::GoogleTileSource(GoogleTileType tileType) :
    MapTileSource(), _tileType(tileType)
{
    this->setCacheMode(MapTileSource::DiskAndMemCaching);
}

GoogleTileSource::~GoogleTileSource()
{
    qDebug() << this << this->name() << "Destructing";
}

QPointF GoogleTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal x = (ll.x()+180) * (tilesOnOneEdge*tileSize)/360; // coord to pixel!
    qreal y = (1-(log(tan(PI/4+(ll.y()*deg2rad)/2)) /PI)) /2  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

QPointF GoogleTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal longitude = (qgs.x()*(360/(tilesOnOneEdge*tileSize)))-180;
    qreal latitude = rad2deg*(atan(sinh((1-qgs.y()*(2/(tilesOnOneEdge*tileSize)))*PI)));

    return QPointF(longitude, latitude);
}

quint64 GoogleTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    return pow(4.0,zoomLevel);
}

quint16 GoogleTileSource::tileSize() const
{
    return 256;
}

quint8 GoogleTileSource::minZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 0;
}

quint8 GoogleTileSource::maxZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 18;
}

QString GoogleTileSource::name() const
{
    switch(_tileType)
    {
    case MAP:
        return "Google Map Tiles";
        break;

    case SKELETON_MAP_LIGHT:
        return "Google Skeleton Map Light Tiles";
        break;

    case SKELETON_MAP_DARK:
        return "Google Skeleton Map Dark Tiles";
        break;

    case TERRAIN:
        return "Google Terrain Tiles";
        break;

    case TERRAIN_MAP:
        return "Google Terrain Map Tiles";
        break;

    case SATELLITE:
        return "Google Satellite Tiles";
        break;

    case HYBRID_SATELLITE_MAP:
        return "Google Hybrid Satellite Map Tiles";
        break;

    default:
        return "Unknown Tiles";
        break;
    }
}

QString GoogleTileSource::tileFileExtension() const
{
    if (_tileType == MAP ||
        _tileType == SKELETON_MAP_LIGHT ||
        _tileType == SKELETON_MAP_DARK)
        return "png";
    else
        return "jpg";
}

//protected
void GoogleTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    MapGraphicsNetwork * network = MapGraphicsNetwork::getInstance();

    QString host;
    QString url;

    //Figure out which server to request from based on our desired tile type
    if (_tileType == MAP)
    {
        host = "http://mt2.google.cn/vt/lyrs=m&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }
    else if (_tileType == SKELETON_MAP_LIGHT)
    {
        host = "http://mt2.google.cn/vt/lyrs=h&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }
    else if (_tileType == SKELETON_MAP_DARK)
    {
        host = "http://mt2.google.cn/vt/lyrs=r&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }
    else if (_tileType == TERRAIN)
    {
        host = "http://mt0.google.cn/vt/lyrs=t&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }
    else if (_tileType == TERRAIN_MAP)
    {
        host = "http://mt2.google.cn/vt/lyrs=p&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }
    else if (_tileType == SATELLITE)
    {
        host = "http://mt3.google.cn/vt/lyrs=s&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }
    else if (_tileType == HYBRID_SATELLITE_MAP)
    {
        host = "http://mt1.google.cn/vt/lyrs=y&hl=zh-CN&gl=cn&";
        url = "x=%1&y=%2&z=%3";
    }

    //Use the unique cacheID to see if this tile has already been requested
    const QString cacheID = this->createCacheID(x,y,z);
    if (_pendingRequests.contains(cacheID))
        return;
    _pendingRequests.insert(cacheID);

    //Build the request
    const QString fetchURL = url.arg(QString::number(x),
                                     QString::number(y),
                                     QString::number(z));
    QNetworkRequest request(QUrl(host + fetchURL));

    //Send the request and setupd a signal to ensure we're notified when it finishes
    QNetworkReply * reply = network->get(request);
    _pendingReplies.insert(reply,cacheID);

    connect(reply,
            SIGNAL(finished()),
            this,
            SLOT(handleNetworkRequestFinished()));
}

//private slot
void GoogleTileSource::handleNetworkRequestFinished()
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
    if (!MapTileSource::cacheID2xyz(cacheID,&x,&y,&z))
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

    //Figure out how long the tile should be cached
    QDateTime expireTime;
    if (reply->hasRawHeader("Cache-Control"))
    {
        //We support the max-age directive only for now
        const QByteArray cacheControl = reply->rawHeader("Cache-Control");
        QRegExp maxAgeFinder("max-age=(\\d+)");
        if (maxAgeFinder.indexIn(cacheControl) != -1)
        {
            bool ok = false;
            const qint64 delta = maxAgeFinder.cap(1).toULongLong(&ok);

            if (ok)
                expireTime = QDateTime::currentDateTimeUtc().addSecs(delta);
        }
    }

    //Notify client of tile retrieval
    this->prepareNewlyReceivedTile(x,y,z, image, expireTime);
}

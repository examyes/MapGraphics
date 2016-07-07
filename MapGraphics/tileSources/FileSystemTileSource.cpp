#include "FileSystemTileSource.h"

#include "guts/MapGraphicsNetwork.h"

#include <cmath>
#include <QPainter>
#include <QStringBuilder>
#include <QtDebug>
#include <QNetworkReply>
#include <QTimer>
#include <QDir>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

FileSystemTileSource::FileSystemTileSource(const QString &path,
                                             const QString &tile_file_extension,
                                             const QString &tile_pattern)
    : MapTileSource()
    , m_path(path)
    , m_tile_file_extension(tile_file_extension)
    , m_tile_pattern(tile_pattern)
{
    this->setCacheMode(MapTileSource::NoCaching);

    QDir dir(m_path);
    if (!dir.exists())
        qDebug() << "Dir" << m_path << "does not exist.";

    m_path = dir.absolutePath();

    if (!m_path.endsWith("/"))
        m_path.append("/");

    connect(this, SIGNAL(loadImage(quint32,quint32,quint8,QString)),
            this, SLOT(onLoadImage(quint32,quint32,quint8,QString)), Qt::QueuedConnection);
}

FileSystemTileSource::~FileSystemTileSource()
{
    qDebug() << this << this->name() << "Destructing";
}

QPointF FileSystemTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal x = (ll.x()+180) * (tilesOnOneEdge*tileSize)/360; // coord to pixel!
    qreal y = (1-(log(tan(PI/4+(ll.y()*deg2rad)/2)) /PI)) /2  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

QPointF FileSystemTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal longitude = (qgs.x()*(360/(tilesOnOneEdge*tileSize)))-180;
    qreal latitude = rad2deg*(atan(sinh((1-qgs.y()*(2/(tilesOnOneEdge*tileSize)))*PI)));

    return QPointF(longitude, latitude);
}

quint64 FileSystemTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    return pow(4.0, zoomLevel);
}

quint16 FileSystemTileSource::tileSize() const
{
    return 256;
}

quint8 FileSystemTileSource::minZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 0;
}

quint8 FileSystemTileSource::maxZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 18;
}

QString FileSystemTileSource::name() const
{
    return "FileSystemTileSource";
}

QString FileSystemTileSource::tileFileExtension() const
{
    return m_tile_file_extension;
}

//protected
void FileSystemTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    QString url = m_tile_pattern;

    //Build the request
    const QString fetchURL = url.arg(QString::number(x),
                                     QString::number(y),
                                     QString::number(z));

    emit loadImage(x, y, z, m_path + fetchURL + "." + m_tile_file_extension);


}

void FileSystemTileSource::onLoadImage(quint32 x,
                                       quint32 y,
                                       quint8 z,
                                       const QString &path)
{
    QImage *image = new QImage(path);

    //Notify client of tile retrieval
    this->prepareNewlyReceivedTile(x, y, z, image);
}

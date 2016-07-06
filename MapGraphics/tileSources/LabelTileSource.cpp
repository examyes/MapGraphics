#include "LabelTileSource.h"

#include <cmath>
#include <QPainter>
#include <QStringBuilder>
#include <QtDebug>

const qreal PI = 3.14159265358979323846;
const qreal deg2rad = PI / 180.0;
const qreal rad2deg = 180.0 / PI;

LabelTileSource::LabelTileSource() :
    MapTileSource()
{
    this->setCacheMode(MapTileSource::NoCaching);
}

LabelTileSource::~LabelTileSource()
{
    qDebug() << this << "destructing";
}

QPointF LabelTileSource::ll2qgs(const QPointF &ll, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal x = (ll.x()+180.0) * (tilesOnOneEdge*tileSize)/360.0; // coord to pixel!
    qreal y = (1-(log(tan(PI/4.0+(ll.y()*deg2rad)/2)) /PI)) /2.0  * (tilesOnOneEdge*tileSize);

    return QPoint(int(x), int(y));
}

QPointF LabelTileSource::qgs2ll(const QPointF &qgs, quint8 zoomLevel) const
{
    const qreal tilesOnOneEdge = pow(2.0,zoomLevel);
    const quint16 tileSize = this->tileSize();
    qreal longitude = (qgs.x()*(360.0/(tilesOnOneEdge*tileSize)))-180.0;
    qreal latitude = rad2deg*(atan(sinh((1.0-qgs.y()*(2.0/(tilesOnOneEdge*tileSize)))*PI)));

    return QPointF(longitude, latitude);
}

quint64 LabelTileSource::tilesOnZoomLevel(quint8 zoomLevel) const
{
    return pow(4.0, zoomLevel);
}

quint16 LabelTileSource::tileSize() const
{
    return 256;
}

quint8 LabelTileSource::minZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 0;
}

quint8 LabelTileSource::maxZoomLevel(QPointF ll)
{
    Q_UNUSED(ll)
    return 50;
}

QString LabelTileSource::name() const
{
    return "Label Tiles";
}

QString LabelTileSource::tileFileExtension() const
{
    return ".png";
}

void LabelTileSource::fetchTile(quint32 x, quint32 y, quint8 z)
{
    QImage * toRet = new QImage(this->tileSize(),
                                this->tileSize(),
                                QImage::Format_ARGB32_Premultiplied);
    //It is important to fill with transparent first!
    toRet->fill(qRgba(0, 0, 0, 127));

    QPainter painter(toRet);
    painter.setPen(Qt::black);

    painter.drawRect(0, 0, this->tileSize(), this->tileSize());

    QFont font = painter.font();
    font.setPixelSize(12);
    painter.setFont(font);

    painter.setPen(Qt::yellow);
    painter.drawText(0, 13, QString("(%1, %2)").arg(x).arg(y));

    painter.end();

    this->prepareNewlyReceivedTile(x,y,z,toRet);
}

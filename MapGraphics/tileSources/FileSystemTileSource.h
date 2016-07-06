#pragma once

#include "MapTileSource.h"
#include "MapGraphics_global.h"
#include <QSet>
#include <QHash>

//Forward declaration so that projects that import us as a library don't necessarily have to use QT += network
class QNetworkReply;

class MAPGRAPHICSSHARED_EXPORT FileSystemTileSource : public MapTileSource
{
    Q_OBJECT

public:
    explicit FileSystemTileSource(const QString &path,
                                   const QString &tile_file_extension=QString("jpg"),
                                   const QString &tile_pattern=QString("%3_%1_%2_s"));
    virtual ~FileSystemTileSource();

    virtual QPointF ll2qgs(const QPointF& ll, quint8 zoomLevel) const;

    virtual QPointF qgs2ll(const QPointF& qgs, quint8 zoomLevel) const;

    virtual quint64 tilesOnZoomLevel(quint8 zoomLevel) const;

    virtual quint16 tileSize() const;

    virtual quint8 minZoomLevel(QPointF ll);

    virtual quint8 maxZoomLevel(QPointF ll);

    virtual QString name() const;

    virtual QString tileFileExtension() const;

protected:
    virtual void fetchTile(quint32 x,
                           quint32 y,
                           quint8 z);
signals:
    void loadImage(quint32 x,
                   quint32 y,
                   quint8 z,
                   const QString &path);

private slots:
    void onLoadImage(quint32 x,
                     quint32 y,
                     quint8 z,
                     const QString &path);

private:
    QString m_path;
    QString m_tile_pattern;
    QString m_tile_file_extension;
};

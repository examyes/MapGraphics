#pragma once

#include <QObject>
#include <QPointF>
#include <QSet>
#include <QHash>

class QNetworkReply;

class TileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit TileDownloader(QObject *parent = 0);

signals:

public slots:
    void download(QPointF geo_start, QPointF geo_stop, int zoom_level, const QString &url_pattern, const QString &path_to_save);

private slots:
    void handleNetworkRequestFinished();

private:
    QPoint latlon2xy(qreal lon, qreal lat, int zoom);

    /**
     * @brief This static method takes the x,y,z of a tile and creates a unique string that is used
     * as a key in the caches to keep track of the tile.
     *
     * @param x x-coordinate of the tile
     * @param y y-coordinate of the tile
     * @param z zoom-level of the tile
     * @return QString unique cacheID
     */
    QString createCacheID(quint32 x, quint32 y, quint8 z);

    /**
     * @brief This static convenience method takes a cacheID and places the numberical x,y,z values that
     * generated the cacheID into the variables pointed to by x,y,z. Returns true on success, false on failure.
     *
     * @param string the cacheID you are converting
     * @param x where you want the x value
     * @param y where you want the y value
     * @param z where you want the z value
     * @return bool
     */
    bool cacheID2xyz(const QString & string, quint32 * x, quint32 * y, quint32 * z);

private:
    //Set used to ensure a tile with a certain cacheID isn't requested twice
    QSet<QString> m_pending_requests;

    //Hash used to keep track of what cacheID goes with what reply
    QHash<QNetworkReply *, QString> m_pending_replies;

    QString m_path_to_save;
};

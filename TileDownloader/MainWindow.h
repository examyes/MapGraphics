#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QHash>

namespace Ui {
class MainWindow;
}

class QNetworkReply;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_action_Exit_triggered();
    void on_pushButton_clicked();

    void handleNetworkRequestFinished();

private:
    QPoint latlon2xy(qreal lat, qreal lon, int zoom);

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
    Ui::MainWindow *ui;
    //Set used to ensure a tile with a certain cacheID isn't requested twice
    QSet<QString> _pendingRequests;

    //Hash used to keep track of what cacheID goes with what reply
    QHash<QNetworkReply *, QString> _pendingReplies;
};

#endif // MAINWINDOW_H

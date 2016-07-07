#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QHash>

namespace Ui {
class MainWindow;
}

class QNetworkReply;
class QThread;
class TileDownloader;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_action_Exit_triggered();
    void on_btn_open_folder_clicked();
    void on_btn_start_clicked();

    void onBrowseFolder();
    void onDownloadFinished();

    void dbg(const QString &message);
    void showMessage(const QString &message);

    void on_btn_clear_log_buffer_clicked();

signals:
    void startDownload(QPointF geo_start, QPointF geo_stop, int zoom_level,
                       const QString &url_pattern, const QString &path_to_save,
                       bool overwrite);

private:
    QPointF getGeoStart(bool *ok);
    QPointF getGeoStop(bool *ok);
    void addMapType(const QString &name, const QString &url_pattern);

private:
    Ui::MainWindow *ui;
    //Set used to ensure a tile with a certain cacheID isn't requested twice
    QSet<QString> _pendingRequests;

    //Hash used to keep track of what cacheID goes with what reply
    QHash<QNetworkReply *, QString> _pendingReplies;
    TileDownloader *m_downloader;
    QThread *m_downloader_thread;
    int m_log_buffer_size;
};

#endif // MAINWINDOW_H

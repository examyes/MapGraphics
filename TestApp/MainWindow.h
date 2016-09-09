#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class QComboBox;
class QSpinBox;

class MapGraphicsView;
class MapGraphicsScene;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionExit_triggered();

    void onMousePositionChanged(QPointF viewPos, QPointF ll);
    void onZoomLevelChanged(quint8 nZoom);

    void setZoomLevel(int i);
    void setRotation(int i);

private:
    void addLine(MapGraphicsScene * scene);

private:
    Ui::MainWindow *ui;
    MapGraphicsView *m_view;
    QSpinBox *m_spin_zoomlevel;
    QSpinBox *m_spin_rotation;
    int m_current_roration;

};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MapGraphicsView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_actionExit_triggered();

    void on_actionRotate_10_triggered();

private:
    Ui::MainWindow *ui;
    MapGraphicsView *m_view;
};

#endif // MAINWINDOW_H

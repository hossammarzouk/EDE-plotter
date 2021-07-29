#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void OpenDir();
    bool ReadFile(const QString &tempFile, const QString &flag);
    void PlotEDE();
    void PlotHDE();

    void ClearData();

private slots:
    void on_actionOpen_triggered();
    void horzScrollBarChanged(int value);

    void on_window_comboBox_currentTextChanged(const QString &arg1);

     void on_files_comboBox_textActivated(const QString &arg1);
     void dragEnterEvent(QDragEnterEvent* e);
     void dropEvent(QDropEvent* e);
     void on_pushButton_clicked();

     void on_horizontal_radioButton_clicked();

     void on_Vertical_radioButton_clicked();

     void on_limits_checkBox_stateChanged(int arg1);

     void on_reset_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    QVector<double> SampleCounter, Ex, Ey, Hx, Hy, Hz;
    QPointer<QCPGraph> mGraph1, mGraph2, mGraph3;
    int isEDE;
};
#endif // MAINWINDOW_H

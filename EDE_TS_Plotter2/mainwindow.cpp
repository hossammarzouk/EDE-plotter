#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <string>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //  this->setFixedSize(this->size()); //prevent window resize
    ui->plot->setInteractions(QCP::iRangeZoom| QCP::iRangeDrag);
    ui->plot->close();
    ui->progressBar->hide();
    ui->temp_label->setAcceptDrops(true);

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionOpen_triggered()
{
    QDir dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                 "/home",
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    QStringList info_files = dir.entryList(QStringList() << "*.ets" << "*.ETS",QDir::Files);

    ui->path_label->setText(dir.absolutePath());

    if (info_files.length() == 0)
        QMessageBox::warning(this,"Empty!!!","No ETS files found");
    else{
        ui->files_comboBox->clear();
        Sleep(100);
        ui->files_comboBox->addItems(info_files);
        if(ReadFile(info_files[0],"OneFile") ==true)
            PlotEDE();
        else
            QMessageBox::critical(this,"Error!!!","Unable to read ets file format!");
    }
}


bool MainWindow::ReadFile(const QString &tempFile,const QString &flag)
{
    if(flag == "OneFile"){
        ClearData();
    }
    QString datafile = ui->path_label->text() + "/" + tempFile;
    QFile file(datafile);
    if (!file.open(QIODevice::ReadOnly)){
        QMessageBox::critical(this,"Error!!!","Error reading data file");
        return false;
    }
    else{
        //ui->statusbar->showMessage("File loaded successfully");
        QDataStream in(&file);
        in.setFloatingPointPrecision(QDataStream::SinglePrecision);
        in.setByteOrder(QDataStream::LittleEndian);
        INT32 a ;

        while (in.atEnd() != true){
            in >> a;
            SampleCounter.append(a) ;
            in >> a;
            Ex.append(a*1.164153218e-6) ;
            in >> a;
            Ey.append(a*1.164153218e-6) ;
        }
        file.close();
        return true;

    }
}

void MainWindow::PlotEDE()
{
    ui->temp_label->hide();
    ui->plot->show();
    ui->plot->update();
    //  ui->plot->addGraph();
    ui->plot->plotLayout()->clear();
    QCPAxisRect *tobAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(0, 0, tobAxisRect);
    tobAxisRect->setupFullAxesBox(true);
    tobAxisRect->axis(QCPAxis::atLeft)->setLabel("Ex(mV)");
    //tobAxisRect->setRangeDrag(Qt::Horizontal);

    QCPAxisRect *bottomAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(1, 0, bottomAxisRect);
    bottomAxisRect->setupFullAxesBox(true);
    bottomAxisRect->axis(QCPAxis::atLeft)->setLabel("Ey(mV)");
    bottomAxisRect->axis(QCPAxis::atBottom)->setLabel("Sample counter");
    //bottomAxisRect->setRangeDrag(Qt::Horizontal);

    QList<QCPAxis*> allAxes;
    allAxes << bottomAxisRect->axes() << tobAxisRect->axes();
    foreach (QCPAxis *axis, allAxes)
    {
        axis->setLayer("axes");
        axis->grid()->setLayer("grid");

    }

    //ui->plot->graph(0)->setData(SampleCounter,Ex);
    ui->plot->plotLayout()->updateLayout();
    ui->plot->rescaleAxes();

    QString window = ui->window_comboBox->currentText();
    double win_length = window.toDouble();
    mGraph1 = ui->plot->addGraph(tobAxisRect->axis(QCPAxis::atBottom), tobAxisRect->axis(QCPAxis::atLeft));
    mGraph1->setData(SampleCounter,Ex);
    mGraph1->rescaleAxes();
    ui->plot->xAxis->setRange(SampleCounter[0], SampleCounter[0]+win_length);

    mGraph2 = ui->plot->addGraph(bottomAxisRect->axis(QCPAxis::atBottom), bottomAxisRect->axis(QCPAxis::atLeft));
    mGraph2->setData(SampleCounter,Ey);
    mGraph2->rescaleAxes();
    //ui->plot->xAxis2->setRange(SampleCounter[0], SampleCounter[0]+win_length);
    mGraph1->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);
    mGraph2->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);
    //ui->plot->xAxis->setTickLabels(false);
    //ui->horizontalScrollBar->setRange(SampleCounter[0],SampleCounter[SampleCounter.length()]);
    connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(mGraph1->keyAxis(), SIGNAL(rangeChanged(QCPRange)), mGraph2->keyAxis(), SLOT(setRange(QCPRange)));
    connect(mGraph2->keyAxis(), SIGNAL(rangeChanged(QCPRange)), mGraph1->keyAxis(), SLOT(setRange(QCPRange)));
if (ui->Vertical_radioButton->isChecked()){
    tobAxisRect->setRangeZoom(Qt::Vertical);
    bottomAxisRect->setRangeZoom(Qt::Vertical);
}
else{
    tobAxisRect->setRangeZoom(Qt::Horizontal);
    bottomAxisRect->setRangeZoom(Qt::Horizontal);

}
ui->plot->replot();
    ui->plot->update();
}


void MainWindow::on_window_comboBox_currentTextChanged(const QString &arg1)
{
    if (ui->files_comboBox->currentText().length() != 0){
        if(arg1 != "All"){
            double win_length = arg1.toDouble();
            //ui->plot->xAxis->setRange(ui->plot->xAxis->range().upper, win_length, Qt::AlignLeft);
            //ui->plot->xAxis->setRange(SampleCounter[0], SampleCounter[0]+win_length);
            mGraph1->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);
        }
        else {
            ui->plot->rescaleAxes();
        }
        ui->plot->replot();
    }
}
void MainWindow::on_pushButton_clicked()
{
    if (ui->files_comboBox->currentText().length() !=0 )
    {
        ClearData();

        ui->progressBar->show();
        for(int i = 0; i < ui->files_comboBox->count();i++)
        {
            ui->progressBar->setValue(100*i/(ui->files_comboBox->count()-1));
            ReadFile(ui->files_comboBox->itemText(i), "AllFiles");
        }

        PlotEDE();
        ui->plot->rescaleAxes();
        ui->plot->replot();
        //      ui->horizontalScrollBar->setEnabled(false);
        //        ui->window_comboBox->setEnabled(false);
        Sleep(1000);
        ui->progressBar->hide();

    }
}


void MainWindow::horzScrollBarChanged(int value)
{
    int value2;
    if (value == 0){
        value2 = 0 +SampleCounter[0]-1;
        ui->plot->xAxis->setRange(value2, ui->plot->xAxis->range().size(), Qt::AlignLeft);
    }
    else{
        value2 = (SampleCounter.length()*value/100)+SampleCounter[0]-1;
        ui->plot->xAxis->setRange(value2, ui->plot->xAxis->range().size(), Qt::AlignRight);
    }
    //  ui->plot->xAxis->setRange(value/100.0, ui->plot->xAxis->range().size(), Qt::AlignCenter);
    //mGraph1->rescaleValueAxis(true);
    //mGraph2->rescaleValueAxis(true);

    ui->plot->replot();

}
void  MainWindow::ClearData()
{
    SampleCounter.clear();
    Ex.clear();
    Ey.clear();

}

void MainWindow::on_files_comboBox_textActivated(const QString &arg1)
{
    horzScrollBarChanged(0);

    ReadFile(arg1,"OneFile");
    PlotEDE();
}


void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
    if( e->mimeData()->hasUrls() )
    {
        e->acceptProposedAction();
    }
    else
    {
        e->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent* e)
{
    if( e->mimeData()->hasUrls() )
    {
        QList<QUrl> list = e->mimeData()->urls();
        QString temmp;

        QDir dir = list[0].toLocalFile();


        QStringList info_files = dir.entryList(QStringList() << "*.ets" << "*.ETS",QDir::Files);
        ui->path_label->setText(dir.absolutePath());

        if (info_files.length() == 0)
            QMessageBox::warning(this,"Empty!!!","No ETS files found");
        else{
            ui->files_comboBox->clear();
            ui->files_comboBox->addItems(info_files);
            if(ReadFile(info_files[0],"OneFile") ==true)
                PlotEDE();
            else
                QMessageBox::critical(this,"Error!!!","Unable to read ets file format!");
        }
    }
    else
    {
        e->ignore();
    }
}


void MainWindow::on_horizontal_radioButton_clicked()
{
   ui->plot->axisRect(0)->setRangeZoom(Qt::Horizontal);
   ui->plot->axisRect(1)->setRangeZoom(Qt::Horizontal);
}

void MainWindow::on_Vertical_radioButton_clicked()
{
    ui->plot->axisRect(0)->setRangeZoom(Qt::Vertical);
    ui->plot->axisRect(1)->setRangeZoom(Qt::Vertical);

}

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
    QStringList info_files2 = dir.entryList(QStringList() << "*.mts" << "*.MTS",QDir::Files);

    if (info_files.length() != 0)
    {  isEDE = 1;
    }
    else if(info_files2.length() != 0)
    { isEDE = 0;
    }
    else{
        QMessageBox::warning(this,"Empty!!!","No ETS/HTS files found");
        return;
    }
    ui->path_label->setText(dir.absolutePath());

    switch (isEDE)
    {
    case 1: // isEDE
        ui->files_comboBox->clear();
        ui->files_comboBox->addItems(info_files);
        if(ReadFile(info_files[0],"OneFile") ==true)
            PlotEDE();
        else
            QMessageBox::critical(this,"Error!!!","Unable to read ets file format!");
        break;

    case 0: //HDE
        ui->files_comboBox->clear();
        ui->files_comboBox->addItems(info_files2);
        if(ReadFile(info_files2[0],"OneFile") ==true)
            PlotHDE();
        else
            QMessageBox::critical(this,"Error!!!","Unable to read mts file format!");
        break;

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

        if(isEDE == 1){
            while (in.atEnd() != true){
                in >> a;
                SampleCounter.append(a) ;
                in >> a;
                Ex.append(a*1.164153218e-6/10) ;   // LSB/gain
                in >> a;
                Ey.append(a*1.164153218e-6/10) ;
            }
        }
        else if (isEDE == 0){
            while (in.atEnd() != true){
                in >> a;
                SampleCounter.append(a) ;
                in >> a;
                Hx.append(a*1.164153218e-6/10) ;   // LSB/gain
                in >> a;
                Hy.append(a*1.164153218e-6/10) ;
                in >> a;
                Hz.append(a*1.164153218e-6/10) ;
            }
        }
        file.close();
        return true;

    }
}

void MainWindow::PlotEDE()
{
    ui->plot->plotLayout()->clear(); // clear default axis rect so we can start from scratch
    ui->plot->clearGraphs();

    ui->temp_label->hide();
    ui->plot->show();
    ui->plot->update();
    QCPAxisRect *tobAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(0, 0, tobAxisRect);
    tobAxisRect->setupFullAxesBox(true);
    tobAxisRect->axis(QCPAxis::atLeft)->setLabel("Ex(mV)");
    tobAxisRect->axis(QCPAxis::atLeft)->setLabelFont(QFont("Times", 10, QFont::Bold));


    QCPAxisRect *bottomAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(1, 0, bottomAxisRect);
    bottomAxisRect->setupFullAxesBox(true);
    bottomAxisRect->axis(QCPAxis::atLeft)->setLabel("Ey(mV)");
    bottomAxisRect->axis(QCPAxis::atBottom)->setLabel("Sample counter");
    bottomAxisRect->axis(QCPAxis::atLeft)->setLabelFont(QFont("Times", 10, QFont::Bold));
    bottomAxisRect->axis(QCPAxis::atBottom)->setLabelFont(QFont("Times", 10, QFont::Bold));

    QList<QCPAxis*> allAxes;
    allAxes << bottomAxisRect->axes() << tobAxisRect->axes();
    foreach (QCPAxis *axis, allAxes)
    {
        axis->setLayer("axes");
        axis->grid()->setLayer("grid");

    }

    ui->plot->plotLayout()->updateLayout();
    ui->plot->rescaleAxes();

    QString window = ui->window_comboBox->currentText();
    double win_length = window.toDouble();
    mGraph1 = ui->plot->addGraph(tobAxisRect->axis(QCPAxis::atBottom), tobAxisRect->axis(QCPAxis::atLeft));
    mGraph1->setPen(QPen(Qt::red));
    mGraph1->setData(SampleCounter,Ex);

    mGraph2 = ui->plot->addGraph(bottomAxisRect->axis(QCPAxis::atBottom), bottomAxisRect->axis(QCPAxis::atLeft));
    mGraph2->setData(SampleCounter,Ey);

    mGraph1->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);
    mGraph2->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);

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

    if(ui->limits_checkBox->isChecked() == true){
        mGraph1->rescaleValueAxis(false,true);
        mGraph2->rescaleValueAxis(false,true);

        tobAxisRect->setRangeDrag(Qt::Horizontal);
        bottomAxisRect->setRangeDrag(Qt::Horizontal);
            }

    else {

        tobAxisRect->setRangeDrag(Qt::Horizontal|Qt::Vertical);
        bottomAxisRect->setRangeDrag(Qt::Horizontal|Qt::Vertical);
    }
    ui->plot->replot();
}

void MainWindow::PlotHDE()
{
    ui->plot->plotLayout()->clear(); // clear default axis rect so we can start from scratch
    ui->plot->clearGraphs();
    ui->temp_label->hide();
    ui->plot->show();
    ui->plot->update();

    QCPAxisRect *tobAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(0, 0, tobAxisRect);
    tobAxisRect->setupFullAxesBox(true);
    tobAxisRect->axis(QCPAxis::atLeft)->setLabel("Hx(mV)");
    tobAxisRect->axis(QCPAxis::atLeft)->setLabelFont(QFont("Times", 10, QFont::Bold));


    QCPAxisRect *middleAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(1, 0, middleAxisRect);
    middleAxisRect->setupFullAxesBox(true);
    middleAxisRect->axis(QCPAxis::atLeft)->setLabel("Hy(mV)");
    middleAxisRect->axis(QCPAxis::atLeft)->setLabelFont(QFont("Times", 10, QFont::Bold));

    QCPAxisRect *bottomAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(2, 0, bottomAxisRect);
    bottomAxisRect->setupFullAxesBox(true);
    bottomAxisRect->axis(QCPAxis::atLeft)->setLabel("Hz(mV)");
    bottomAxisRect->axis(QCPAxis::atBottom)->setLabel("Sample counter");
    bottomAxisRect->axis(QCPAxis::atLeft)->setLabelFont(QFont("Times", 10, QFont::Bold));
    bottomAxisRect->axis(QCPAxis::atBottom)->setLabelFont(QFont("Times", 10, QFont::Bold));

    QList<QCPAxis*> allAxes;
    allAxes << bottomAxisRect->axes() << tobAxisRect->axes() << middleAxisRect->axes();
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
    mGraph1->setPen(QPen(Qt::red));
    mGraph1->setData(SampleCounter,Hx);

    mGraph2 = ui->plot->addGraph(middleAxisRect->axis(QCPAxis::atBottom), middleAxisRect->axis(QCPAxis::atLeft));
    mGraph2->setPen(QPen(Qt::blue));
    mGraph2->setData(SampleCounter,Hy);

    mGraph3 = ui->plot->addGraph(bottomAxisRect->axis(QCPAxis::atBottom), bottomAxisRect->axis(QCPAxis::atLeft));
    mGraph3->setPen(QPen(Qt::magenta));
    mGraph3->setData(SampleCounter,Hz);


    mGraph1->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);
    mGraph2->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);
    mGraph3->keyAxis()->setRange(SampleCounter[0], SampleCounter[0]+win_length);

    //ui->plot->xAxis->setTickLabels(false);
    connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
    connect(mGraph1->keyAxis(), SIGNAL(rangeChanged(QCPRange)), mGraph2->keyAxis(), SLOT(setRange(QCPRange)));
    connect(mGraph2->keyAxis(), SIGNAL(rangeChanged(QCPRange)), mGraph1->keyAxis(), SLOT(setRange(QCPRange)));
    connect(mGraph1->keyAxis(), SIGNAL(rangeChanged(QCPRange)), mGraph3->keyAxis(), SLOT(setRange(QCPRange)));
    connect(mGraph3->keyAxis(), SIGNAL(rangeChanged(QCPRange)), mGraph2->keyAxis(), SLOT(setRange(QCPRange)));

    if(ui->limits_checkBox->isChecked() == true){

        mGraph1->rescaleValueAxis(false,true);
        mGraph2->rescaleValueAxis(false,true);
        mGraph3->rescaleValueAxis(false,true);

        tobAxisRect->setRangeDrag(Qt::Horizontal);
        middleAxisRect->setRangeDrag(Qt::Horizontal);
        bottomAxisRect->setRangeDrag(Qt::Horizontal);
            }

    else {

        tobAxisRect->setRangeDrag(Qt::Horizontal|Qt::Vertical);
        middleAxisRect->setRangeDrag(Qt::Horizontal|Qt::Vertical);
        bottomAxisRect->setRangeDrag(Qt::Horizontal|Qt::Vertical);
    }

    if (ui->Vertical_radioButton->isChecked()){
        tobAxisRect->setRangeZoom(Qt::Vertical);
        middleAxisRect->setRangeZoom(Qt::Vertical);
        bottomAxisRect->setRangeZoom(Qt::Vertical);
    }
    else{
        tobAxisRect->setRangeZoom(Qt::Horizontal);
        middleAxisRect->setRangeZoom(Qt::Horizontal);
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

            if(ui->limits_checkBox->isChecked() == true){

                mGraph1->rescaleValueAxis(false,true);
                mGraph2->rescaleValueAxis(false,true);
                if(isEDE == 0)
                    mGraph3->rescaleValueAxis(false,true);
            }
        }
        else {
            ui->plot->rescaleAxes();
            ui->limits_checkBox->setChecked(true);

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

        if(isEDE == 1)
            PlotEDE();
        else if (isEDE == 0)
            PlotHDE();
        ui->plot->rescaleAxes();
        ui->limits_checkBox->setChecked(true);
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

    // stop autoscale y axis

    if(ui->limits_checkBox->isChecked() == true){

        mGraph1->rescaleValueAxis(false,true);
        mGraph2->rescaleValueAxis(false,true);
        if(isEDE == 0)
            mGraph3->rescaleValueAxis(false,true);
    }
    ui->plot->replot();

}
void  MainWindow::ClearData()
{
    SampleCounter.clear();
    Ex.clear();
    Ey.clear();
    Hx.clear();
    Hy.clear();
    Hz.clear();

}

void MainWindow::on_files_comboBox_textActivated(const QString &arg1)
{
    horzScrollBarChanged(0);

    ReadFile(arg1,"OneFile");
    if(isEDE == 1)
        PlotEDE();
    else if (isEDE == 0)
        PlotHDE();
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
        QStringList info_files2 = dir.entryList(QStringList() << "*.mts" << "*.MTS",QDir::Files);

        if (info_files.length() != 0)
        {  isEDE = 1;
        }
        else if(info_files2.length() != 0)
        { isEDE = 0;
        }
        else{
            QMessageBox::warning(this,"Empty!!!","No ETS/HTS files found");
            return;
        }
        ui->path_label->setText(dir.absolutePath());

        switch (isEDE)
        {
        case 1: // isEDE
            ui->files_comboBox->clear();
            ui->files_comboBox->addItems(info_files);
            if(ReadFile(info_files[0],"OneFile") ==true)
                PlotEDE();
            else
                QMessageBox::critical(this,"Error!!!","Unable to read ets file format!");
            break;

        case 0: //HDE
            ui->files_comboBox->clear();
            ui->files_comboBox->addItems(info_files2);
            if(ReadFile(info_files2[0],"OneFile") ==true)
                PlotHDE();
            else
                QMessageBox::critical(this,"Error!!!","Unable to read mts file format!");
            break;

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
    if(isEDE == 0)
        ui->plot->axisRect(2)->setRangeZoom(Qt::Horizontal);

}

void MainWindow::on_Vertical_radioButton_clicked()
{
    ui->plot->axisRect(0)->setRangeZoom(Qt::Vertical);
    ui->plot->axisRect(1)->setRangeZoom(Qt::Vertical);
    if(isEDE == 0)
        ui->plot->axisRect(2)->setRangeZoom(Qt::Vertical);
}

void MainWindow::on_limits_checkBox_stateChanged(int arg1)
{
    if(ui->limits_checkBox->isChecked() == true){

        mGraph1->rescaleValueAxis(false,true);
        mGraph2->rescaleValueAxis(false,true);

        ui->plot->axisRect(0)->setRangeDrag(Qt::Horizontal);
        ui->plot->axisRect(1)->setRangeDrag(Qt::Horizontal);
        if(isEDE == 0){
            mGraph3->rescaleValueAxis(false,true);
            ui->plot->axisRect(2)->setRangeDrag(Qt::Horizontal);}
    }
else{
        ui->plot->axisRect(0)->setRangeDrag(Qt::Horizontal|Qt::Vertical);
        ui->plot->axisRect(1)->setRangeDrag(Qt::Horizontal|Qt::Vertical);
        if(isEDE == 0){
            ui->plot->axisRect(2)->setRangeDrag(Qt::Horizontal | Qt::Vertical);}

    }
    ui->plot->replot();

}

void MainWindow::on_reset_pushButton_clicked()
{
    if(ui->files_comboBox->count() != 0)
    {
        ui->files_comboBox->setCurrentIndex(0);
        ui->window_comboBox->setCurrentIndex(4);
        ui->limits_checkBox->setChecked(true);
        ui->horizontalScrollBar->setValue(0);

        ReadFile(ui->files_comboBox->itemText(0),"OneFile");
        if (isEDE == 1)
            PlotEDE();
        else if(isEDE == 0)
            PlotHDE();
    }
}

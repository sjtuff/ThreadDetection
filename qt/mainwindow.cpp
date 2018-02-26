#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include "xlsxdocument.h"
#include <QDebug>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //时间显示
    QTimer *time_show;
    time_show=new QTimer(this);
    time_show->start(200);
    connect(time_show,SIGNAL(timeout()),this,SLOT(set_current_time()));

    //设置背景颜色为浅灰色
    QPalette pal_initial(this->palette());
    pal_initial.setColor(QPalette::Background, QColor(195, 195, 195));
    this->setAutoFillBackground(true);
    this->setPalette(pal_initial);

    all_widgets=ui->stackedWidget;
    label_themes=ui->label_themes;
    label_show_error=ui->label_show_error;

    //按钮组合
    buttons=new QButtonGroup();
    buttons->addButton(ui->pbt_start,0);
    buttons->addButton(ui->pbt_view,1);
    buttons->addButton(ui->pbt_result,2);
    buttons->addButton(ui->pbt_setting,3);
    buttons->addButton(ui->pbt_error,4);
    buttons->addButton(ui->pbt_diagnose,5);
    buttons->addButton(ui->pbt_calibration,6);
    buttons->addButton(ui->pbt_debug,7);
    buttons->addButton(ui->pbt_exit,8);
    connect(buttons,SIGNAL(buttonClicked(int)),this,SLOT(get_pressed_buttons(int)));

    //按钮按下处于保存状态设定
    ui->pbt_start->setCheckable(true);
    ui->pbt_start->setAutoExclusive(true);
    ui->pbt_view->setCheckable(true);
    ui->pbt_view->setAutoExclusive(true);
    ui->pbt_result->setCheckable(true);
    ui->pbt_result->setAutoExclusive(true);
    ui->pbt_diagnose->setCheckable(true);
    ui->pbt_diagnose->setAutoExclusive(true);
    ui->pbt_setting->setCheckable(true);
    ui->pbt_setting->setAutoExclusive(true);
    ui->pbt_error->setCheckable(true);
    ui->pbt_error->setAutoExclusive(true);
    ui->pbt_calibration->setCheckable(true);
    ui->pbt_calibration->setAutoExclusive(true);
    ui->pbt_debug->setCheckable(true);
    ui->pbt_debug->setAutoExclusive(true);
    ui->pbt_exit->setCheckable(true);
    ui->pbt_exit->setAutoExclusive(true);

    all_widgets->setCurrentIndex(0);

    //表格页设置
    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setCalendarPopup(true);
    ui->tableWidget->horizontalScrollBar()->setStyleSheet("QScrollBar{background:gray;}");
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:rgb(0, 160, 255);}");
    //ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->verticalHeader()->setVisible(false);

    ui->tableWidget_2->horizontalHeader()->setStyleSheet("QHeaderView::section{background:rgb(255, 255, 127);}");
    ui->tableWidget_2->verticalHeader()->setStyleSheet("QHeaderView::section{background:rgb(255, 255, 127);}");
    ui->tableWidget_2->resizeColumnsToContents();

    ui->tableWidget_3->horizontalHeader()->setStyleSheet("QHeaderView::section{background:rgb(255, 255, 127);}");
    ui->tableWidget_3->verticalHeader()->setVisible(false);
    ui->tableWidget_3->setSpan(0,0,3,1);
    ui->tableWidget_3->setSpan(0,1,3,1);

    ui->tableWidget_4->verticalHeader()->setVisible(false);
    ui->tableWidget_4->horizontalHeader()->setVisible(false);

    //诊断页设计
    img_button_circle.load(":/button/circle_in.png");
    img_button_circle_in.load(":/button/circle_in.png");
    img_button_circle=img_button_circle.scaled(ui->label_128->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    ui->label_128->setPixmap(QPixmap::fromImage(img_button_circle));




}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::set_current_time()
{
    if(time_show)
    {
        QString str_time;
        QDateTime current_one=QDateTime::currentDateTime();
        str_time=current_one.toString(QString("yyyy/MM/dd hh:mm:ss"));
        ui->time_label->setText(str_time);
    }

}

//组合按钮响应

void MainWindow::get_pressed_buttons(int i)
{

    time_show=true;
    switch (i) {
    case 0:
    {
        QMessageBox::StandardButton start_detection=QMessageBox::question(NULL, tr("Start Determing"),
                                                                          QString::fromLocal8Bit("确定要开始进行螺纹检测吗？"),
                                                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if(start_detection== QMessageBox::Yes)
        {
            //excel操作
            QString data_name;
            QDateTime current_one=QDateTime::currentDateTime();
            data_name=current_one.toString(QString("yyyy.MM.dd"));
            data_name="./"+data_name+".xlsx";
            //qDebug()<<data_name;
            QXlsx::Document xlsx(data_name);
            int i=1;
            QVariant a = xlsx.read(i,1);
            while (a.isValid())
            {
                qDebug()<<a;
                i++;
                a = xlsx.read(i,1);
            }
            xlsx.write(i,1,QString("Fanfan"));
            xlsx.write(i+1,1,QString("Zhangzhe"));
            xlsx.write(i+2,1,QString("Xukai"));
            xlsx.saveAs(data_name);



        }
        break;
    }
    case 1:
    {
        //设置背景颜色
        QPalette pal_view(this->palette());
        pal_view.setColor(QPalette::Background, QColor(0, 213, 0));
        this->setAutoFillBackground(true);
        this->setPalette(pal_view);
        //转到一览页面
        all_widgets->setCurrentIndex(i);
        //标签设置成“一览”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("一 览"));
        label_themes->setPalette(pal_view);
//        ui->comboBox_diameter->setCurrentIndex(1);
        break;
    }

    case 2:
    {
        //设置背景颜色
        QPalette pal_result(this->palette());
        pal_result.setColor(QPalette::Background, QColor(255, 255, 127));
        this->setAutoFillBackground(true);
        this->setPalette(pal_result);
        //转到结果页面
        all_widgets->setCurrentIndex(2);
        //标签设置成“结果”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("结 果"));
        label_themes->setPalette(pal_result);
        break;
    }

    case 3:
    {
        //设置背景颜色
        QPalette pal_setup(this->palette());
        pal_setup.setColor(QPalette::Background, QColor(0, 255, 255));
        this->setAutoFillBackground(true);
        this->setPalette(pal_setup);
        //转到设定页面
        all_widgets->setCurrentIndex(i);
        //标签设置成“设定”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("设 定"));
        label_themes->setPalette(pal_setup);
        break;

    }

    case 4:
    {
        //设置背景颜色
        QPalette pal_error(this->palette());
        pal_error.setColor(QPalette::Background, QColor(255, 85, 0));
        this->setAutoFillBackground(true);
        this->setPalette(pal_error);
        //转到诊断页面
        all_widgets->setCurrentIndex(i);
        //标签设置成“报警”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("报 警"));
        label_themes->setPalette(pal_error);
        break;

    }

    case 5:
    {
        //设置背景颜色
        QPalette pal_diagnose(this->palette());
        pal_diagnose.setColor(QPalette::Background, QColor(5, 109, 244));
        this->setAutoFillBackground(true);
        this->setPalette(pal_diagnose);
        //转到诊断页面
        all_widgets->setCurrentIndex(i);
        //标签设置成“诊断”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("诊 断"));
        label_themes->setPalette(pal_diagnose);
        break;
    }

    case 6:
    {
        //设置背景颜色
        QPalette pal_calibration(this->palette());
        pal_calibration.setColor(QPalette::Background, QColor(255, 170, 127));
        this->setAutoFillBackground(true);
        this->setPalette(pal_calibration);
        //转到标定页面
        all_widgets->setCurrentIndex(i);
        //标签设置成“标定”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("标 定"));
        label_themes->setPalette(pal_calibration);
        break;

    }

    case 7:
    {
        //设置背景颜色
        QPalette pal_debug(this->palette());
        pal_debug.setColor(QPalette::Background, QColor(170, 170, 0));
        this->setAutoFillBackground(true);
        this->setPalette(pal_debug);
        //转到调试页面
        all_widgets->setCurrentIndex(i);
        //标签设置成“调试”
        label_themes->setAutoFillBackground(true);
        label_themes->setText(QString::fromLocal8Bit("调 试"));
        label_themes->setPalette(pal_debug);
        break;

    }
    case 8:
    {
        QMessageBox::StandardButton start_detection=QMessageBox::question(NULL, tr("Start Determing"),
                                                                          QString::fromLocal8Bit("确定要退出吗？"),
                                                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if(start_detection== QMessageBox::Yes)
        {






        }
        break;

    }
    default:
        break;
    }

}


void MainWindow::on_dateEdit_userDateChanged(const QDate &date)
{
    QString a=date.toString();
    qDebug()<<a;
}

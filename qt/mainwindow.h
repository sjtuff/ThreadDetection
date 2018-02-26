#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QTableWidget>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool time_show=false;
    QImage img_button_circle;
    QImage img_button_circle_in;


private:
    Ui::MainWindow *ui;
    QStackedWidget *all_widgets;
    QButtonGroup *buttons;
    QLabel *label_themes;
    QLabel *label_show_error;

private slots:
    void set_current_time();
    void get_pressed_buttons(int);
    void on_dateEdit_userDateChanged(const QDate &date);
};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QMainWindow>
#include "pe/cofffile.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
    /* Resets the state of the window to get ready to
     * load a new project */
    void reset();
    
    /* Updates the PE Info tab */
    void updatePEInfo(CoffFilePtr cofffile);

private:
    Ui::MainWindow *ui_;

private slots:
    void on_actionNew_triggered();
};

#endif // MAINWINDOW_H

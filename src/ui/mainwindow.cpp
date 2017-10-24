#include "mainwindow.h"
#include "log.h"
#include "logmodel.h"
#include "pe/pefile.h"
#include "ui_mainwindow.h"

#include <QStringListModel>
#include <isa.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    ui_->textConsole->setModel(LogModel::get());
}

MainWindow::~MainWindow()
{
    delete ui_;
}

void MainWindow::on_actionNew_triggered()
{
    Log::normal("Opening new file");

    QString path = QFileDialog::getOpenFileName(
        this, tr("Open File"), QString(),
        tr("Binaries (*.exe *.dll);;All Files (*)"));

    if (!path.isEmpty())
    {
        QFile file(path);
        if (!file.open(QFile::ReadOnly))
        {
            Log::error(QString("Failed to open file: ") + file.errorString());
        }

        PEFilePtr pefile = std::make_shared<PEFile>();

        if (pefile->parse(&file))
        {
            ISA::get()->projectHandler()->open(pefile);
        }
    }
}


void MainWindow::reset()
{
    ui_->peInfo->updateFile(nullptr);
}


void MainWindow::updatePEInfo(CoffFilePtr cofffile)
{
    ui_->peInfo->updateFile(cofffile);
}

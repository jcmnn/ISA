#include "peinfowidget.h"
#include "log.h"
#include "ui_peinfowidget.h"
#include <functional>

PEInfoWidget::PEInfoWidget(QWidget *parent)
    : QWidget(parent), ui_(new Ui::PEInfoWidget)
{
    ui_->setupUi(this);

    ui_->tableCoff->setModel(&coffModel_);
    ui_->tableOptional->setModel(&optionalModel_);
    ui_->tableSection->setModel(&sectionModel_);
    connect(&coffModel_, &PECoffModel::dataChanged, this,
            &PEInfoWidget::resizeCoff);
    connect(&optionalModel_, &PEOptionalModel::dataChanged, this,
            &PEInfoWidget::resizeOptional);
    connect(&sectionModel_, &PESectionModel::dataChanged, this,
            &PEInfoWidget::resizeSection);

    ui_->tableCoff->verticalHeader()->show();
    ui_->tableOptional->verticalHeader()->show();
    ui_->tableSection->horizontalHeader()->show();

    resizeCoff();
    resizeOptional();
    resizeSection();
}

PEInfoWidget::~PEInfoWidget()
{
    delete ui_;
}

void PEInfoWidget::updateFile(PEFilePtr file)
{
    coffModel_.setFile(file);
    optionalModel_.setFile(file);
    sectionModel_.setFile(file);
}

void PEInfoWidget::resizeCoff()
{
    ui_->tableCoff->resizeRowsToContents();
    ui_->tableCoff->resizeColumnsToContents();
}

void PEInfoWidget::resizeOptional()
{
    ui_->tableOptional->resizeRowsToContents();
    ui_->tableOptional->resizeColumnsToContents();
}

void PEInfoWidget::resizeSection()
{
    ui_->tableSection->resizeRowsToContents();
    ui_->tableSection->resizeColumnsToContents();
}

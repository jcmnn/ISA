#ifndef PEINFOWIDGET_H
#define PEINFOWIDGET_H

#include "pe/pefile.h"
#include <QWidget>

namespace Ui
{
class PEInfoWidget;
}

class PEInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PEInfoWidget(QWidget *parent = 0);
    ~PEInfoWidget();

public slots:
    void updateFile(CoffFilePtr file);

    // Resize the coff table to fit elements
    void resizeCoff();

    // resize the optional table to fit elements
    void resizeOptional();

    // size the section table to fit elements
    void resizeSection();

private:
    Ui::PEInfoWidget *ui_;
    PECoffModel coffModel_;
    PEOptionalModel optionalModel_;
    PESectionModel sectionModel_;
};

#endif // PEINFOWIDGET_H

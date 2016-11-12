#ifndef DLGHISTORY_H
#define DLGHISTORY_H

#include <QDialog>

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

namespace Ui {
class dlgHistory;
}

QT_END_NAMESPACE

class dlgHistory : public QDialog
{
    Q_OBJECT

public:
    explicit dlgHistory(QWidget *parent = 0);
    ~dlgHistory();
    void addItem(QString);

private slots:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);

private:
    Ui::dlgHistory *ui;
};

#endif // DLGHISTORY_H

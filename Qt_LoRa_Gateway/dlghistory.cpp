#include "dlghistory.h"
#include "ui_dlghistory.h"

#include <QSettings>
#include <QDateTime>

dlgHistory::dlgHistory(QWidget *parent) :
    QDialog(parent, (Qt::WindowTitleHint | Qt::WindowCloseButtonHint)),
    ui(new Ui::dlgHistory)
{
    ui->setupUi(this);

    connect(ui->historyList->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), ui->historyList, SLOT(scrollToBottom()));
}

dlgHistory::~dlgHistory()
{
    delete ui;
}

void dlgHistory::addItem(QString data)
{
    ui->historyList->addItem(QDateTime::currentDateTimeUtc().toString("MMM dd HH:mm:ss ") + data);
}

void dlgHistory::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    //specify registry key
    QSettings settings( QSettings::UserScope,"MoeTronix", "SerialTerminal");
    settings.beginGroup("HistoryWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    //bool ismin = settings.value("minstate", false).toBool();
    settings.endGroup();
}

void dlgHistory::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QSettings settings( QSettings::UserScope,"MoeTronix", "SerialTerminal");
    settings.beginGroup("HistoryWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("minstate",isMinimized());
    settings.endGroup();
}

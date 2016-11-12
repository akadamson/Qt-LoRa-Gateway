#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QFuture>
#include <QNetworkAccessManager>

#include "serialbase.h"
#include "dlghistory.h"
#include "dlgssdv.h"

#define A_THREAD

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void closeEvent(QCloseEvent *event);
	void OnTimer();
	void OnSerialDlg();
    void onHistoryDlg();
    void onSSDVDlg();
	void OnNewPortStatus(int portstatus);
	void OnNewRxData(quint8* pData, int numbytes);	//slot got new data from serial port
    void on_pushButton_released();
    void updateQueCounts();
    //void replyFinished(QNetworkReply *reply);

private:
	void readSettings();
	void writeSettings();
    void processLine(QString);
    void uploadSSDV(QStringList *, QNetworkAccessManager *);
    void uploadSentence(QString, QNetworkAccessManager *);
    void doBackground();
	//persistant variables saved in registry
	tSerialSettings m_PortSettings;

	//non-persistant local variables
    dlgHistory *historyDlg = NULL;
    dlgSSDV *SSDVDlg = NULL;
    bool _closing = false;
	CSerialBase* m_pSerialBase;
	QTimer *m_pTimer;
    QStringList *lstCommands;
    bool alreadyChecked = false;
    int outputQue_p = 0;
    int outputQueEnd_p = 0;
    int rxSentenceQueCount = 0;
    int ssdvQueCount = 0;

    Ui::MainWindow *ui;
    QMutex queMutex;
    QFuture<void> *f1;
};

#endif // MAINWINDOW_H

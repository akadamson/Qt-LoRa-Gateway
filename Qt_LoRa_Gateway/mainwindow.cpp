#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serialdialog.h"
#include "dlghistory.h"
#include "dlgssdv.h"

#include <QSettings>
#include <QtDebug>
#include <QTimer>
#include <QDateTime>
#include <QtNetwork>
#include <QUrl>
#include <QUrlQuery>
#include <QtConcurrent/QtConcurrentRun>
#include <QThread>

#define PROGRAM_TITLE_VERSION "LoRa Serial/Bluetooth Gateway 1.3qt"

//#define DBG(string) historyDlg->addItem(string)

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);	//Sets up Qt gui

	setWindowTitle(PROGRAM_TITLE_VERSION);

	//create serial port class
	m_pSerialBase = new CSerialBase();
    lstCommands = new QStringList();

    historyDlg = new dlgHistory(this);
    historyDlg->setFixedSize(591, 500);

    SSDVDlg = new dlgSSDV(this);
    SSDVDlg->setFixedSize(480, 420);
    //SSDVDlg->setModal(false);

    //m_pSerialBase->Flush();

	//read persistant settings from registry
	readSettings();

	//connect serial setup menu button
	connect(ui->actionSerialPort, SIGNAL( triggered() ), this, SLOT( OnSerialDlg() ) );

    connect(ui->actionHistory, SIGNAL(triggered()), this, SLOT(onHistoryDlg()));
    connect(ui->actionSSDV, SIGNAL(triggered()), this, SLOT(onSSDVDlg()));

	//setup a status timer
	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));
    m_pTimer->start(250);		//start up 250ms status timer

	//connect serial class signals
	connect( m_pSerialBase, SIGNAL( NewPortStatus(int)), this, SLOT( OnNewPortStatus(int) ) );
	connect( m_pSerialBase, SIGNAL( NewRxDataSig(quint8*, int)), this, SLOT( OnNewRxData(quint8*, int) ) );

    // connect results list
    connect(ui->resultsList->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), ui->resultsList, SLOT(scrollToBottom()));

	//open port with last settings
	m_pSerialBase->OpenPort(m_PortSettings);

    // create the concurrent threads

    // init the _closing flag for thread clean up
    _closing = false;

    // setup for running background threads
    f1 = new QFuture<void>;
    *f1 = QtConcurrent::run(this, &doBackground);
}

MainWindow::~MainWindow()
{
	qDebug()<<"Destructor";

	if(m_pSerialBase)
		delete m_pSerialBase;

    // delete command list object
    delete lstCommands;

	delete ui;

    delete historyDlg;

    if (SSDVDlg)
        delete SSDVDlg;
}

/////////////////////////////////////////////////////////////////////
// Called when program is closed to save persistant data
/////////////////////////////////////////////////////////////////////
void MainWindow::closeEvent(QCloseEvent *event)
{
	Q_UNUSED(event);
	qDebug()<<"closeEvent";
	m_pSerialBase->ClosePort();

    // clear the command list holding area
    lstCommands->clear();

    // save in reg the serial port settings
	writeSettings();

    // Threads should end on flag setting, wait for them to complete
    // before finally closing window
    _closing = true;
    f1->waitForFinished();
    delete f1;
}

/////////////////////////////////////////////////////////////////////
// Program persistant data save/recall methods
/////////////////////////////////////////////////////////////////////
void MainWindow::writeSettings()
{
	//specify registry key
	QSettings settings( QSettings::UserScope,"MoeTronix", "SerialTerminal");
	settings.beginGroup("MainWindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("minstate",isMinimized());
	settings.endGroup();

	//save serial port settings
	settings.beginGroup("Serial");
	settings.setValue("PortName",m_PortSettings.name);
	settings.setValue("BaudRate",m_PortSettings.baudRate);
	settings.setValue("DataBits",m_PortSettings.dataBits);
	settings.setValue("Parity",m_PortSettings.parity);
	settings.setValue("StopBits",m_PortSettings.stopBits);
	settings.setValue("FlowControl",m_PortSettings.flowControl);
	settings.endGroup();
}

void MainWindow::readSettings()
{
	//specify registry key
	QSettings settings( QSettings::UserScope,"MoeTronix", "SerialTerminal");
	settings.beginGroup("MainWindow");
	restoreGeometry(settings.value("geometry").toByteArray());
	bool ismin = settings.value("minstate", false).toBool();
	settings.endGroup();

	settings.beginGroup("Serial");
	m_PortSettings.name = settings.value("PortName","COM1").toString();
	m_PortSettings.baudRate = settings.value("BaudRate", 230400).toInt();
	m_PortSettings.dataBits = static_cast<QSerialPort::DataBits>(settings.value("DataBits", 8).toInt());
	m_PortSettings.parity = static_cast<QSerialPort::Parity>(settings.value("Parity", 0).toInt() );
	m_PortSettings.stopBits = static_cast<QSerialPort::StopBits>(settings.value("StopBits", 1).toInt() );
	m_PortSettings.flowControl = static_cast<QSerialPort::FlowControl>(settings.value("FlowControl", 0).toInt());
	settings.endGroup();

	if(ismin)
		showMinimized();
}

/////////////////////////////////////////////////////////////
/// Slot called to open serial setup dialog.
/////////////////////////////////////////////////////////////
void MainWindow::OnSerialDlg()
{
	CSerialDialog dlg(this);
	m_pSerialBase->ClosePort();
	dlg.m_PortSettings = m_PortSettings;
	dlg.InitDlg();
	if(QDialog::Accepted == dlg.exec() )
	{
		m_PortSettings = dlg.m_PortSettings;
	}
	m_pSerialBase->OpenPort(m_PortSettings);
}

void MainWindow::onHistoryDlg()
{
    historyDlg->show();
}

void MainWindow::onSSDVDlg()
{
    SSDVDlg->show();
}

/////////////////////////////////////////////////////////////
// Slot called when serial port status changes.
/////////////////////////////////////////////////////////////
void MainWindow::OnNewPortStatus( int portstatus )
{
	switch(portstatus)
	{
		case STATUS_PORTCLOSED:
			ui->label_Status->setText("Port Closed");
			break;
		case STATUS_PORTOPEN:
			ui->label_Status->setText("Port Open");
			break;
		case STATUS_PORTERROR:
            ui->label_Status->setText("Port Error");
			break;
	}
}

/////////////////////////////////////////////////////////////////////
// Status Timer event handler
// if port open and commands to gateway ready, write them
/////////////////////////////////////////////////////////////////////
void MainWindow::OnTimer()
{
	if( m_pSerialBase->IsPortOpen() )
	{
        if (!lstCommands->isEmpty())
        {
            m_pSerialBase->WriteData((quint8 *)lstCommands->first().toUtf8().data(), lstCommands->first().length());
            lstCommands->removeFirst();
        }
	}
}

/*
void MainWindow::replyFinished(QNetworkReply *reply)
{
    QByteArray bytes = reply->readAll();
    qDebug() << "Reply received" << bytes;
}
*/
// Upload any telemetry sentences to habitat
void MainWindow::uploadSentence(QString st, QNetworkAccessManager *manager)
{
#if 0
    QUrlQuery params;
#else
    QString json;
#endif

#ifndef A_THREAD
    QEventLoop eventLoop;
#endif

#ifndef A_THREAD
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
#endif

#if 0
    // start to build the request structure
    QNetworkRequest request(QUrl(QString("http://habitat.habhub.org/transition/payload_telemetry")));

    // add the content header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // build up the http post parameters
    params.addQueryItem("callsign", ui->callSign->text());
    params.addQueryItem("string", st + QChar(10));
    params.addQueryItem("string_type", "ascii");
    params.addQueryItem("metadata", "{}");
    params.addQueryItem("time_created", "");

//qDebug() << "Telemetry params" << params.query(QUrl::FullyEncoded).toUtf8();

    //QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    // do the actual post
    QNetworkReply *reply = manager->post(request, params.query(QUrl::FullyEncoded).toUtf8());
#else
    // start to build the request structure
    QNetworkRequest request(QUrl(QString("http://habitat.habhub.org/habitat")));

    // set the content header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray _tRaw;
    QString _raw = _tRaw.append(QString(st + QChar(10))).toBase64();

    //QString _raw = _tRaw.toBase64();
    _tRaw.clear();

//qDebug() << _raw;
    _tRaw.append(_raw);
    QByteArray _id = QCryptographicHash::hash(_tRaw, QCryptographicHash::Sha256);
//qDebug() << _id.toHex();

    json.clear();
    json.append("{");
    json.append("\"_id\": \""); json.append(_id.toHex()); json.append("\",");
    json.append("\"type\": \"payload_telemetry\",");

    json.append("\"data\": { ");
        json.append("\"_raw\": \""); json.append(_raw); json.append("\"},");
    json.append("\"receivers\": { ");
        json.append("\"" + ui->callSign->text() + "\": { ");
            json.append("\"time_created\": \""); json.append(QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ssZ"));json.append("\",");
            json.append("\"time_uploaded\": \""); json.append(QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ssZ"));json.append("\" }");
        json.append("}");
    json.append("}");

//qDebug() << "Telemetry params" << json.toUtf8();

    //QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    // do the actual post
    QNetworkReply *reply = manager->post(request, json.toUtf8());
#endif

#ifndef A_THREAD
    eventLoop.exec();
#else
    while (!reply->isFinished())
    {
        qApp->processEvents();
    }
#endif
#if 1
    // check the reply
    if (reply->error() == QNetworkReply::NoError)
    {
qDebug() << "Success posting telemetry" << "[" << reply->readAll() << "]";
    }
    else
    {
qDebug() << "Failure posting telmetry" << reply->errorString() << "[" << reply->readAll() << "]";
       historyDlg->addItem("[ERROR] : Failure posting telemetry");
    }
#endif
    reply->deleteLater();
}

// update the habitat telemetry
void MainWindow::uploadSSDV(QStringList *packets, QNetworkAccessManager *manager)
{
    QString json;
#ifndef A_THREAD
    QEventLoop eventLoop;
#endif

#ifndef A_THREAD
    QObject::connect(&manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
#endif

    QNetworkRequest request(QUrl(QString("http://ssdv.habhub.org/api/v0/packets")));

    // set the content header
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // create json string
    json.clear();
    json.append("{");
    json.append("\"type\": \"packets\",");
    json.append("\"packets\": [");

    for (QStringList::iterator i = packets->begin(); i != packets->end(); i++)
    {
        if (i > packets->begin())
            json.append(",");

        json.append("{");
        json.append("\"type\": \"packet\",");
        json.append("\"packet\":\"55"); json.append(*i); json.append("\",");
        json.append("\"encoding\": \"hex\",");
        json.append("\"received\": \""); json.append(QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ssZ")); json.append("\",");
        json.append("\"receiver\": \"" + ui->callSign->text() + "\"");
        json.append("}");
    }

    json.append("]}");

//qDebug() << "SSDV json" << QString::fromUtf8(json.toUtf8(), json.size());

    // do the actual post of the json
    QNetworkReply *reply = manager->post(request, json.toUtf8());

#ifndef A_THREAD
    eventLoop.exec();
#else
    while (!reply->isFinished())
    {
        qApp->processEvents();
    }
#endif
#if 1
    // check the error
    if (reply->error() == QNetworkReply::NoError)
    {
qDebug() << "Success posting SSDV" << "[" << reply->readAll() << "]";
    }
    else
    {
qDebug() << "Failure posting SSDV" << reply->errorString() << "[" << reply->readAll() << "]";
       historyDlg->addItem("[ERROR] : Failure posting SSDV");
    }
#endif
    reply->deleteLater();
}

// process incoming serial data
void MainWindow::processLine(QString line)
{
    // convert the command to UC
    QString Command(line.left(line.indexOf("=")).toUpper());

    if (!QString::compare(Command, "CURRENTRSSI"))
    {
        ui->currentRSSI->setText(line.mid(line.indexOf("=") + 1) + "dBm");
    }
    else if (!QString::compare(Command, "ERROR"))
    {
        historyDlg->addItem(line.mid(line.indexOf("=") + 1));
    }
    else if (!QString::compare(Command, "MESSAGE"))
    {
//qDebug() << line.mid(line.indexOf("=") + 1);
        if (line.mid(line.indexOf("=") + 1).length() > 5)
        {
            ui->resultsList->addItem(line.mid(line.indexOf("=") + 1));
            ui->rxSentenceCount->setText(QString::number(ui->rxSentenceCount->text().toInt() + 1));

            if (ui->cbUploadToHab->isChecked())
            {
                if (!alreadyChecked)
                {
                    alreadyChecked = true;
                    outputQue_p = ui->rxSentenceCount->text().toInt() + ui->ssdvCount->text().toInt() - 1;
                }

                outputQueEnd_p = ui->rxSentenceCount->text().toInt() + ui->ssdvCount->text().toInt();
                rxSentenceQueCount++;
                updateQueCounts();
            }
            else
            {
                alreadyChecked = false;
            }
        }
        else
        {
            historyDlg->addItem("[INFO] : MESSAGE too short");
        }
    }
    else if (!QString::compare(Command, "HEX"))
    {
        if (!QString::compare(line.mid(line.indexOf("=") + 1).left(2), "66") || !QString::compare(line.mid(line.indexOf("=") + 1).left(2), "E6"))
        {
            if (line.mid(line.indexOf("=") + 1).length() > 5)
            {
                ui->resultsList->addItem(line.mid(line.indexOf("=") + 1));
                ui->ssdvCount->setText(QString::number(ui->ssdvCount->text().toInt() + 1));

                if (ui->cbUploadToHab->isChecked())
                {
                    if (!alreadyChecked)
                    {
                        alreadyChecked = true;
                        outputQue_p = ui->ssdvCount->text().toInt() + ui->rxSentenceCount->text().toInt() - 1;
                    }

                    outputQueEnd_p = ui->ssdvCount->text().toInt() + ui->rxSentenceCount->text().toInt();
                    ssdvQueCount++;
                    updateQueCounts();
                }
                else
                {
                    alreadyChecked = false;
                }

                SSDVDlg->ssdvDecode("55" + line.mid(line.indexOf("=") + 1).toUtf8());
            }
            else
            {
                historyDlg->addItem("[INFO] : HEX too short");
            }
        }
    }
    else if (!QString::compare(Command, "FREQERR"))
    {
        ui->freqError->setText(line.mid(line.indexOf("=") + 1) + " kHz");
    }
    else if (!QString::compare(Command, "PACKETRSSI"))
    {
        ui->packetRSSI->setText(line.mid(line.indexOf("=") + 1) + "dBm");
    }
    else if (!QString::compare(Command, "PACKETSNR"))
    {
        ui->packetSNR->setText(line.mid(line.indexOf("=") + 1));
    }
    Command.clear();
}

// update the SSDV Queue counter
void MainWindow::updateQueCounts(void)
{
    queMutex.lock();
    ui->telemQue->setText(QString::number(rxSentenceQueCount));
    ui->ssdvQue->setText(QString::number(ssdvQueCount));
    queMutex.unlock();
}

/////////////////////////////////////////////////////////////////////
/// Called when new RX data arrives from serial port
/////////////////////////////////////////////////////////////////////
void MainWindow::OnNewRxData(quint8* pData, int numbytes)
{
     static QString Buffer = "";
     QChar character;

//qDebug()<< numbytes << pData[0];

	//put received bytes in text box
    for(int i = 0; i < numbytes; i++)
    {
        character = pData[i];

       if ((character == 13) || (character == 10))
        {
            if (Buffer.length() > 0)
            {
//qDebug() << Buffer;
                processLine(Buffer);
                Buffer.clear();
            }
        }
        else
        {
            if (Buffer.length() < 1000)
            {
                Buffer.append(character);
//qDebug() << Buffer;
            }
        }
    }
}

// build the lines of commands to support the send button
void MainWindow::on_pushButton_released()
{
    // testing lines uncomment on, and change the #if 1 ot #if 0, pressing the set button will send which ever is uncommented
    //processLine("MESSAGE=$$HIRF-SSDV,36,19:53:21,34.08461,-83.94766,00369,0,0,9,29.7*4989");
    //processLine("HEX=66C3635345010000140F00000000C1A314B45050DC518A5A280131498A7518A006E28C53B1462801B8A314EC518A03FA1B8A314EC518A006E29314EC518A03FA1B8A314EC526280131498A7628C500371462968A004A4A7525002518A5A28109498A5A280FE84A29692800A28A2800A4A5A2800A28A281852D252D0028A70A6D2834012034F06A2069E0D0018A314EC526280FE84C52629D8A3147F5FD7F403714629D8A3147F403714B8A5C518A3FAFE804C518A5C518A006E28C53B1462801B8A314B8A31407F43714629D8A4C500371453B1498A004A4A7525054F7F05313C08B6EAF95263EEB5FE33BDD1ACE1AEC1B96E93C6A424E2F40A61A386A7352");
    //processLine("HEX=E6C3635345010000140F00000000C1A314B45050DC518A5A280131498A7518A006E28C53B1462801B8A314EC518A03FA1B8A314EC518A006E29314EC518A03FA1B8A314EC526280131498A7628C500371462968A004A4A7525002518A5A28109498A5A280FE84A29692800A28A2800A4A5A2800A28A281852D252D0028A70A6D2834012034F06A2069E0D0018A314EC526280FE84C52629D8A3147F5FD7F403714629D8A3147F403714B8A5C518A3FAFE804C518A5C518A006E28C53B1462801B8A314B8A31407F43714629D8A4C500371453B1498A004A4A7525054F7F05313C08B6EAF95263EEB5FE33BDD1ACE1AEC1B96E93C6A424E2F40A61A386A7352");

#if 1
    lstCommands->clear();
    lstCommands->append("~F" + ui->frequency->text() + QChar(13));
    lstCommands->append("~M" + QString::number(ui->comboBox_3->currentIndex()) + QChar(13));
    if (ui->comboBox_4->currentIndex() > 0)
    {
        lstCommands->append("~E" + QString::number(ui->comboBox_4->currentIndex() + 4) + QChar(13));
    }
    if (ui->comboBox_5->currentIndex() > 0)
    {
        lstCommands->append("~S" + QString::number(ui->comboBox_5->currentIndex() + 5) + QChar(13));
    }
    if (ui->comboBox->currentIndex() > 0)
    {
        lstCommands->append("~B" + ui->comboBox->currentText() + QChar(13));
    }
    if (ui->comboBox_2->currentIndex() > 0)
    {
        lstCommands->append("~I" + QString::number(ui->comboBox_2->currentIndex() - 1) + QChar(13));
    }
#endif
}

// main thread to process inet upload requests
void MainWindow::doBackground()
{
    QNetworkAccessManager manager;
    QStringList sList;
    QString tString;
    //int i = 0;

    // start the thread and leave it running the entire time the app is running
    do
    {
        // if there are Sentences to be uploaded
        if (ui->telemQue->text().toInt() && !QString::compare(ui->resultsList->item(outputQue_p)->text().left(1), "$"))
        {
//qDebug() << ui->resultsList->count() <<  outputQue_p << outputQueEnd_p;
//qDebug() << ui->resultsList->item(outputQue_p)->text();

            // upload the Sentence
            uploadSentence(ui->resultsList->item(outputQue_p)->text(), &manager);
            // update the pointers
            outputQue_p++;
            rxSentenceQueCount--;
            // update the ui counters
            updateQueCounts();
        }
        else if (ui->ssdvQue->text().toInt() && (outputQue_p != outputQueEnd_p))  // if there are SSDV frames to be uploaded
        {
//qDebug() << ui->resultsList->count() << outputQue_p << outputQueEnd_p;
//qDebug() << ui->resultsList->item(outputQue_p)->text();

            // get a local copy of string
            tString.append(ui->resultsList->item(outputQue_p)->text());
            // see if the first char is an E, if so replace with a 6
            if (!QString::compare(tString.left(2), "E6"))
            {
                tString.replace(0, 1, "6");
//qDebug() << "Convert from E6 to 66";
            }

            // put it in the stringlist
            sList.append(tString);
            // clear the temp string for use next time
            tString.clear();

            //if (++i >= 3)
            //{
                // upload the SSDV
                uploadSSDV(&sList, &manager);
                //ssdvQueCount -= i;
                // clear the stringlist
                sList.clear();
                //i = 0;
            //}

            // update the pointers
            outputQue_p++;
            ssdvQueCount--;
            // update the ui counters
            updateQueCounts();
        }
        else
        {
#if 0
            // just in case we have leftover SSDV frames, upload them
            if ((i > 0) && (outputQue_p == outputQueEnd_p))
            {
                // upload the SSDV
                uploadSSDV(&sList, &manager);
                ssdvQueCount -= i;
                // clear the stringlist
                sList.clear();
                i = 0;

                // update the ui counters
                updateQueCounts();
            }
            else
            {
#endif
                // if we didn't do anything sleep so we aren't in a tight do/while loop
                this->thread()->usleep(100);
#if 0
            }
#endif
        }
        //this->thread()->msleep(5000); // debugging only to slow down the thread to check for queue function
    }
    while (!_closing); // when the app finally closes, we'll get a flag and can end the thread.
}

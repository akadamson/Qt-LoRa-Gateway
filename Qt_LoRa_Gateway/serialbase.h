#ifndef SERIALBASE_H
#define SERIALBASE_H

#include <QIODevice>
#include <QSerialPort>
#include "threadwrapper.h"

typedef struct
{
	QString name;
	qint32 baudRate;
	QSerialPort::DataBits dataBits;
	QSerialPort::Parity parity;
	QSerialPort::StopBits stopBits;
	QSerialPort::FlowControl flowControl;
}tSerialSettings;

//#define USE_VIRTUAL_DECODE_FUNCT 1	//uncomment to use virtual decoder function

//#define RXQ_SIZE 32
#define RXBUF_SIZE 16384
#define TXQ_SIZE 1024
#define TXBUF_SIZE 16384
//#define RXQ_SIZE 16384
#define RXQ_SIZE 1024

#define	STATUS_PORTCLOSED 0
#define	STATUS_PORTOPEN 1
#define	STATUS_PORTCONNECTED 2
#define	STATUS_PORTERROR 3

class CSerialBase : public CThreadWrapper
{
	Q_OBJECT
public:
	CSerialBase();
	virtual ~CSerialBase();
	//Exposed functions to application
	void OpenPort(tSerialSettings Settings){SetParameters(Settings); emit StartSig();}
	void ClosePort(){emit StopSig();}
	void SetParameters(tSerialSettings Settings){m_PortSettings = Settings;}	//setup port name and parameters
	void WriteData(quint8* pData, int numbytes );	//call to write data to port
	bool IsPortOpen(){ if(!m_pSerialPort) return false; else if( m_pSerialPort->isOpen() ) return true; else return false; }
#ifdef USE_VIRTUAL_DECODE_FUNCT
	//Derive a custom class to implement this virtual function which is called by worker thread
	virtual void ProcessRxData( quint8* pData, int numbytes ){Q_UNUSED(numbytes);Q_UNUSED(pData);}
#endif
    void Flush();

protected:
	void Start(){emit StartSig();}	//starts serial port thread
	void Stop(){emit StopSig();}	//stops serial port thread
	tSerialSettings m_PortSettings;
	QSerialPort* m_pSerialPort;

signals:
	void NewPortStatus(int status);	//can be connected by app to process serial port status changes
	//The following are internal signals
	void StartSig();	//starts serial port
	void StopSig();		//stops serial port
	void WriteDataSig(quint8* pData, int numbytes);	//signal write to serial port
	void NewRxDataSig(quint8* pData, int numbytes);	//signal got new data from serial port

private slots:
	void StartSlot();	//starts serial port
	void StopSlot();	//stops serial port
	void ReadDataSlot();
	void WriteDataSlot(quint8* pData, int numbytes);
	void OnError(QSerialPort::SerialPortError);
	void ThreadInit();	//override function is called by new thread when started
	void ThreadExit();	//override function is called by thread before exiting

protected:

private:
	quint8 m_TxBuf[TXQ_SIZE][TXBUF_SIZE];
	qint64 m_TxQPos;
	quint8 m_RxBuf[RXQ_SIZE][RXBUF_SIZE];
	qint64 m_RxQPos;
};

#endif // SERIALBASE_H

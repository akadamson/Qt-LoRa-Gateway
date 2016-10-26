/////////////////////////////////////////////////////////////////////
// serialbase.cpp: implementation of the CSerialBase class.
//This class can be derived from to implement custom protocols etc.
//It uses a worker thread to process rx and tx data.
//
// History:
//	2013-12-09  Initial creation MSW
/////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------*/
/*------------------------> I N C L U D E S <--------------------------------*/
/*---------------------------------------------------------------------------*/
#include "serialbase.h"
#include <QDebug>

/*---------------------------------------------------------------------------*/
/*--------------------> L O C A L   D E F I N E S <--------------------------*/
/*---------------------------------------------------------------------------*/

/////////////////////////////////////////////////////////////////////
//   constructor/destructor
/////////////////////////////////////////////////////////////////////
CSerialBase::CSerialBase()
{
	m_pSerialPort = NULL;
	m_TxQPos = 0;
	m_RxQPos = 0;
	qDebug()<<"GUI thread"<<this->thread()->currentThread();
}

CSerialBase::~CSerialBase()
{
	qDebug()<<"Cleanup";
	CleanupThread();	//tell thread to cleanup after itself by calling ThreadExit()
}

//////////////////////////////////////////////////////////////////////////
//Called in worker thread when it starts to initialize things
//in the thread context
//////////////////////////////////////////////////////////////////////////
void CSerialBase::ThreadInit()	//overrided funciton is called by new thread when started
{
	m_pSerialPort = new QSerialPort();
	//create connections to span thread context to GUI context
	connect(this,SIGNAL( StartSig()), this, SLOT(StartSlot()) );
	connect(this,SIGNAL( StopSig()), this, SLOT(StopSlot()) );
	connect(m_pSerialPort, SIGNAL(readyRead()), this, SLOT(ReadDataSlot()));
	connect(m_pSerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(OnError(QSerialPort::SerialPortError)));
	connect(this, SIGNAL(WriteDataSig(quint8*, int) ), this, SLOT( WriteDataSlot(quint8*, int) ) );
	this->thread()->setPriority(QThread::HighestPriority);
qDebug()<<"CSerialBase Thread "<<this->thread()->currentThread();
}

/////////////////////////////////////////////////////////////////////
// Slot Called by this worker thread when error occurs
/////////////////////////////////////////////////////////////////////
void CSerialBase::OnError(QSerialPort::SerialPortError error)
{
	if(error != QSerialPort::NoError)
	{
		emit NewPortStatus(STATUS_PORTERROR);
		qDebug()<<"Serial Error = "<<error;
	}
}

/////////////////////////////////////////////////////////////////////
// Called by this worker thread to cleanup after itself
/////////////////////////////////////////////////////////////////////
void CSerialBase::ThreadExit()
{
qDebug()<<"CSerialBase Thread exit";
	//dsconnect any signals that may fire before exiting
	disconnect(m_pSerialPort, SIGNAL(readyRead()), this, SLOT(ReadDataSlot()));
	disconnect(m_pSerialPort, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(OnError(QSerialPort::SerialPortError)));
	if(m_pSerialPort)
		delete m_pSerialPort;
}

/////////////////////////////////////////////////////////////////////
// Starts up serial port in worker thread
/////////////////////////////////////////////////////////////////////
void CSerialBase::StartSlot()
{
	m_RxQPos = 0;
	m_TxQPos = 0;
	m_pSerialPort->setPortName(m_PortSettings.name);
qDebug()<<"COM Port="<<m_PortSettings.name;
	if( !m_pSerialPort->open(QIODevice::ReadWrite) )
	{
		emit NewPortStatus(STATUS_PORTERROR);
qDebug()<<"Serial Port Open Error";
	}
	else
	{
		emit NewPortStatus(STATUS_PORTOPEN);
		//must set port parameters AFTER port open!
		m_pSerialPort->setBaudRate(m_PortSettings.baudRate);
		m_pSerialPort->setDataBits(m_PortSettings.dataBits);
		m_pSerialPort->setParity(m_PortSettings.parity);
		m_pSerialPort->setFlowControl(m_PortSettings.flowControl);
qDebug()<<"Serial Port Open OK";
	}
}

//////////////////////////////////////////////////////////////////////////
// Called in thread to stop the serial port in worker thread
//////////////////////////////////////////////////////////////////////////
void CSerialBase::StopSlot()
{
	if( m_pSerialPort->isOpen() )
	{
		m_pSerialPort->close();
		emit NewPortStatus(STATUS_PORTCLOSED);
qDebug()<<"Serial Port Closed";
	}
}

/////////////////////////////////////////////////////////////////////
//  Slot called by worker thread when data is available
/////////////////////////////////////////////////////////////////////
void CSerialBase::ReadDataSlot()
{
qint64 n;
quint8 pBuf[RXQ_SIZE];
    n = m_pSerialPort->read( (char*)pBuf, RXQ_SIZE);
	if(n>0)
	{
#ifdef USE_VIRTUAL_DECODE_FUNCT
		ProcessRxData( pBuf, (int)n );	//Call derived virtual function to process
#else
		for(int i=0; i<n; i++)
			m_RxBuf[m_RxQPos][i] = pBuf[i];
		emit NewRxDataSig(m_RxBuf[m_RxQPos], n);	//signal new rx data ready
		m_RxQPos++;	//inc buffer queue ptr
		if(m_RxQPos>=RXQ_SIZE)
			m_RxQPos = 0;
#endif
	}
	if(n<0)
	{
		emit NewPortStatus(STATUS_PORTERROR);
		qDebug()<<"Read Eror";
	}
//qDebug()<<m_RxQPos << "Rx="<<n;
}

void CSerialBase::Flush()
{
    m_pSerialPort->flush();
    m_RxQPos = 0;
}

/////////////////////////////////////////////////////////////////////
// called by worker thread to write buffer to serial port
/////////////////////////////////////////////////////////////////////
void CSerialBase::WriteDataSlot(quint8* pData, int numbytes)
{
	qint64 sent = m_pSerialPort->write((const char*)pData, (qint64)numbytes );
	if(sent != (qint64)numbytes)
	{
		emit NewPortStatus(STATUS_PORTERROR);
		qDebug()<<"Tx write Error";
	}
//qDebug()<<"Tx="<<sent<< pData[2] << pData[3];
}

////////////////////////////////////////////////////////////////
//Called by application to write output pData to serial port.
//Just writes to buffer queue then signals thread
////////////////////////////////////////////////////////////////
void CSerialBase::WriteData(quint8* pData, int numbytes )
{
	if( (0==numbytes) || !pData || !m_pSerialPort->isOpen() )
		return;
	for(int i=0; i<numbytes; i++)
		m_TxBuf[m_TxQPos][i] = pData[i];
	emit WriteDataSig(m_TxBuf[m_TxQPos], numbytes);	//wake up thread to process tx data
	m_TxQPos++;	//inc buffer queue ptr
	if(m_TxQPos>=TXQ_SIZE)
		m_TxQPos = 0;
	return;
}

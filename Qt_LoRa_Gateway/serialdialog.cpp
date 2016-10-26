#include "serialdialog.h"
#include "ui_serialdialog.h"
#include <QSerialPortInfo>
#include <QIntValidator>
#include <QLineEdit>

QT_USE_NAMESPACE

CSerialDialog::CSerialDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CSerialDialog)
{
	ui->setupUi(this);

	intValidator = new QIntValidator(0, 4000000, this);

	ui->baudRateBox->setInsertPolicy(QComboBox::NoInsert);

	connect(ui->serialPortNameListBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(showPortInfo(int)));
	connect(ui->baudRateBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(checkCustomBaudRatePolicy(int)));

	fillPortsParameters();
	fillPortsInfo();

}

CSerialDialog::~CSerialDialog()
{
	delete ui;
}

//Fill in initial data
void CSerialDialog::InitDlg()
{
	//get index of portname if exists
	int idx = ui->serialPortNameListBox->findText(m_PortSettings.name);
	if(idx>=0)
	{
		ui->serialPortNameListBox->setCurrentIndex(idx);
		showPortInfo(idx);
	}
	ui->baudRateBox->setCurrentIndex( ui->baudRateBox->findData(m_PortSettings.baudRate) );
	ui->dataBitsBox->setCurrentIndex( ui->dataBitsBox->findData(m_PortSettings.dataBits) );
	ui->parityBox->setCurrentIndex( ui->parityBox->findData(m_PortSettings.parity) );
	ui->stopBitsBox->setCurrentIndex( ui->stopBitsBox->findData(m_PortSettings.stopBits) );
	ui->flowControlBox->setCurrentIndex( ui->flowControlBox->findData(m_PortSettings.flowControl) );

}

void CSerialDialog::showPortInfo(int idx)
{
	if (idx != -1)
	{
		QStringList list = ui->serialPortNameListBox->itemData(idx).toStringList();
		ui->descriptionLabel->setText(tr("Descr: %1").arg(list.at(1)));
		ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(list.at(2)));
		ui->locationLabel->setText(tr("Location: %1").arg(list.at(3)));
		ui->vidLabel->setText(tr("Vendor Identifier: %1").arg(list.at(4)));
		ui->pidLabel->setText(tr("Product Identifier: %1").arg(list.at(5)));
	}
}

void CSerialDialog::checkCustomBaudRatePolicy(int idx)
{
	bool isCustomBaudRate = !ui->baudRateBox->itemData(idx).isValid();
	ui->baudRateBox->setEditable(isCustomBaudRate);
	if (isCustomBaudRate)
	{
		ui->baudRateBox->clearEditText();
		QLineEdit *edit = ui->baudRateBox->lineEdit();
		edit->setValidator(intValidator);
	}
}

void CSerialDialog::fillPortsParameters()
{
	// fill baud rate (is not the entire list of available values,
	// desired values??, add yours independently)
	ui->baudRateBox->addItem(QLatin1String("9600"), QSerialPort::Baud9600);
	ui->baudRateBox->addItem(QLatin1String("19200"), QSerialPort::Baud19200);
	ui->baudRateBox->addItem(QLatin1String("38400"), QSerialPort::Baud38400);
	ui->baudRateBox->addItem(QLatin1String("57600"), QSerialPort::Baud57600);
	ui->baudRateBox->addItem(QLatin1String("115200"), QSerialPort::Baud115200);
	ui->baudRateBox->addItem(QLatin1String("230400"), 230400);
	ui->baudRateBox->addItem(QLatin1String("Custom"));

	// fill data bits
	ui->dataBitsBox->addItem(QLatin1String("5"), QSerialPort::Data5);
	ui->dataBitsBox->addItem(QLatin1String("6"), QSerialPort::Data6);
	ui->dataBitsBox->addItem(QLatin1String("7"), QSerialPort::Data7);
	ui->dataBitsBox->addItem(QLatin1String("8"), QSerialPort::Data8);
	ui->dataBitsBox->setCurrentIndex(0);

	// fill parity
	ui->parityBox->addItem(QLatin1String("None"), QSerialPort::NoParity);
	ui->parityBox->addItem(QLatin1String("Even"), QSerialPort::EvenParity);
	ui->parityBox->addItem(QLatin1String("Odd"), QSerialPort::OddParity);
	ui->parityBox->addItem(QLatin1String("Mark"), QSerialPort::MarkParity);
	ui->parityBox->addItem(QLatin1String("Space"), QSerialPort::SpaceParity);

	// fill stop bits
	ui->stopBitsBox->addItem(QLatin1String("1"), QSerialPort::OneStop);
	ui->stopBitsBox->addItem(QLatin1String("2"), QSerialPort::TwoStop);

	// fill flow control
	ui->flowControlBox->addItem(QLatin1String("None"), QSerialPort::NoFlowControl);
	ui->flowControlBox->addItem(QLatin1String("RTS/CTS"), QSerialPort::HardwareControl);
	ui->flowControlBox->addItem(QLatin1String("XON/XOFF"), QSerialPort::SoftwareControl);
}

void CSerialDialog::fillPortsInfo()
{
	ui->serialPortNameListBox->clear();
	foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
	{
		QStringList list;
		list << info.portName()
			<< info.description()
			<< info.manufacturer()
			<< info.systemLocation()
			<< (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString())
			<< (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : QString());
		ui->serialPortNameListBox->addItem(list.first(), list);
	}
}

void CSerialDialog::accept()
{	//OK was pressed so get all data from edit controls
	m_PortSettings.name = ui->serialPortNameListBox->currentText();

	// Baud Rate
	if (ui->baudRateBox->currentIndex() == 4)
	{
		// custom baud rate
		m_PortSettings.baudRate = ui->baudRateBox->currentText().toInt();
	}
	else
	{
		// standard baud rate
		m_PortSettings.baudRate = static_cast<QSerialPort::BaudRate>(
			ui->baudRateBox->itemData(ui->baudRateBox->currentIndex()).toInt());
	}

	// Data bits
	m_PortSettings.dataBits = static_cast<QSerialPort::DataBits>(
		ui->dataBitsBox->itemData(ui->dataBitsBox->currentIndex()).toInt());

	// Parity
	m_PortSettings.parity = static_cast<QSerialPort::Parity>(
		ui->parityBox->itemData(ui->parityBox->currentIndex()).toInt());

	// Stop bits
	m_PortSettings.stopBits = static_cast<QSerialPort::StopBits>(
		ui->stopBitsBox->itemData(ui->stopBitsBox->currentIndex()).toInt());

	// Flow control
	m_PortSettings.flowControl = static_cast<QSerialPort::FlowControl>(
		ui->flowControlBox->itemData(ui->flowControlBox->currentIndex()).toInt());

	QDialog::accept();	//call base class
}

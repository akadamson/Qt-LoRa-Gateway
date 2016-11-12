#include "stdlib.h"
#include "dlgssdv.h"
#include "dlghistory.h"
#include "ui_dlgssdv.h"

#include <QSettings>
#include <QtDebug>

dlgSSDV::dlgSSDV(QWidget *parent) :
    QDialog(parent, (Qt::WindowTitleHint | Qt::WindowCloseButtonHint)),
    ui(new Ui::dlgSSDV)
{
    ui->setupUi(this);

    // create the QT image datastore
    image = new QImage();

    // create the max JPEG storage space
    jpeg = (uint8_t *)malloc(jpeg_length);
    // create QT graphics scene
    scene = new QGraphicsScene(this);
    // create the pixmap that will be displayed in the scene
    jpg_item = new QGraphicsPixmapItem();
    // add the pixmap to the scene
    scene->addItem(jpg_item);
    // tell the UI element that is has a scene to display
    ui->SSDVView->setScene(scene);
}

dlgSSDV::~dlgSSDV()
{
    // house cleaning
    delete scene;
    delete ui;
    if (jpeg)
        free(jpeg);
    delete image;
}

void dlgSSDV::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    //specify registry key
    QSettings settings( QSettings::UserScope,"MoeTronix", "SerialTerminal");
    settings.beginGroup("SSDVWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    //bool ismin = settings.value("minstate", false).toBool();
    settings.endGroup();
}

void dlgSSDV::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    QSettings settings( QSettings::UserScope,"MoeTronix", "SerialTerminal");
    settings.beginGroup("SSDVWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("minstate",isMinimized());
    settings.endGroup();
}

void dlgSSDV::ssdvDecode(QByteArray st)
{
    int errors, r;
    ssdv_packet_info_t p;
    ssdv_t tssdv;
    char s[16];

    // make sure that the incoming packet is SSDV sized
    if (QByteArray::fromHex(st).size() == SSDV_PKT_SIZE)
    {
        // convert incoming *string of HEX ASCII* to binary hex, store it for use locally
        memcpy(pkt, QByteArray::fromHex(st), SSDV_PKT_SIZE);

        // is it a valid packet
        if (ssdv_dec_is_packet(pkt, &errors))
            return;
        image_errors += errors;

        // decode the header
        ssdv_dec_header(&p, pkt);

        // belongs to new image?
        if (QString::compare(ui->lblCallsign->text(), QString(p.callsign_s)) ||
            ui->lblID->text().toInt() != p.image_id ||
            QString::compare(ui->lblSize->text(), QString::number(p.width) + "x" + QString::number(p.height)))
        {
            // init counters
            image_errors = 0;
            image_rec_packets = 0;
            image_lost_packets = 0;

            // init SSDV structure and storage
            ssdv_dec_init(&ssdv);
            memset(jpeg, 0, jpeg_length);
            // tell the SSDV decoder that is has storage
            ssdv_dec_set_buffer(&ssdv, jpeg, jpeg_length);
        }
        //else
        //{
            // same image so just increment the packet counter
            image_rec_packets++;
        //}

        // add incoming to SSDV
        r = ssdv_dec_feed(&ssdv, pkt);
        if (r == SSDV_ERROR)
        {
            //historyDlg->addItem("[ERROR] : Error on SSDV decode");
qDebug() << "Error on SSDV decode"            ;
            image_lost_packets++;
        }

        uint8_t *jpeg_out;
        size_t length = 0;

        // because we don't want to have to rebuild the whole image each time, we copy the
        // ssdv structure to a temp structure that is used to get an in process JPEG
        memcpy(&tssdv, &ssdv, sizeof(ssdv_t));
        ssdv_dec_get_jpeg(&tssdv, &jpeg_out, &length);

        // load data to QT image storage
        image->loadFromData(jpeg_out, length, "JPEG");
        // build the pixmap for display
        jpg_item->setPixmap(QPixmap::fromImage(image->scaled(320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        // update header elements in the UI
        ui->lblCallsign->setText(QString(p.callsign_s));
        snprintf(s, 16, "%d", p.image_id);
        ui->lblID->setText(QString(s));
        snprintf(s, 16, "%ix%i", p.width, p.height);
        ui->lblSize->setText(QString(s));
        ui->pbSSDV->setMaximum(p.mcu_count);
        ui->pbSSDV->setValue(p.mcu_id);
    }
    else
    {
        //historyDlg->addItem("[ERROR] : Lost SSDV packet");
qDebug() << "Lost SSDV packet";
        image_lost_packets++;
    }

    // update counter/non header elements in the UI
    snprintf(s, 16, "%d", image_rec_packets);
    ui->lblReceived->setText(QString(s));
    snprintf(s, 16, "%d", image_lost_packets);
    ui->lblLost->setText(QString(s));
    snprintf(s, 16, "%d byte%s", image_errors, (image_errors == 1 ? "" : "s"));
    ui->lblFixes->setText(QString(s));
}

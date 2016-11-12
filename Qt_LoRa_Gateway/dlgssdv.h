#ifndef DLGSSDV_H
#define DLGSSDV_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>

#include "ssdv.h"

QT_USE_NAMESPACE

QT_BEGIN_NAMESPACE

namespace Ui {
class dlgSSDV;
}

QT_END_NAMESPACE

class dlgSSDV : public QDialog
{
    Q_OBJECT

public:
    explicit dlgSSDV(QWidget *parent = 0);
    ~dlgSSDV();
    void ssdvDecode(QByteArray);

private slots:
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);

private:
    Ui::dlgSSDV *ui;
    ssdv_t ssdv;
    QGraphicsScene *scene;
    QImage *image;
    QGraphicsPixmapItem *jpg_item = NULL;

    uint8_t pkt[SSDV_PKT_SIZE], *jpeg;
    size_t jpeg_length = 1024 * 1024 * 4;
    int image_rec_packets = 0;
    int image_lost_packets = 0;
    int image_errors = 0;
};

#endif // DLGSSDV_H

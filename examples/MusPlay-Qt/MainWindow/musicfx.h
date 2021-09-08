#ifndef MUSICFX_H
#define MUSICFX_H

#include <QDialog>

namespace Ui {
class MusicFX;
}

class MusicFX : public QDialog
{
    Q_OBJECT

public:
    explicit MusicFX(QWidget *parent = nullptr);
    ~MusicFX();

protected:
    void changeEvent(QEvent *e);

public slots:
    void resetAll();

private slots:
    void on_resetPanning_clicked();
    void on_stereoPanRight_sliderMoved(int arg1);
    void on_stereoPanLeft_sliderMoved(int arg1);
    void on_flipStereo_clicked(bool checked);
    void on_resetPosition_clicked();
    void on_distance_sliderMoved(int arg1);
    void on_angle_sliderMoved(int arg1);

    void updatePositionEffect();
    void updatePanningEffect();
    void updateChannelsFlip();

private:
    Ui::MusicFX *ui;

    bool       m_sendPanning = false;
    int16_t    m_angle = 0;
    uint8_t    m_distance = 0;
    uint8_t    m_panLeft = 255;
    uint8_t    m_panRight = 255;
    uint8_t    m_channelFlip = 0;
};

#endif // MUSICFX_H

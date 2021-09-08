#include "musicfx.h"
#include "ui_musicfx.h"
#include "../Player/mus_player.h"


MusicFX::MusicFX(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MusicFX)
{
    ui->setupUi(this);
}

MusicFX::~MusicFX()
{
    delete ui;
}

void MusicFX::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MusicFX::resetAll()
{
    m_angle = 0;
    m_distance = 0;
    m_panLeft = 255;
    m_panRight = 255;
    m_channelFlip = 0;

    ui->stereoPanLeft->blockSignals(true);
    ui->stereoPanLeft->setValue(255);
    ui->stereoPanLeft->blockSignals(false);
    ui->stereoPanRight->blockSignals(true);
    ui->stereoPanRight->setValue(255);
    ui->stereoPanRight->blockSignals(false);
    ui->flipStereo->blockSignals(true);
    ui->flipStereo->setChecked(false);
    ui->flipStereo->blockSignals(false);
    ui->angle->setValue(0);
    ui->distance->setValue(0);
}

void MusicFX::on_angle_valueChanged(int value)
{
    m_angle = (Sint16)(value);
    qDebug() << "Angle" << m_angle;
    updatePositionEffect();
    m_sendPanning = false;
}

void MusicFX::on_distance_valueChanged(int value)
{
    m_distance = (Uint8)value;
    qDebug() << "Distance" << m_distance;
    updatePositionEffect();
    m_sendPanning = false;
}


void MusicFX::on_resetPosition_clicked()
{
    m_angle = 0;
    m_distance = 0;
    ui->angle->setValue(0);
    ui->distance->setValue(0);
    updatePositionEffect();
    m_sendPanning = false;
}


void MusicFX::on_flipStereo_clicked(bool value)
{
    qDebug() << "Flip stereo" << value;
    m_channelFlip = value ? 1 : 0;
    updateChannelsFlip();
}


void MusicFX::on_stereoPanLeft_valueChanged(int value)
{
    qDebug() << "Pan left" << value;
    m_panLeft = (Uint8)value;
    updatePanningEffect();
    m_sendPanning = true;
}


void MusicFX::on_stereoPanRight_valueChanged(int value)
{
    qDebug() << "Pan right" << value;
    m_panRight = (Uint8)value;
    updatePanningEffect();
    m_sendPanning = true;
}


void MusicFX::on_resetPanning_clicked()
{
    m_channelFlip = 0;
    m_panLeft = 255;
    m_panRight = 255;
    ui->stereoPanLeft->blockSignals(true);
    ui->stereoPanLeft->setValue(255);
    ui->stereoPanLeft->blockSignals(false);
    ui->stereoPanRight->blockSignals(true);
    ui->stereoPanRight->setValue(255);
    ui->stereoPanRight->blockSignals(false);
    ui->flipStereo->blockSignals(true);
    ui->flipStereo->setChecked(false);
    ui->flipStereo->blockSignals(false);
    updatePanningEffect();
    updateChannelsFlip();
    m_sendPanning = true;
}


void MusicFX::updatePositionEffect()
{
    Mix_SetMusicEffectPosition(PGE_MusicPlayer::s_playMus, m_angle, m_distance);
}

void MusicFX::updatePanningEffect()
{
    Mix_SetMusicEffectPanning(PGE_MusicPlayer::s_playMus, m_panLeft, m_panRight);
}

void MusicFX::updateChannelsFlip()
{
    Mix_SetMusicEffectReverseStereo(PGE_MusicPlayer::s_playMus, m_channelFlip);
}

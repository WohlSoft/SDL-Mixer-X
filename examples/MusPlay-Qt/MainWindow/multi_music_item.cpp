#include <QMessageBox>
#include <QToolTip>
#include <QtDebug>
#include <QDateTime>

#include "multi_music_item.h"
#include "ui_multi_music_item.h"
#include "seek_bar.h"


static const char *musicTypeC(Mix_Music *mus)
{
    Mix_MusicType type  = MUS_NONE;
    if(mus)
        type = Mix_GetMusicType(mus);

    return (
               type == MUS_NONE ? "No Music" :
               type == MUS_CMD ? "CMD" :
               type == MUS_WAV ? "PCM Wave" :
               type == MUS_MOD ? "Tracker music" :
               type == MUS_MID ? "MIDI" :
               type == MUS_OGG ? "OGG" :
               type == MUS_MP3 ? "MP3" :
               type == MUS_FLAC ? "FLAC" :
#ifdef SDL_MIXER_X
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
(SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 2)
               type == MUS_OPUS ? "OPUS" :
#   endif
               type == MUS_ADLMIDI ? "IMF/MUS/XMI" :
               type == MUS_GME ? "Game Music Emulator" :
#else
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
(SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION > 0) || \
(SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION == 0 && SDL_MIXER_PATCHLEVEL >= 4)
               type == MUS_OPUS ? "OPUS" :
#   endif
#endif
               "<Unknown>");
}

MultiMusicItem::MultiMusicItem(QString music, QWidget *parent) :
    QWidget(parent),
    m_curMusPath(music),
    ui(new Ui::MultiMusicItem)
{
    ui->setupUi(this);
    ui->musicTitle->setText(QString());
    ui->musicType->setText(QString::fromUtf8(musicTypeC(nullptr)));

    m_seekBar = new SeekBar(this);
    ui->gridLayout->removeWidget(ui->musicPosition);
    ui->gridLayout->addWidget(m_seekBar, 1, 0, 1, 8);
    m_seekBar->setLength(100);
    m_seekBar->setVisible(true);
    ui->musicPosition->setVisible(false);

    QObject::connect(&m_positionWatcher, SIGNAL(timeout()), this, SLOT(updatePositionSlider()));
    QObject::connect(m_seekBar, &SeekBar::positionSeeked, this, &MultiMusicItem::musicPosition_seeked);

    ui->tempo->setVisible(false);
    m_seekBar->setEnabled(false);
}

MultiMusicItem::~MultiMusicItem()
{
    on_stop_clicked();
    closeMusic();
    delete ui;
}

void MultiMusicItem::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MultiMusicItem::on_stop_clicked()
{
    ui->playpause->setIcon(QIcon(":/buttons/play.png"));
    ui->playpause->setToolTip(tr("Play"));

    if(!m_curMus)
        return;

    if(Mix_PlayingMusicStream(m_curMus))
    {
        qDebug() << "Stopping music" << m_curMusPath;
        Mix_HaltMusicStream(m_curMus);
    }
}

void MultiMusicItem::updatePositionSlider()
{
    if(!m_curMus)
        return;

    const double pos = Mix_GetMusicPosition(m_curMus);
    m_positionWatcherLock = true;
    if(pos < 0.0)
    {
        m_seekBar->setEnabled(false);
        m_positionWatcher.stop();
    }
    else
    {
        m_seekBar->setPosition(pos);
        ui->playingTimeLabel->setText(QDateTime::fromTime_t((uint)std::floor(pos)).toUTC().toString("hh:mm:ss"));
    }
    m_positionWatcherLock = false;
}

void MultiMusicItem::closeMusic()
{
    if(m_curMus)
    {
        m_positionWatcher.stop();
        on_stop_clicked();
        Mix_FreeMusic(m_curMus);
        m_curMus = nullptr;

        ui->musicTitle->setText(QString());
        ui->playingTimeLabel->setText("--:--:--");
        ui->playingTimeLenghtLabel->setText("/ --:--:--");
        m_seekBar->setPosition(0.0);
        m_seekBar->setEnabled(false);
    }
}

void MultiMusicItem::openMusic()
{
    if(m_curMus)
        return; // Already open

    qDebug() << "Opening" << m_curMusPath << ui->pathArgs->text();
    Mix_SetLockMIDIArgs(ui->pathArgs->text().isEmpty() ? 1 : 0);
    m_curMus = Mix_LoadMUS_RW_ARG(SDL_RWFromFile(m_curMusPath.toUtf8().data(), "rb"),
                                  1,
                                  ui->pathArgs->text().toUtf8().data());
    if(!m_curMus)
        QMessageBox::warning(this, tr("Can't open music"), tr("Can't open music: %1").arg(Mix_GetError()));
    else
    {
        ui->musicTitle->setText(QString::fromUtf8(Mix_GetMusicTitle(m_curMus)));
        ui->musicType->setText(QString::fromUtf8(musicTypeC(m_curMus)));

        m_positionWatcher.stop();
        m_seekBar->setEnabled(false);
        ui->playingTimeLabel->setText("--:--:--");
        ui->playingTimeLenghtLabel->setText("/ --:--:--");

        double total = Mix_GetMusicTotalTime(m_curMus);
        double curPos = Mix_GetMusicPosition(m_curMus);

        double loopStart = Mix_GetMusicLoopStartTime(m_curMus);
        double loopEnd = Mix_GetMusicLoopEndTime(m_curMus);

        ui->tempo->setVisible(Mix_GetMusicTempo(m_curMus) >= 0.0);

        if(total > 0.0 && curPos >= 0.0)
        {
            m_seekBar->setEnabled(true);
            m_seekBar->setLength(total);
            m_seekBar->setPosition(0.0);
            m_seekBar->setLoopPoints(loopStart, loopEnd);
            ui->playingTimeLenghtLabel->setText(QDateTime::fromTime_t((uint)std::floor(total)).toUTC().toString("/ hh:mm:ss"));
            m_positionWatcher.start(128);
        }
    }
}

void MultiMusicItem::reOpenMusic()
{
    if(m_curMus)
        closeMusic();
    openMusic();
}


void MultiMusicItem::on_playpause_clicked()
{
    openMusic();

    if(!m_curMus)
        return;

    if(Mix_PlayingMusicStream(m_curMus))
    {
        if(Mix_PausedMusicStream(m_curMus))
        {
            qDebug() << "Resuming music" << m_curMusPath;
            Mix_ResumeMusicStream(m_curMus);
            ui->playpause->setIcon(QIcon(":/buttons/pause.png"));
            ui->playpause->setToolTip(tr("Pause"));
        }
        else
        {
            qDebug() << "Pausing music" << m_curMusPath;
            Mix_PauseMusicStream(m_curMus);
            ui->playpause->setIcon(QIcon(":/buttons/play.png"));
            ui->playpause->setToolTip(tr("Resume"));
        }
    }
    else
    {
        qDebug() << "Play music" << m_curMusPath;
        Mix_PlayMusicStream(m_curMus, -1);
        ui->playpause->setIcon(QIcon(":/buttons/pause.png"));
        ui->playpause->setToolTip(tr("Pause"));
    }
}


void MultiMusicItem::on_closeMusic_clicked()
{
    closeMusic();
    emit wannaClose(this);
}

void MultiMusicItem::on_pathArgs_editingFinished()
{
    reOpenMusic();
}

void MultiMusicItem::on_musicVolume_sliderMoved(int position)
{
    if(m_curMus)
    {
        Mix_VolumeMusicStream(m_curMus, position);
        QToolTip::showText(QCursor::pos(), QString("%1").arg(position), this);
    }
}


void MultiMusicItem::on_tempo_sliderMoved(int position)
{
    if(m_curMus)
    {
        double tempoFactor = 1.0 + 0.01 * double(position);
        Mix_SetMusicTempo(m_curMus, tempoFactor);
        QToolTip::showText(QCursor::pos(), QString("%1").arg(tempoFactor), this);
    }
}

void MultiMusicItem::musicPosition_seeked(double value)
{
    if(m_positionWatcherLock)
        return;

    qDebug() << "Seek to: " << value;
    if(Mix_PlayingMusicStream(m_curMus))
    {
        Mix_SetMusicStreamPosition(m_curMus, value);
        ui->playingTimeLabel->setText(QDateTime::fromTime_t((uint)value).toUTC().toString("hh:mm:ss"));
    }
}

void MultiMusicItem::on_playFadeIn_clicked()
{
    openMusic();

    if(!m_curMus)
        return;

    if(!Mix_PlayingMusicStream(m_curMus))
        Mix_FadeInMusicStream(m_curMus, -1, 4000);
}


void MultiMusicItem::on_stopFadeOut_clicked()
{
    if(!m_curMus)
        return;

    if(Mix_PlayingMusicStream(m_curMus))
        Mix_FadeOutMusicStream(m_curMus, 4000);
}


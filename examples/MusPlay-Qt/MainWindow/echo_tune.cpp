#include "echo_tune.h"
#include "ui_echo_tune.h"
#include "../Effects/spc_echo.h"

EchoTune::EchoTune(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EchoTune)
{
    ui->setupUi(this);
}

EchoTune::~EchoTune()
{
    delete ui;
}

void EchoTune::on_echo_reload_clicked()
{
    ui->echo_eon->blockSignals(true);
    ui->echo_eon->setChecked(echoEffectGetReg(ECHO_EON) != 0);
    ui->echo_eon->blockSignals(false);

    ui->echo_edl->blockSignals(true);
    ui->echo_edl->setValue(echoEffectGetReg(ECHO_EDL));
    ui->echo_edl->blockSignals(false);

    ui->echo_efb->blockSignals(true);
    ui->echo_efb->setValue(echoEffectGetReg(ECHO_EFB));
    ui->echo_efb->blockSignals(false);

    ui->echo_mvoll->blockSignals(true);
    ui->echo_mvoll->setValue(echoEffectGetReg(ECHO_MVOLL));
    ui->echo_mvoll->blockSignals(false);

    ui->echo_mvolr->blockSignals(true);
    ui->echo_mvolr->setValue(echoEffectGetReg(ECHO_MVOLR));
    ui->echo_mvolr->blockSignals(false);

    ui->echo_evoll->blockSignals(true);
    ui->echo_evoll->setValue(echoEffectGetReg(ECHO_EVOLL));
    ui->echo_evoll->blockSignals(false);

    ui->echo_evolr->blockSignals(true);
    ui->echo_evolr->setValue(echoEffectGetReg(ECHO_EVOLR));
    ui->echo_evolr->blockSignals(false);

    ui->echo_fir0->setValue(echoEffectGetReg(ECHO_FIR0));
    ui->echo_fir1->setValue(echoEffectGetReg(ECHO_FIR1));
    ui->echo_fir2->setValue(echoEffectGetReg(ECHO_FIR2));
    ui->echo_fir3->setValue(echoEffectGetReg(ECHO_FIR3));
    ui->echo_fir4->setValue(echoEffectGetReg(ECHO_FIR4));
    ui->echo_fir5->setValue(echoEffectGetReg(ECHO_FIR5));
    ui->echo_fir6->setValue(echoEffectGetReg(ECHO_FIR6));
    ui->echo_fir7->setValue(echoEffectGetReg(ECHO_FIR7));
}

void EchoTune::on_reset_clicked()
{
    echoEffectResetDefaults();
    on_echo_reload_clicked();
}

void EchoTune::on_echo_eon_clicked(bool checked)
{
    echoEffectSetReg(ECHO_EON, checked ? 1 : 0);
}

void EchoTune::on_echo_edl_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_EDL, (Uint8)arg1);
}

void EchoTune::on_echo_efb_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_EFB, (Uint8)arg1);
}

void EchoTune::on_echo_mvoll_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_MVOLL, (Uint8)arg1);
}

void EchoTune::on_echo_mvolr_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_MVOLR, (Uint8)arg1);
}

void EchoTune::on_echo_evoll_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_EVOLL, (Uint8)arg1);
}

void EchoTune::on_echo_evolr_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_EVOLR, (Uint8)arg1);
}

void EchoTune::on_echo_fir0_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR0, (Uint8)arg1);
}

void EchoTune::on_echo_fir1_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR1, (Uint8)arg1);
}

void EchoTune::on_echo_fir2_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR2, (Uint8)arg1);
}

void EchoTune::on_echo_fir3_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR3, (Uint8)arg1);
}

void EchoTune::on_echo_fir4_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR4, (Uint8)arg1);
}

void EchoTune::on_echo_fir5_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR5, (Uint8)arg1);
}

void EchoTune::on_echo_fir6_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR6, (Uint8)arg1);
}

void EchoTune::on_echo_fir7_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR7, (Uint8)arg1);
}

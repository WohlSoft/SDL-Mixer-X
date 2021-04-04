#include "echo_tune.h"
#include "ui_echo_tune.h"
#include <QSettings>
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

void EchoTune::saveSetup()
{
    QSettings set;

    set.beginGroup("spc-echo");

    set.setValue("eon", ui->echo_eon->isChecked());
    set.setValue("edl", ui->echo_edl->value());
    set.setValue("efb", ui->echo_efb->value());

    set.setValue("mvoll", ui->echo_mvoll->value());
    set.setValue("mvolr", ui->echo_mvolr->value());
    set.setValue("evoll", ui->echo_evoll->value());
    set.setValue("evolr", ui->echo_evolr->value());

    set.setValue("fir0", ui->echo_fir0->value());
    set.setValue("fir1", ui->echo_fir1->value());
    set.setValue("fir2", ui->echo_fir2->value());
    set.setValue("fir3", ui->echo_fir3->value());
    set.setValue("fir4", ui->echo_fir4->value());
    set.setValue("fir5", ui->echo_fir5->value());
    set.setValue("fir6", ui->echo_fir6->value());
    set.setValue("fir7", ui->echo_fir7->value());
    set.endGroup();

    set.sync();
}

void EchoTune::loadSetup()
{
    QSettings set;

    set.beginGroup("spc-echo");

    ui->echo_eon->blockSignals(true);
    ui->echo_eon->setChecked(set.value("eon", echoEffectGetReg(ECHO_EON) != 0).toBool());
    ui->echo_eon->blockSignals(false);

    ui->echo_edl->blockSignals(true);
    ui->echo_edl->setValue(set.value("edl", echoEffectGetReg(ECHO_EDL)).toInt());
    ui->echo_edl->blockSignals(false);

    ui->echo_efb->blockSignals(true);
    ui->echo_efb->setValue(set.value("efb", echoEffectGetReg(ECHO_EFB)).toInt());
    ui->echo_efb->blockSignals(false);

    ui->echo_mvoll->blockSignals(true);
    ui->echo_mvoll->setValue(set.value("mvoll", echoEffectGetReg(ECHO_MVOLL)).toInt());
    ui->echo_mvoll->blockSignals(false);

    ui->echo_mvolr->blockSignals(true);
    ui->echo_mvolr->setValue(set.value("mvolr", echoEffectGetReg(ECHO_MVOLR)).toInt());
    ui->echo_mvolr->blockSignals(false);

    ui->echo_evoll->blockSignals(true);
    ui->echo_evoll->setValue(set.value("evoll", echoEffectGetReg(ECHO_EVOLL)).toInt());
    ui->echo_evoll->blockSignals(false);

    ui->echo_evolr->blockSignals(true);
    ui->echo_evolr->setValue(set.value("evolr", echoEffectGetReg(ECHO_EVOLR)).toInt());
    ui->echo_evolr->blockSignals(false);

    ui->echo_fir0->setValue(set.value("fir0", echoEffectGetReg(ECHO_FIR0)).toInt());
    ui->echo_fir1->setValue(set.value("fir1", echoEffectGetReg(ECHO_FIR1)).toInt());
    ui->echo_fir2->setValue(set.value("fir2", echoEffectGetReg(ECHO_FIR2)).toInt());
    ui->echo_fir3->setValue(set.value("fir3", echoEffectGetReg(ECHO_FIR3)).toInt());
    ui->echo_fir4->setValue(set.value("fir4", echoEffectGetReg(ECHO_FIR4)).toInt());
    ui->echo_fir5->setValue(set.value("fir5", echoEffectGetReg(ECHO_FIR5)).toInt());
    ui->echo_fir6->setValue(set.value("fir6", echoEffectGetReg(ECHO_FIR6)).toInt());
    ui->echo_fir7->setValue(set.value("fir7", echoEffectGetReg(ECHO_FIR7)).toInt());

    set.endGroup();
}

void EchoTune::sendAll()
{
    echoEffectSetReg(ECHO_EON, ui->echo_eon->isChecked() ? 1 : 0);
    echoEffectSetReg(ECHO_EDL, (Uint8)ui->echo_edl->value());
    echoEffectSetReg(ECHO_EFB, (Uint8)ui->echo_efb->value());
    echoEffectSetReg(ECHO_MVOLL, (Uint8)ui->echo_mvoll->value());
    echoEffectSetReg(ECHO_MVOLR, (Uint8)ui->echo_mvolr->value());
    echoEffectSetReg(ECHO_EVOLL, (Uint8)ui->echo_evoll->value());
    echoEffectSetReg(ECHO_EVOLR, (Uint8)ui->echo_evolr->value());

    echoEffectSetReg(ECHO_FIR0, (Uint8)(Sint8)ui->echo_fir0->value());
    echoEffectSetReg(ECHO_FIR1, (Uint8)(Sint8)ui->echo_fir1->value());
    echoEffectSetReg(ECHO_FIR2, (Uint8)(Sint8)ui->echo_fir2->value());
    echoEffectSetReg(ECHO_FIR3, (Uint8)(Sint8)ui->echo_fir3->value());
    echoEffectSetReg(ECHO_FIR4, (Uint8)(Sint8)ui->echo_fir4->value());
    echoEffectSetReg(ECHO_FIR5, (Uint8)(Sint8)ui->echo_fir5->value());
    echoEffectSetReg(ECHO_FIR6, (Uint8)(Sint8)ui->echo_fir6->value());
    echoEffectSetReg(ECHO_FIR7, (Uint8)(Sint8)ui->echo_fir7->value());
}

void EchoTune::on_save_clicked()
{
    saveSetup();
}

void EchoTune::on_sendAll_clicked()
{
    sendAll();
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
    echoEffectSetReg(ECHO_EFB, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_mvoll_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_MVOLL, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_mvolr_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_MVOLR, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_evoll_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_EVOLL, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_evolr_valueChanged(int arg1)
{
    echoEffectSetReg(ECHO_EVOLR, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir0_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR0, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir1_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR1, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir2_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR2, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir3_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR3, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir4_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR4, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir5_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR5, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir6_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR6, (Uint8)(Sint8)arg1);
}

void EchoTune::on_echo_fir7_sliderMoved(int arg1)
{
    echoEffectSetReg(ECHO_FIR7, (Uint8)(Sint8)arg1);
}


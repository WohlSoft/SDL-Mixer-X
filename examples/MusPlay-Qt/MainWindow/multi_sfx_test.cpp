#include <QFileDialog>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include "multi_sfx_test.h"
#include "multi_sfx_item.h"
#include "ui_multi_sfx_test.h"

#include "qfile_dialogs_default_options.hpp"


static MultiSfxTester* m_testerSelf = nullptr;

MultiSfxTester::MultiSfxTester(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::MultiSfxTester)
{
    Q_ASSERT(!m_testerSelf); // No multiple instances allowed!
    ui->setupUi(this);
    m_testerSelf = this;
    Mix_ChannelFinished(&sfxFinished);
    QObject::connect(&m_randomPlayTrigger,
                     &QTimer::timeout,
                     this,
                     &MultiSfxTester::playRandom);
    qsrand((uint)time(nullptr));
}

MultiSfxTester::~MultiSfxTester()
{
    Mix_ChannelFinished(nullptr);
    delete ui;
    m_testerSelf = nullptr;
}

void MultiSfxTester::openSfxByArg(QString musPath)
{
    auto* it = new MultiSfxItem(musPath, this);
    m_items.push_back(it);
    QObject::connect(it,
                     &MultiSfxItem::wannaClose,
                     this,
                     &MultiSfxTester::wannaClose);
    auto* l = qobject_cast<QVBoxLayout*>(ui->sfxList->widget()->layout());
    l->insertWidget(l->count() - 1, it);
}

void MultiSfxTester::reloadAll()
{
    for(auto& i : m_items)
        i->reOpenSfx();
}

void MultiSfxTester::changeEvent(QEvent* e)
{
    QDialog::changeEvent(e);

    switch(e->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    default:
        break;
    }
}

void MultiSfxTester::closeEvent(QCloseEvent* e)
{
    for(auto* it : m_items)
    {
        auto* l = qobject_cast<QVBoxLayout*>(ui->sfxList->widget()->layout());
        l->removeWidget(it);
        it->on_stop_clicked();
        it->deleteLater();
    }

    m_items.clear();
}

void MultiSfxTester::dropEvent(QDropEvent* e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    for(const QUrl& url : e->mimeData()->urls())
    {
        const QString& fileName = url.toLocalFile();
        openSfxByArg(fileName);
    }
}

void MultiSfxTester::dragEnterEvent(QDragEnterEvent* e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void MultiSfxTester::anySfxFinished(int numChannel)
{
    for(auto& i : m_items)
        i->sfxStoppedHook(numChannel);
}

void MultiSfxTester::on_openSfx_clicked()
{
    QString file = QFileDialog::getOpenFileName(this,
                                                tr("Open SFX file"),
                                                (m_lastSfx.isEmpty() ?
                                                QApplication::applicationDirPath() :
                                                m_lastSfx),
                                                "All (*.*)", nullptr,
                                                c_fileDialogOptions);

    if(file.isEmpty())
        return;

    m_lastSfx = file;
    openSfxByArg(file);
}

void MultiSfxTester::on_randomPlaying_clicked(bool checked)
{
    if(checked)
    {
        m_randomPlayTrigger.start(200);
        ui->randomPlaying->setText(tr("Stop random"));
    }
    else
    {
        m_randomPlayTrigger.stop();
        ui->randomPlaying->setText(tr("Play random"));
    }
}

void MultiSfxTester::playRandom()
{
    m_randomPlayTrigger.setInterval(100 + (qrand() % ui->maxRandomDelay->value()));

    if(m_items.isEmpty())
        return;

    int len = m_items.size();
    int select = qrand() % len;
    m_items[select]->on_play_clicked();
}

void MultiSfxTester::wannaClose(MultiSfxItem* item)
{
    auto* l = qobject_cast<QVBoxLayout*>(ui->sfxList->widget()->layout());
    l->removeWidget(item);
    m_items.removeAll(item);
    item->on_stop_clicked();
    item->deleteLater();
}

void MultiSfxTester::sfxFinished(int numChannel)
{
    if(m_testerSelf)
        QMetaObject::invokeMethod(m_testerSelf,
                                  "anySfxFinished",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, numChannel));
}

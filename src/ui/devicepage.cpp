#include "devicepage.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace f75 {

static QLabel* makeValue()
{
    auto* l = new QLabel(QStringLiteral("—"));
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return l;
}

DevicePage::DevicePage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* title = new QLabel(tr("Device"));
    QFont f = title->font();
    f.setPointSizeF(f.pointSizeF() * 1.4);
    f.setBold(true);
    title->setFont(f);
    layout->addWidget(title);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(24);
    form->setVerticalSpacing(10);

    m_conn = makeValue();
    m_model = makeValue();
    m_fw = makeValue();
    m_id = makeValue();
    m_status = makeValue();

    form->addRow(tr("Connection"), m_conn);
    form->addRow(tr("Model"), m_model);
    form->addRow(tr("Firmware"), m_fw);
    form->addRow(tr("USB ID"), m_id);
    form->addRow(tr("Status"), m_status);
    layout->addLayout(form);

    auto* note = new QLabel(tr(
        "The AULA F75 is driven over the vendor HID collection "
        "(Usage Page 0xFF02, Report ID 0x13).  No kernel driver or Wine prefix "
        "is required — access is granted through the bundled udev rule."));
    note->setWordWrap(true);
    note->setEnabled(false);
    layout->addWidget(note);

    layout->addStretch(1);
}

void DevicePage::setConnection(bool connected, const QString& path)
{
    m_conn->setText(connected ? tr("Connected — %1").arg(path) : tr("Not connected"));
}

void DevicePage::setModel(const QString& model, int modelId)
{
    if (model.isEmpty())
    {
        m_model->setText(tr("Unknown"));
    }
    else
    {
        m_model->setText(tr("%1 (0x%2)").arg(model).arg(modelId, 2, 16, QLatin1Char('0')).toUpper());
    }
}

void DevicePage::setFirmware(const QString& fw)
{
    m_fw->setText(fw);
}

void DevicePage::setId(uint16_t vid, uint16_t pid)
{
    m_id->setText(QStringLiteral("%1:%2")
                      .arg(vid, 4, 16, QLatin1Char('0'))
                      .arg(pid, 4, 16, QLatin1Char('0')).toUpper());
}

void DevicePage::setStatus(const QString& text)
{
    m_status->setText(text);
}

} // namespace f75

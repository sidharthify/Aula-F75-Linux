#include "lightingpage.h"

#include "colorbutton.h"
#include "keyboardview.h"
#include "mainwindow.h"
#include "model/keymap.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

namespace f75 {

// The on-board effects the F75 exposes, in driver order.  The index maps to
// byte 10 of the LED profile (see MainWindow::sendEffect).
static const char* const kEffectNames[] = {
    "Fixed on", "Respire", "Rainbow", "Flash away", "Raindrops",
    "Ripples shining", "Stars twinkle", "Retro snake", "Neon stream",
    "Reaction", "Sine wave", "Rotating windmill", "Colorful waterfall",
    "Blossoming", "Self define", "Off"
};
static const int kEffectCount = 16;

LightingPage::LightingPage(MainWindow* mw, QWidget* parent)
    : QWidget(parent)
    , m_mw(mw)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    m_tabs = new QTabWidget(this);
    root->addWidget(m_tabs);

    // ----- Solid ---------------------------------------------------------
    {
        auto* page = new QWidget();
        auto* l = new QVBoxLayout(page);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(14);

        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Colour:")));
        m_solidColor = new ColorButton(m_mw->profile().effectColor, page);
        row->addWidget(m_solidColor);
        row->addStretch(1);
        l->addLayout(row);

        auto* apply = new QPushButton(tr("Apply to keyboard"), page);
        apply->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        l->addWidget(apply, 0, Qt::AlignLeft);
        l->addStretch(1);

        connect(m_solidColor, &ColorButton::colorChanged, this, [this] { onSolidChanged(); });
        connect(apply, &QPushButton::clicked, this, &LightingPage::applySolid);

        m_tabs->addTab(page, QIcon::fromTheme(QStringLiteral("color-fill")), tr("Solid"));
    }

    // ----- Per-key -------------------------------------------------------
    {
        auto* page = new QWidget();
        auto* l = new QVBoxLayout(page);
        l->setContentsMargins(12, 12, 12, 12);
        l->setSpacing(10);

        m_view = new KeyboardView(page);
        l->addWidget(m_view, 1);

        auto* controls = new QGroupBox(tr("Paint"), page);
        auto* cl = new QHBoxLayout(controls);
        cl->addWidget(new QLabel(tr("Colour:")));
        m_paintColor = new ColorButton(QColor(0, 160, 255), controls);
        cl->addWidget(m_paintColor);
        cl->addWidget(new QLabel(tr("Brightness:")));
        m_brightness = new QSlider(Qt::Horizontal, controls);
        m_brightness->setRange(5, 100);
        m_brightness->setValue(m_mw->profile().keyBrightness);
        m_brightness->setFixedWidth(160);
        cl->addWidget(m_brightness);
        m_live = new QCheckBox(tr("Live preview"), controls);
        m_live->setChecked(true);
        cl->addWidget(m_live);
        cl->addStretch(1);
        l->addWidget(controls);

        auto* presets = new QHBoxLayout();
        auto mk = [&](const QString& t, const char* icon, void (LightingPage::*slot)())
        {
            auto* b = new QPushButton(QIcon::fromTheme(QString::fromLatin1(icon)), t, page);
            connect(b, &QPushButton::clicked, this, slot);
            presets->addWidget(b);
        };
        mk(tr("All"), "edit-select-all", &LightingPage::presetAll);
        mk(tr("Off"), "edit-delete", &LightingPage::presetOff);
        mk(tr("WASD"), "input-gaming", &LightingPage::presetWasd);
        mk(tr("Arrows"), "go-up", &LightingPage::presetArrows);
        mk(tr("Rows"), "format-justify-fill", &LightingPage::presetRows);
        mk(tr("Random"), "roll", &LightingPage::presetRandom);
        presets->addStretch(1);

        auto* apply = new QPushButton(tr("Apply to keyboard"), page);
        apply->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        presets->addWidget(apply);
        l->addLayout(presets);

        connect(apply, &QPushButton::clicked, this, &LightingPage::applyPerKey);
        connect(m_view, &KeyboardView::keyPainted, this, &LightingPage::onPerKeyPainted);
        connect(m_paintColor, &ColorButton::colorChanged, this, [this](const QColor& c) {
            m_view->setCurrentColor(c);
        });
        connect(m_brightness, &QSlider::valueChanged, this, [this](int v) {
            m_mw->profile().keyBrightness = v;
            m_view->setDisplayBrightness(v);
            if (m_live->isChecked())
            {
                m_mw->sendPerKey();
            }
        });

        m_view->setCurrentColor(m_paintColor->color());
        m_view->setColors(m_mw->profile().colors);
        m_view->setDisplayBrightness(m_brightness->value());

        m_tabs->addTab(page, QIcon::fromTheme(QStringLiteral("preferences-desktop-keyboard")),
                       tr("Per-Key"));
    }

    // ----- Effects -------------------------------------------------------
    {
        auto* page = new QWidget();
        auto* l = new QVBoxLayout(page);
        l->setContentsMargins(20, 20, 20, 20);
        l->setSpacing(14);

        auto* form = new QFormLayout();
        form->setHorizontalSpacing(24);
        form->setVerticalSpacing(12);

        m_effect = new QComboBox(page);
        for (int i = 0; i < kEffectCount; ++i)
        {
            m_effect->addItem(QIcon::fromTheme(QStringLiteral("video-display")),
                              tr(kEffectNames[i]));
        }
        m_effect->setCurrentIndex(qBound(0, m_mw->profile().effectId - 1, kEffectCount - 1));
        form->addRow(tr("Effect:"), m_effect);

        m_effectColor = new ColorButton(m_mw->profile().effectColor, page);
        form->addRow(tr("Colour:"), m_effectColor);

        m_effectBrightness = new QSlider(Qt::Horizontal, page);
        m_effectBrightness->setRange(0, 9);
        m_effectBrightness->setValue(qBound(0, int(m_mw->profile().brightness), 9));
        m_effectBrightness->setTickPosition(QSlider::TicksBelow);
        m_effectBrightness->setTickInterval(1);
        form->addRow(tr("Brightness:"), m_effectBrightness);

        m_effectSpeed = new QSlider(Qt::Horizontal, page);
        m_effectSpeed->setRange(0, 4);
        m_effectSpeed->setValue(m_mw->profile().speed);
        m_effectSpeed->setTickPosition(QSlider::TicksBelow);
        m_effectSpeed->setTickInterval(1);
        form->addRow(tr("Speed:"), m_effectSpeed);

        m_direction = new QComboBox(page);
        m_direction->addItem(tr("Forward"));
        m_direction->addItem(tr("Reverse"));
        m_direction->setCurrentIndex(m_mw->profile().direction ? 1 : 0);
        form->addRow(tr("Direction:"), m_direction);

        m_colorful = new QCheckBox(tr("Colourful (random per key)"), page);
        m_colorful->setChecked(m_mw->profile().colorful);
        form->addRow(QString(), m_colorful);

        l->addLayout(form);

        auto* apply = new QPushButton(tr("Apply to keyboard"), page);
        apply->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        l->addWidget(apply, 0, Qt::AlignLeft);
        connect(apply, &QPushButton::clicked, this, &LightingPage::applyEffect);

        auto* note = new QLabel(tr(
            "On-board effects are written to the keyboard's LED profile with "
            "opcode 0x04, exactly as the Windows driver does.  Applying an "
            "effect leaves direct (Per-Key) mode."));
        note->setWordWrap(true);
        note->setEnabled(false);
        l->addWidget(note);
        l->addStretch(1);

        m_tabs->addTab(page, QIcon::fromTheme(QStringLiteral("effects")), tr("Effects"));
    }

    m_tabs->setCurrentIndex(1); // Per-Key is the headline feature
}

void LightingPage::refreshFromProfile()
{
    m_view->setColors(m_mw->profile().colors);
    m_brightness->setValue(m_mw->profile().keyBrightness);
    m_solidColor->blockSignals(true);
    m_solidColor->setColor(m_mw->profile().effectColor);
    m_solidColor->blockSignals(false);
}

void LightingPage::syncView()
{
    m_view->setColors(m_mw->profile().colors);
}

void LightingPage::onSolidChanged()
{
    m_mw->profile().effectColor = m_solidColor->color();
}

void LightingPage::onPerKeyPainted(int led, const QColor& c)
{
    Q_UNUSED(led);
    Q_UNUSED(c);
    // KeyboardView already stored the colour in its own copy; mirror to profile.
    m_mw->profile().colors = m_view->colors();
    if (m_live->isChecked())
    {
        m_mw->sendPerKey();
    }
}

void LightingPage::applySolid()
{
    m_mw->profile().effectColor = m_solidColor->color();
    m_mw->sendSolid(m_solidColor->color());
}

void LightingPage::applyPerKey()
{
    m_mw->profile().colors = m_view->colors();
    m_mw->sendPerKey(true);
}

void LightingPage::applyEffect()
{
    auto& p = m_mw->profile();
    p.effectId   = m_effect->currentIndex() + 1;
    p.effectColor = m_effectColor->color();
    p.brightness = static_cast<uint8_t>(m_effectBrightness->value());
    p.speed      = static_cast<uint8_t>(m_effectSpeed->value());
    p.direction  = static_cast<uint8_t>(m_direction->currentIndex());
    p.colorful   = m_colorful->isChecked();
    m_mw->sendEffect();
}

void LightingPage::presetAll()
{
    const QColor c = m_paintColor->color();
    for (int i = 0; i < m_mw->profile().colors.size(); ++i)
    {
        m_mw->profile().colors[i] = c;
    }
    syncView();
    applyPerKey();
}

void LightingPage::presetOff()
{
    for (int i = 0; i < m_mw->profile().colors.size(); ++i)
    {
        m_mw->profile().colors[i] = QColor(0, 0, 0, 0);
    }
    syncView();
    applyPerKey();
}

void LightingPage::presetWasd()
{
    static const int leds[] = { 8, 9, 14, 15 }; // Q A S D by LED value
    for (int i = 0; i < m_mw->profile().colors.size(); ++i)
    {
        m_mw->profile().colors[i] = QColor(0, 0, 0, 0);
    }
    for (int led : leds)
    {
        m_mw->profile().colors[led] = m_paintColor->color();
    }
    syncView();
    applyPerKey();
}

void LightingPage::presetArrows()
{
    static const int leds[] = { 77, 82, 83, 89 }; // Left Up Down Right
    for (int i = 0; i < m_mw->profile().colors.size(); ++i)
    {
        m_mw->profile().colors[i] = QColor(0, 0, 0, 0);
    }
    for (int led : leds)
    {
        m_mw->profile().colors[led] = m_paintColor->color();
    }
    syncView();
    applyPerKey();
}

void LightingPage::presetRows()
{
    static const QColor rowColors[6] = {
        QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255),
        QColor(255, 255, 0), QColor(255, 0, 255), QColor(0, 255, 255),
    };
    for (int led = 0; led < m_mw->profile().colors.size(); ++led)
    {
        m_mw->profile().colors[led] = rowColors[led % 6];
    }
    syncView();
    applyPerKey();
}

void LightingPage::presetRandom()
{
    auto* rng = QRandomGenerator::global();
    for (int led = 0; led < m_mw->profile().colors.size(); ++led)
    {
        m_mw->profile().colors[led] = QColor(rng->bounded(256), rng->bounded(256), rng->bounded(256));
    }
    syncView();
    applyPerKey();
}

} // namespace f75

#include "settingsdialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QDebug>
#include "settingsmanager.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_opacitySlider(nullptr)
    , m_fontSizeSpin(nullptr)
    , m_fontColorEdit(nullptr)
    , m_fontFamilyCombo(nullptr)
    , m_encodingCombo(nullptr)
    , m_autoChapterCombo(nullptr)
    , m_saveButton(nullptr)
    , m_cancelButton(nullptr)
{
    setWindowTitle("设置");
    setMinimumSize(450, 500);
    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* shortcutGroup = new QGroupBox("快捷键设置", this);
    shortcutGroup->setObjectName("shortcutGroup");
    QGridLayout* shortcutLayout = new QGridLayout(shortcutGroup);

    createShortcutRow("透明背景切换", "ToggleTransparency");
    createShortcutRow("隐藏窗口", "HideWindow");
    createShortcutRow("打开文件", "OpenFile");
    createShortcutRow("退出程序", "Quit");
    createShortcutRow("下一章", "NextChapter");
    createShortcutRow("上一章", "PrevChapter");

    mainLayout->addWidget(shortcutGroup);

    QGroupBox* appearanceGroup = new QGroupBox("外观设置", this);
    QGridLayout* appearanceLayout = new QGridLayout(appearanceGroup);

    QLabel* opacityLabel = new QLabel("背景透明度:", this);
    m_opacitySlider = new QSlider(Qt::Horizontal, this);
    m_opacitySlider->setRange(10, 100);
    QLabel* opacityValueLabel = new QLabel("100%", this);
    connect(m_opacitySlider, &QSlider::valueChanged,
            [opacityValueLabel](int value) {
                opacityValueLabel->setText(QString::number(value) + "%");
            });

    appearanceLayout->addWidget(opacityLabel, 0, 0);
    appearanceLayout->addWidget(m_opacitySlider, 0, 1);
    appearanceLayout->addWidget(opacityValueLabel, 0, 2);

    QLabel* fontSizeLabel = new QLabel("字体大小:", this);
    m_fontSizeSpin = new QSpinBox(this);
    m_fontSizeSpin->setRange(8, 72);
    appearanceLayout->addWidget(fontSizeLabel, 1, 0);
    appearanceLayout->addWidget(m_fontSizeSpin, 1, 1, 1, 2);

    QLabel* fontColorLabel = new QLabel("字体颜色:", this);
    m_fontColorEdit = new QLineEdit(this);
    m_fontColorEdit->setPlaceholderText("#000000");
    appearanceLayout->addWidget(fontColorLabel, 2, 0);
    appearanceLayout->addWidget(m_fontColorEdit, 2, 1, 1, 2);

    QLabel* fontFamilyLabel = new QLabel("字体:", this);
    m_fontFamilyCombo = new QComboBox(this);
    QStringList fonts = QFontDatabase().families();
    m_fontFamilyCombo->addItems(fonts);
    appearanceLayout->addWidget(fontFamilyLabel, 3, 0);
    appearanceLayout->addWidget(m_fontFamilyCombo, 3, 1, 1, 2);

    mainLayout->addWidget(appearanceGroup);

    QGroupBox* fileGroup = new QGroupBox("文件设置", this);
    QGridLayout* fileLayout = new QGridLayout(fileGroup);

    QLabel* encodingLabel = new QLabel("默认编码:", this);
    m_encodingCombo = new QComboBox(this);
    m_encodingCombo->addItems(QStringList() << "UTF-8" << "GBK" << "GB2312" << "Big5");
    fileLayout->addWidget(encodingLabel, 0, 0);
    fileLayout->addWidget(m_encodingCombo, 0, 1, 1, 2);

    QLabel* autoChapterLabel = new QLabel("自动识别目录:", this);
    m_autoChapterCombo = new QComboBox(this);
    m_autoChapterCombo->addItems(QStringList() << "启用" << "禁用");
    fileLayout->addWidget(autoChapterLabel, 1, 0);
    fileLayout->addWidget(m_autoChapterCombo, 1, 1, 1, 2);

    mainLayout->addWidget(fileGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_saveButton = new QPushButton("保存", this);
    m_cancelButton = new QPushButton("取消", this);
    connect(m_saveButton, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancel);

    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::createShortcutRow(const QString& label, const QString& name)
{
    static QGridLayout* s_shortcutLayout = nullptr;
    
    QGroupBox* shortcutGroup = findChild<QGroupBox*>("shortcutGroup");
    if (shortcutGroup) {
        s_shortcutLayout = qobject_cast<QGridLayout*>(shortcutGroup->layout());
    }
    
    if (!s_shortcutLayout) {
        return;
    }
    
    int row = s_shortcutLayout->rowCount();
    
    QLabel* nameLabel = new QLabel(label, this);
    KeyCaptureButton* captureBtn = new KeyCaptureButton(this);
    captureBtn->setShortcutName(name);
    
    QLineEdit* edit = new QLineEdit(this);
    edit->setReadOnly(true);
    edit->setMaximumWidth(150);
    
    m_shortcutButtons.insert(name, captureBtn);
    m_shortcutEdits.insert(name, edit);
    
    connect(captureBtn, &KeyCaptureButton::shortcutCaptured,
            this, &SettingsDialog::onCaptureShortcut);
    connect(captureBtn, &KeyCaptureButton::shortcutCaptured,
            [edit](const QString&, const QString& shortcut) {
                edit->setText(shortcut);
            });
    
    s_shortcutLayout->addWidget(nameLabel, row, 0);
    s_shortcutLayout->addWidget(edit, row, 1);
    s_shortcutLayout->addWidget(captureBtn, row, 2);
}

void SettingsDialog::loadSettings()
{
    SettingsManager* settings = SettingsManager::instance();

    QMapIterator<QString, KeyCaptureButton*> it(m_shortcutButtons);
    while (it.hasNext()) {
        it.next();
        QString shortcut = settings->getShortcut(it.key());
        m_shortcutButtons.value(it.key())->setText(shortcut);
        m_shortcutEdits.value(it.key())->setText(shortcut);
    }

    m_opacitySlider->setValue(settings->getBackgroundOpacity());
    m_fontSizeSpin->setValue(settings->getFontSize());
    m_fontColorEdit->setText(settings->getFontColor());

    int fontIndex = m_fontFamilyCombo->findText(settings->getFontFamily());
    if (fontIndex >= 0) {
        m_fontFamilyCombo->setCurrentIndex(fontIndex);
    }

    int encodingIndex = m_encodingCombo->findText(settings->getDefaultEncoding());
    if (encodingIndex >= 0) {
        m_encodingCombo->setCurrentIndex(encodingIndex);
    }

    m_autoChapterCombo->setCurrentIndex(settings->getAutoChapterDetection() ? 0 : 1);
}

void SettingsDialog::saveSettings()
{
    SettingsManager* settings = SettingsManager::instance();

    QMapIterator<QString, KeyCaptureButton*> it(m_shortcutButtons);
    while (it.hasNext()) {
        it.next();
        settings->setShortcut(it.key(), m_shortcutButtons.value(it.key())->text());
    }

    settings->setBackgroundOpacity(m_opacitySlider->value());
    settings->setFontSize(m_fontSizeSpin->value());
    settings->setFontColor(m_fontColorEdit->text());
    settings->setFontFamily(m_fontFamilyCombo->currentText());
    settings->setDefaultEncoding(m_encodingCombo->currentText());
    settings->setAutoChapterDetection(m_autoChapterCombo->currentIndex() == 0);

    settings->sync();
}

void SettingsDialog::onCaptureShortcut(const QString& name, const QString& shortcut)
{
    Q_UNUSED(name);
    Q_UNUSED(shortcut);
}

void SettingsDialog::onSave()
{
    saveSettings();
    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}

KeyCaptureButton::KeyCaptureButton(QWidget *parent)
    : QPushButton(parent)
    , m_shortcutName("")
    , m_capturing(false)
{
    setText("点击设置");
    setMaximumWidth(100);
}

void KeyCaptureButton::setShortcutName(const QString& name)
{
    m_shortcutName = name;
}

QString KeyCaptureButton::getShortcutName() const
{
    return m_shortcutName;
}

void KeyCaptureButton::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        m_capturing = false;
        setText("点击设置");
        return;
    }

    int key = event->key();
    if (key == Qt::Key_Control || key == Qt::Key_Shift ||
        key == Qt::Key_Alt || key == Qt::Key_Meta) {
        return;
    }

    QString shortcut;
    Qt::KeyboardModifiers mods = event->modifiers();

    if (mods & Qt::ControlModifier) shortcut += "Ctrl+";
    if (mods & Qt::ShiftModifier) shortcut += "Shift+";
    if (mods & Qt::AltModifier) shortcut += "Alt+";

    QString keyText = QKeySequence(key).toString();
    if (keyText.contains("+")) {
        keyText = keyText.split("+").last();
    }
    shortcut += keyText;

    setText(shortcut);
    m_capturing = false;

    emit shortcutCaptured(m_shortcutName, shortcut);
}

void KeyCaptureButton::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if (m_capturing) {
        m_capturing = false;
        setText("点击设置");
    }
}

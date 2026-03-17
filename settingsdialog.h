#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMap>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class KeyCaptureButton;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void loadSettings();
    void saveSettings();

private slots:
    void onCaptureShortcut(const QString& name, const QString& shortcut);
    void onSave();
    void onCancel();

private:
    void setupUi();
    void createShortcutRow(const QString& label, const QString& name);

    QMap<QString, KeyCaptureButton*> m_shortcutButtons;
    QMap<QString, QLineEdit*> m_shortcutEdits;

    QSlider* m_opacitySlider;
    QSpinBox* m_fontSizeSpin;
    QLineEdit* m_fontColorEdit;
    QComboBox* m_fontFamilyCombo;
    QComboBox* m_encodingCombo;
    QComboBox* m_autoChapterCombo;

    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

class KeyCaptureButton : public QPushButton
{
    Q_OBJECT

public:
    explicit KeyCaptureButton(QWidget *parent = nullptr);
    void setShortcutName(const QString& name);
    QString getShortcutName() const;

signals:
    void shortcutCaptured(const QString& name, const QString& shortcut);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QString m_shortcutName;
    bool m_capturing;
};

#endif

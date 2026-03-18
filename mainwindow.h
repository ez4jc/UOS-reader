#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextBrowser>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QListWidget>
#include <QCloseEvent>
#include <QPoint>
#include <QRect>
#include <QPushButton>
#include <QObject>

class TextReader;
class SettingsManager;
class ShortcutsManager;
class SystemTray;
class SettingsDialog;
class GlobalHotkey;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.zgyd.Reader")

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

public slots:
    Q_SCRIPTABLE void ToggleVisibility();

private slots:
    void onOpenFile();
    void onShowSettings();
    void onToggleTransparency();
    void onHideToTray();
    void onShortcutActivated(const QString& name);
    void onChapterSelected(int index);
    void onNextChapter();
    void onPrevChapter();
    void onFileLoaded(const QString& fileName);
    void onLoadError(const QString& error);
    void onSettingsChanged();
    void onQuit();
    void onRestoreFromTray();
    void onCloseButtonClicked();
    void onMinimizeButtonClicked();
    void onTextBrowserScroll(int value);

private:
    void setupUi();
    void createMenuBar();
    void createToolBar();
    void createTitleButtons();
    int chapterIndexForLine(int lineNumber) const;
    void registerShortcuts();
    void updateHideWindowShortcut();
    void updateAppearance();
    void updateChapterList();
    void enterTransparentMode();
    void exitTransparentMode();
    void scrollToChapter(int index);
    void scrollTextBrowser(QWheelEvent *event);

    QTextBrowser* m_textBrowser;
    QListWidget* m_chapterList;
    QPushButton* m_closeButton;
    QPushButton* m_minimizeButton;

    TextReader* m_textReader;
    SettingsManager* m_settings;
    ShortcutsManager* m_shortcuts;
    GlobalHotkey* m_hideHotkey;
    SystemTray* m_tray;

    bool m_isTransparent;
    int m_currentChapter;
    bool m_isDragging;
    bool m_isChangingChapter;
    QPoint m_dragPosition;
    QRect m_lastVisibleGeometry;
};

#endif

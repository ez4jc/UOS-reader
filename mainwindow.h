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
#include <QPushButton>

class TextReader;
class SettingsManager;
class ShortcutsManager;
class SystemTray;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

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

private:
    void setupUi();
    void createMenuBar();
    void createToolBar();
    void createTitleButtons();
    void registerShortcuts();
    void updateAppearance();
    void updateChapterList();
    void enterTransparentMode();
    void exitTransparentMode();

    QTextBrowser* m_textBrowser;
    QListWidget* m_chapterList;
    QPushButton* m_closeButton;
    QPushButton* m_minimizeButton;

    TextReader* m_textReader;
    SettingsManager* m_settings;
    ShortcutsManager* m_shortcuts;
    SystemTray* m_tray;

    bool m_isTransparent;
    int m_currentChapter;
    bool m_isDragging;
    QPoint m_dragPosition;
};

#endif

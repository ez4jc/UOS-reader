#include "mainwindow.h"
#include "textreader.h"
#include "settingsmanager.h"
#include "shortcutsmanager.h"
#include "systemtray.h"
#include "settingsdialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QSplitter>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_textBrowser(nullptr)
    , m_chapterList(nullptr)
    , m_closeButton(nullptr)
    , m_minimizeButton(nullptr)
    , m_textReader(nullptr)
    , m_settings(nullptr)
    , m_shortcuts(nullptr)
    , m_tray(nullptr)
    , m_isTransparent(false)
    , m_currentChapter(0)
    , m_isDragging(false)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setupUi();
    createTitleButtons();

    m_textReader = new TextReader(this);
    m_settings = SettingsManager::instance();
    m_shortcuts = new ShortcutsManager(this);
    m_tray = new SystemTray(this);

    m_tray->setParentWindow(this);

    connect(m_tray, &SystemTray::requestShowWindow, this, &MainWindow::onRestoreFromTray);
    connect(m_tray, &SystemTray::requestQuit, this, &MainWindow::onQuit);

    connect(m_textReader, &TextReader::fileLoaded, this, &MainWindow::onFileLoaded);
    connect(m_textReader, &TextReader::loadError, this, &MainWindow::onLoadError);

    registerShortcuts();
    updateAppearance();

    connect(m_settings, &SettingsManager::settingsChanged,
            this, &MainWindow::onSettingsChanged);

    setWindowTitle("阅读器");
    resize(900, 600);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    m_chapterList = new QListWidget(this);
    m_chapterList->setMaximumWidth(200);
    connect(m_chapterList, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
                if (item) {
                    onChapterSelected(m_chapterList->row(item));
                }
            });
    splitter->addWidget(m_chapterList);

    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setOpenExternalLinks(false);
    splitter->addWidget(m_textBrowser);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);

    setCentralWidget(splitter);

    createMenuBar();
    createToolBar();
    statusBar()->showMessage("就绪");
}

void MainWindow::createMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu("文件(&F)");

    QAction* openAction = new QAction("打开(&O)", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    fileMenu->addAction(openAction);

    fileMenu->addSeparator();

    QAction* quitAction = new QAction("退出(&Q)", this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &MainWindow::onQuit);
    fileMenu->addAction(quitAction);

    QMenu* viewMenu = menuBar()->addMenu("视图(&V)");

    QAction* transparencyAction = new QAction("透明背景(&T)", this);
    transparencyAction->setCheckable(true);
    connect(transparencyAction, &QAction::triggered, this, &MainWindow::onToggleTransparency);
    viewMenu->addAction(transparencyAction);

    viewMenu->addSeparator();

    QAction* hideAction = new QAction("隐藏到托盘(&H)", this);
    connect(hideAction, &QAction::triggered, this, &MainWindow::onHideToTray);
    viewMenu->addAction(hideAction);

    QMenu* settingsMenu = menuBar()->addMenu("设置(&S)");

    QAction* settingsAction = new QAction("首选项(&P)", this);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onShowSettings);
    settingsMenu->addAction(settingsAction);
}

void MainWindow::createToolBar()
{
    QToolBar* toolbar = addToolBar("工具栏");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);

    QAction* openAction = new QAction("打开", this);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    toolbar->addAction(openAction);

    toolbar->addSeparator();

    QAction* prevAction = new QAction("上一章", this);
    connect(prevAction, &QAction::triggered, this, &MainWindow::onPrevChapter);
    toolbar->addAction(prevAction);

    QAction* nextAction = new QAction("下一章", this);
    connect(nextAction, &QAction::triggered, this, &MainWindow::onNextChapter);
    toolbar->addAction(nextAction);

    toolbar->addSeparator();

    QAction* transAction = new QAction("透明", this);
    connect(transAction, &QAction::triggered, this, &MainWindow::onToggleTransparency);
    toolbar->addAction(transAction);

    QAction* hideAction = new QAction("隐藏", this);
    connect(hideAction, &QAction::triggered, this, &MainWindow::onHideToTray);
    toolbar->addAction(hideAction);
}

void MainWindow::createTitleButtons()
{
    m_minimizeButton = new QPushButton(this);
    m_minimizeButton->setText("─");
    m_minimizeButton->setFixedSize(30, 30);
    m_minimizeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #ffc107;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 15px;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #ff9800;"
        "}"
    );
    connect(m_minimizeButton, &QPushButton::clicked, this, &MainWindow::onMinimizeButtonClicked);

    m_closeButton = new QPushButton(this);
    m_closeButton->setText("×");
    m_closeButton->setFixedSize(30, 30);
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #ff4d4d;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 15px;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #ff0000;"
        "}"
    );
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::onCloseButtonClicked);
}

void MainWindow::onMinimizeButtonClicked()
{
    if (m_isTransparent) {
        exitTransparentMode();
    }
    showMinimized();
}

void MainWindow::onCloseButtonClicked()
{
    if (m_isTransparent) {
        exitTransparentMode();
    }
    onQuit();
}

void MainWindow::registerShortcuts()
{
    m_shortcuts->registerShortcut(this, "OpenFile", m_settings->getShortcut("OpenFile"));
    m_shortcuts->registerShortcut(this, "ToggleTransparency", m_settings->getShortcut("ToggleTransparency"));
    m_shortcuts->registerShortcut(this, "HideWindow", m_settings->getShortcut("HideWindow"));
    m_shortcuts->registerShortcut(this, "Quit", m_settings->getShortcut("Quit"));
    m_shortcuts->registerShortcut(this, "NextChapter", m_settings->getShortcut("NextChapter"));
    m_shortcuts->registerShortcut(this, "PrevChapter", m_settings->getShortcut("PrevChapter"));

    connect(m_shortcuts, &ShortcutsManager::shortcutActivated,
            this, &MainWindow::onShortcutActivated);
}

void MainWindow::updateAppearance()
{
    int opacity = m_settings->getBackgroundOpacity();
    int fontSize = m_settings->getFontSize();
    QString fontColor = m_settings->getFontColor();
    QString fontFamily = m_settings->getFontFamily();

    QString style = QString(
        "QTextBrowser {"
        "  background-color: rgba(255, 255, 255, %1);"
        "  color: %2;"
        "  font-family: %3;"
        "  font-size: %4px;"
        "}"
    ).arg(opacity * 2.55).arg(fontColor).arg(fontFamily).arg(fontSize);

    m_textBrowser->setStyleSheet(style);
    m_textBrowser->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    if (opacity < 100) {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet("MainWindow { background: transparent; }");
    } else {
        setAttribute(Qt::WA_TranslucentBackground, false);
        setStyleSheet("");
    }

    if (m_closeButton && !m_isTransparent) {
        m_closeButton->move(width() - 40, 5);
    }
    if (m_minimizeButton && !m_isTransparent) {
        m_minimizeButton->move(width() - 80, 5);
    }
}

void MainWindow::updateChapterList()
{
    m_chapterList->clear();

    if (!m_textReader->isLoaded()) {
        return;
    }

    QList<Chapter> chapters = m_textReader->getChapters();
    for (const Chapter& chapter : chapters) {
        m_chapterList->addItem(chapter.title);
    }
}

void MainWindow::onOpenFile()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "打开文本文件",
        QString(),
        "文本文件 (*.txt);;所有文件 (*.*)"
    );

    if (!filePath.isEmpty()) {
        m_textReader->loadFile(filePath);
    }
}

void MainWindow::onShowSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_shortcuts->reloadFromSettings();
        registerShortcuts();
        updateAppearance();
    }
}

void MainWindow::onToggleTransparency()
{
    if (m_isTransparent) {
        exitTransparentMode();
    } else {
        enterTransparentMode();
    }
}

void MainWindow::enterTransparentMode()
{
    m_isTransparent = true;
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    
    m_textBrowser->setStyleSheet(
        "QTextBrowser {"
        "  background-color: rgba(255, 255, 255, 0);"
        "  color: #000000;"
        "  border: none;"
        "  padding: 10px;"
        "}"
    );
    m_textBrowser->setAttribute(Qt::WA_TransparentForMouseEvents);
    
    menuBar()->hide();
    QToolBar* toolbar = findChild<QToolBar*>("mainToolbar");
    if (toolbar) toolbar->hide();
    statusBar()->hide();
    m_chapterList->hide();
    m_closeButton->hide();
    m_minimizeButton->hide();
    
    setStyleSheet("MainWindow { background: transparent; }");
    show();
}

void MainWindow::exitTransparentMode()
{
    m_isTransparent = false;
    setAttribute(Qt::WA_TranslucentBackground, false);
    setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    
    menuBar()->show();
    QToolBar* toolbar = findChild<QToolBar*>("mainToolbar");
    if (toolbar) toolbar->show();
    statusBar()->show();
    m_chapterList->show();
    m_closeButton->show();
    m_minimizeButton->show();
    
    updateAppearance();
    show();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_isDragging) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
    }
}

void MainWindow::onHideToTray()
{
    if (m_isTransparent) {
        exitTransparentMode();
    }
    hide();
    m_tray->show();
    m_tray->showMessage("阅读器已隐藏", "点击托盘图标恢复");
}

void MainWindow::onShortcutActivated(const QString& name)
{
    if (name == "OpenFile") {
        onOpenFile();
    } else if (name == "ToggleTransparency") {
        onToggleTransparency();
    } else if (name == "HideWindow") {
        onHideToTray();
    } else if (name == "Quit") {
        onQuit();
    } else if (name == "NextChapter") {
        onNextChapter();
    } else if (name == "PrevChapter") {
        onPrevChapter();
    }
}

void MainWindow::onChapterSelected(int index)
{
    if (index < 0) return;

    m_currentChapter = index;
    QString content = m_textReader->getChapterContent(index);
    m_textBrowser->setPlainText(content);

    statusBar()->showMessage(QString("第 %1 章").arg(index + 1));
}

void MainWindow::onNextChapter()
{
    QList<Chapter> chapters = m_textReader->getChapters();
    if (m_currentChapter < chapters.size() - 1) {
        m_currentChapter++;
        m_chapterList->setCurrentRow(m_currentChapter);
        onChapterSelected(m_currentChapter);
    }
}

void MainWindow::onPrevChapter()
{
    if (m_currentChapter > 0) {
        m_currentChapter--;
        m_chapterList->setCurrentRow(m_currentChapter);
        onChapterSelected(m_currentChapter);
    }
}

void MainWindow::onFileLoaded(const QString& fileName)
{
    setWindowTitle(fileName + " - 阅读器");
    updateChapterList();

    QList<Chapter> chapters = m_textReader->getChapters();
    if (!chapters.isEmpty()) {
        m_currentChapter = 0;
        onChapterSelected(0);
    }

    statusBar()->showMessage("文件加载成功");
}

void MainWindow::onLoadError(const QString& error)
{
    QMessageBox::warning(this, "加载错误", error);
    statusBar()->showMessage("文件加载失败");
}

void MainWindow::onSettingsChanged()
{
    updateAppearance();
}

void MainWindow::onRestoreFromTray()
{
    if (m_isTransparent) {
        exitTransparentMode();
    }
    show();
    activateWindow();
    raise();
}

void MainWindow::onQuit()
{
    close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    m_tray->hide();
}

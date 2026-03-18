#include "mainwindow.h"
#include "textreader.h"
#include "settingsmanager.h"
#include "shortcutsmanager.h"
#include "systemtray.h"
#include "settingsdialog.h"
#include "globalhotkey.h"

#include <QDBusConnection>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QListWidget>
#include <QSplitter>
#include <QDebug>
#include <QAbstractTextDocumentLayout>
#include <QWheelEvent>
#include <QScrollBar>
#include <QTextBlock>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_textBrowser(nullptr)
    , m_chapterList(nullptr)
    , m_closeButton(nullptr)
    , m_minimizeButton(nullptr)
    , m_textReader(nullptr)
    , m_settings(nullptr)
    , m_shortcuts(nullptr)
    , m_hideHotkey(nullptr)
    , m_tray(nullptr)
    , m_isTransparent(false)
    , m_currentChapter(0)
    , m_isDragging(false)
    , m_isChangingChapter(false)
{
    setWindowFlags(Qt::Window);
    setupUi();
    createTitleButtons();

    m_textReader = new TextReader(this);
    m_settings = SettingsManager::instance();
    m_shortcuts = new ShortcutsManager(this);
    m_hideHotkey = new GlobalHotkey(this);
    m_tray = new SystemTray(this);

    QDBusConnection::sessionBus().registerService("com.zgyd.Reader");
    QDBusConnection::sessionBus().registerObject("/Reader", this, QDBusConnection::ExportScriptableSlots);

    m_tray->setParentWindow(this);

    connect(m_tray, &SystemTray::requestShowWindow, this, &MainWindow::onRestoreFromTray);
    connect(m_tray, &SystemTray::requestQuit, this, &MainWindow::onQuit);
    connect(m_hideHotkey, &GlobalHotkey::activated, this, &MainWindow::onHideToTray);

    connect(m_textReader, &TextReader::fileLoaded, this, &MainWindow::onFileLoaded);
    connect(m_textReader, &TextReader::loadError, this, &MainWindow::onLoadError);

    registerShortcuts();
    updateAppearance();

    connect(m_settings, &SettingsManager::settingsChanged,
            this, &MainWindow::onSettingsChanged);

    QString lastFile = m_settings->getLastFile();
    if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
        m_textReader->loadFile(lastFile);
        m_currentChapter = m_settings->getLastChapter();
    }

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
    m_textBrowser->installEventFilter(this);
    m_textBrowser->viewport()->installEventFilter(this);
    connect(m_textBrowser->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(onTextBrowserScroll(int)));
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
    menuBar()->setAutoFillBackground(true);
    menuBar()->setStyleSheet(
        "QMenuBar {"
        "  background-color: palette(window);"
        "  color: palette(window-text);"
        "}"
        "QMenuBar::item {"
        "  background-color: palette(window);"
        "  color: palette(window-text);"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: palette(highlight);"
        "  color: palette(highlighted-text);"
        "}"
        "QMenu {"
        "  background-color: palette(window);"
        "  color: palette(window-text);"
        "}"
        "QMenu::item:selected {"
        "  background-color: palette(highlight);"
        "  color: palette(highlighted-text);"
        "}"
    );

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
    m_minimizeButton = nullptr;
    m_closeButton = nullptr;
}

void MainWindow::onMinimizeButtonClicked()
{
    if (m_isTransparent) {
        exitTransparentMode();
    }

    setWindowState((windowState() & ~Qt::WindowActive) | Qt::WindowMinimized);
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
    m_shortcuts->registerShortcut(this, "Quit", m_settings->getShortcut("Quit"));
    m_shortcuts->registerShortcut(this, "NextChapter", m_settings->getShortcut("NextChapter"));
    m_shortcuts->registerShortcut(this, "PrevChapter", m_settings->getShortcut("PrevChapter"));
    updateHideWindowShortcut();

    connect(m_shortcuts, &ShortcutsManager::shortcutActivated,
            this, &MainWindow::onShortcutActivated,
            Qt::UniqueConnection);
}

void MainWindow::updateHideWindowShortcut()
{
    if (!m_hideHotkey || !m_shortcuts) {
        return;
    }

    const QString sequence = m_settings->getShortcut("HideWindow");
    m_hideHotkey->setActivationCommand("/usr/bin/qdbus com.zgyd.Reader /Reader com.zgyd.Reader.ToggleVisibility");
    if (m_hideHotkey->isAvailable()) {
        m_shortcuts->registerShortcut(this, "HideWindow", QString());
        m_hideHotkey->setShortcut(sequence);
        return;
    }

    m_hideHotkey->setShortcut(QString());
    m_shortcuts->registerShortcut(this, "HideWindow", sequence);
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
    menuBar()->setAutoFillBackground(true);
    
    if (opacity < 100) {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::transparent);
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet("MainWindow { background: transparent; }");
    } else {
        setAttribute(Qt::WA_TranslucentBackground, false);
        setStyleSheet("");
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
        updateHideWindowShortcut();
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
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    
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
    
    setStyleSheet("MainWindow { background: transparent; }");
    show();
}

void MainWindow::exitTransparentMode()
{
    m_isTransparent = false;
    setAttribute(Qt::WA_TranslucentBackground, false);
    setWindowFlags(Qt::Window);
    
    menuBar()->show();
    menuBar()->setStyleSheet("");
    QToolBar* toolbar = findChild<QToolBar*>("mainToolbar");
    if (toolbar) toolbar->show();
    statusBar()->show();
    m_chapterList->show();
    
    setStyleSheet("");
    updateAppearance();
    show();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_textBrowser || watched == m_textBrowser->viewport()) &&
        event->type() == QEvent::Wheel &&
        m_isTransparent) {
        scrollTextBrowser(static_cast<QWheelEvent*>(event));
        return true;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (m_isTransparent && event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
        return;
    }

    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isTransparent && (event->buttons() & Qt::LeftButton) && m_isDragging) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
        return;
    }

    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isTransparent && event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
        return;
    }

    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (m_isTransparent && m_textBrowser) {
        scrollTextBrowser(event);
        return;
    }

    QMainWindow::wheelEvent(event);
}

void MainWindow::ToggleVisibility()
{
    onHideToTray();
}

void MainWindow::onHideToTray()
{
    if (isHidden()) {
        onRestoreFromTray();
    } else {
        if (m_isTransparent) {
            exitTransparentMode();
        }
        hide();
        m_tray->show();
        m_tray->showMessage("阅读器已隐藏", "点击托盘图标恢复");
    }
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
    scrollToChapter(index);
}

void MainWindow::scrollToChapter(int index)
{
    const QList<Chapter> chapters = m_textReader->getChapters();
    if (index < 0 || index >= chapters.size() || !m_textBrowser->document()) {
        return;
    }

    const Chapter& chapter = chapters.at(index);
    QTextBlock block = m_textBrowser->document()->findBlockByLineNumber(chapter.startLine);
    if (!block.isValid()) {
        return;
    }

    m_isChangingChapter = true;
    m_currentChapter = index;

    QTextCursor cursor(block);
    m_textBrowser->setTextCursor(cursor);
    if (QAbstractTextDocumentLayout* layout = m_textBrowser->document()->documentLayout()) {
        const QRectF blockRect = layout->blockBoundingRect(block);
        m_textBrowser->verticalScrollBar()->setValue(static_cast<int>(blockRect.top()));
    } else {
        m_textBrowser->ensureCursorVisible();
    }
    m_chapterList->setCurrentRow(index);

    m_settings->setLastChapter(index);
    m_settings->setLastScrollPos(m_textBrowser->verticalScrollBar()->value());
    m_settings->sync();

    statusBar()->showMessage(QString("第 %1 章").arg(index + 1));
    m_isChangingChapter = false;
}

void MainWindow::scrollTextBrowser(QWheelEvent *event)
{
    if (!m_textBrowser) {
        QMainWindow::wheelEvent(event);
        return;
    }

    QScrollBar* scrollBar = m_textBrowser->verticalScrollBar();
    if (!scrollBar) {
        event->ignore();
        return;
    }

    int deltaY = event->pixelDelta().y();
    if (deltaY == 0) {
        deltaY = (event->angleDelta().y() / 120) * scrollBar->singleStep() * 3;
    }

    if (deltaY == 0) {
        event->ignore();
        return;
    }

    const int previousValue = scrollBar->value();
    const int nextValue = qBound(scrollBar->minimum(),
                                 previousValue - deltaY,
                                 scrollBar->maximum());

    if (nextValue != previousValue) {
        scrollBar->setValue(nextValue);
    }

    event->accept();
}

void MainWindow::onTextBrowserScroll(int value)
{
    if (m_isChangingChapter) {
        return;
    }

    m_settings->setLastScrollPos(value);

    const QTextCursor cursor = m_textBrowser->cursorForPosition(QPoint(0, 0));
    const int visibleLine = cursor.block().blockNumber();
    const int chapterIndex = chapterIndexForLine(visibleLine);
    if (chapterIndex >= 0 && chapterIndex != m_currentChapter) {
        m_currentChapter = chapterIndex;
        m_chapterList->setCurrentRow(chapterIndex);
        m_settings->setLastChapter(chapterIndex);
        statusBar()->showMessage(QString("第 %1 章").arg(chapterIndex + 1));
    }
}

void MainWindow::onNextChapter()
{
    QList<Chapter> chapters = m_textReader->getChapters();
    if (m_currentChapter < chapters.size() - 1) {
        scrollToChapter(m_currentChapter + 1);
    }
}

void MainWindow::onPrevChapter()
{
    if (m_currentChapter > 0) {
        scrollToChapter(m_currentChapter - 1);
    }
}

void MainWindow::onFileLoaded(const QString& fileName)
{
    setWindowTitle(fileName + " - 阅读器");
    updateChapterList();
    m_textBrowser->setPlainText(m_textReader->getContent());

    m_settings->setLastFile(fileName);

    int lastChapter = m_settings->getLastChapter();
    int lastScrollPos = m_settings->getLastScrollPos();
    QList<Chapter> chapters = m_textReader->getChapters();
    if (!chapters.isEmpty()) {
        if (lastChapter >= 0 && lastChapter < chapters.size()) {
            m_currentChapter = lastChapter;
        } else {
            m_currentChapter = 0;
        }
        scrollToChapter(m_currentChapter);
        if (lastScrollPos > 0) {
            m_textBrowser->verticalScrollBar()->setValue(lastScrollPos);
        }
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

int MainWindow::chapterIndexForLine(int lineNumber) const
{
    const QList<Chapter> chapters = m_textReader->getChapters();
    for (int i = chapters.size() - 1; i >= 0; --i) {
        if (lineNumber >= chapters.at(i).startLine) {
            return i;
        }
    }

    return chapters.isEmpty() ? -1 : 0;
}

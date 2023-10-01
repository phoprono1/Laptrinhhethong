//mainwindow.cpp
#include "mainwindow.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStringList>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setFixedSize(800, 600); // Đặt kích thước tùy ý ở đây
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    m_processList = new QListWidget(centralWidget);
    layout->addWidget(m_processList);

    QPushButton *startProcessButton = new QPushButton("Start Process", centralWidget);
    QPushButton *stopProcessButton = new QPushButton("Stop Process", centralWidget);
    QPushButton *saveSessionButton = new QPushButton("Save Session", centralWidget);
    QPushButton *loadSessionButton = new QPushButton("Load Session", centralWidget);

    layout->addWidget(startProcessButton);
    layout->addWidget(stopProcessButton);
    layout->addWidget(saveSessionButton);
    layout->addWidget(loadSessionButton);

    m_currentProcess = nullptr;

    connect(startProcessButton, &QPushButton::clicked, this, &MainWindow::onStartProcessButtonClicked);
    connect(stopProcessButton, &QPushButton::clicked, this, &MainWindow::onStopProcessButtonClicked);
    connect(saveSessionButton, &QPushButton::clicked, this, &MainWindow::onSaveSessionButtonClicked);
    connect(loadSessionButton, &QPushButton::clicked, this, &MainWindow::onLoadSessionButtonClicked);
}

MainWindow::~MainWindow()
{
    for (const QList<QProcess*> &sessionProcesses : m_sessionProcesses)
    {
        for (QProcess *process : sessionProcesses)
        {
            process->terminate();
            process->waitForFinished();
            delete process;
        }
    }
}

void MainWindow::onStartProcessButtonClicked()
{
    QString command = QInputDialog::getText(this, "Enter Command", "Enter the command to execute:");
    if (!command.isEmpty())
    {
        QString sessionKey = "SomeUniqueSessionKey"; // Đặt một khóa duy nhất cho từng session
        createProcess(command, sessionKey);
    }
}

void MainWindow::onStopProcessButtonClicked()
{
    // Dừng tất cả các tiến trình trong nhóm cho từng session
    for (const QList<QProcess*> &sessionProcesses : m_sessionProcesses)
    {
        for (QProcess *process : sessionProcesses)
        {
            process->terminate();
            process->waitForFinished();
        }
    }
    m_sessionProcesses.clear();

    // Cập nhật danh sách tiến trình sau khi dừng
    updateProcessList();
}

void MainWindow::onSaveSessionButtonClicked()
{
    QString sessionName = QFileDialog::getSaveFileName(this, "Save Session", "", "Session Files (*.session)");
    if (!sessionName.isEmpty())
    {
        QString sessionKey = "SomeUniqueSessionKey"; // Đặt một khóa duy nhất cho từng session
        createSession(sessionName, sessionKey);
    }
}

void MainWindow::onLoadSessionButtonClicked()
{
    QString sessionName = QFileDialog::getOpenFileName(this, "Load Session", "", "Session Files (*.session)");
    if (!sessionName.isEmpty())
    {
        QString sessionKey = "SomeUniqueSessionKey"; // Đặt một khóa duy nhất cho từng session
        loadSession(sessionName, sessionKey);
    }
}

void MainWindow::createProcess(const QString &command, const QString &sessionKey)
{
    QProcess *newProcess = new QProcess(this);

    // Sử dụng môi trường hệ thống mặc định
    newProcess->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

    // Khi có kết quả, gửi nó đến hàm processFinished()
    if (newProcess)
    {
        newProcess->start(command);

        // Kết nối tín hiệu readyReadStandardOutput với hàm processFinished
        connect(newProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::processFinished);

        m_sessionProcesses[sessionKey].append(newProcess); // Thêm tiến trình mới vào danh sách
        qDebug() << "Command: " << command;
    }
    else
    {
        qDebug() << "Failed to create QProcess.";
    }
}

void MainWindow::updateProcessList()
{
    m_processList->clear();

    for (const QList<QProcess*> &sessionProcesses : m_sessionProcesses)
    {
        for (QProcess *process : sessionProcesses)
        {
            QString processName = process->program();
            m_processList->addItem(processName);
        }
    }
}

void MainWindow::processFinished()
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process)
    {
        return;
    }

    QByteArray processOutput = process->readAll();
    QString processList = QString::fromUtf8(processOutput);
    qDebug() << "Process output: " << processList;
    m_processList->addItem(processList);
}

void MainWindow::createSession(const QString &sessionName, const QString &sessionKey)
{
    // Lưu danh sách các lệnh đã chạy trong một tệp văn bản
    QFile file(sessionName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        for (QProcess *process : m_sessionProcesses[sessionKey])
        {
            QString command = process->program();
            out << command << "\n";
        }
        file.close();
    }
}

void MainWindow::loadSession(const QString &sessionName, const QString &sessionKey)
{
    // Dừng tất cả các tiến trình trong session trước khi nạp phiên
    for (QProcess *process : m_sessionProcesses[sessionKey])
    {
        process->terminate();
        process->waitForFinished();
    }

    // Đọc danh sách lệnh từ tệp và tạo lại các tiến trình
    QFile file(sessionName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString command = in.readLine();
            createProcess(command, sessionKey); // Tạo tiến trình mới
        }
        file.close();
    }

    // Cập nhật danh sách tiến trình sau khi nạp phiên
    updateProcessList();
}

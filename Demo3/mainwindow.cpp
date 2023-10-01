//mainwindow.cpp
#include "mainwindow.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStringList>
#include <QInputDialog>
#include <unistd.h>
#include <QLabel>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setFixedSize(800, 600); // Đặt kích thước tùy ý ở đây
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    m_sessions = QList<Session>();
    m_sessionComboBox = new QComboBox(centralWidget); // Thay đổi từ QComboBox* m_sessionComboBox
    layout->addWidget(m_sessionComboBox);


    m_processList = new QListWidget(centralWidget);
    layout->addWidget(m_processList);

    // Tạo một nút để tạo phiên mới
    QPushButton *createSessionButton = new QPushButton("Create Session", centralWidget);
    layout->addWidget(createSessionButton);

    // Khi nút được nhấp, gọi onCreateSessionButtonClicked()
    connect(createSessionButton, &QPushButton::clicked, this, &MainWindow::onCreateSessionButtonClicked);

    QPushButton *startProcessButton = new QPushButton("Start Process", centralWidget);
    QPushButton *stopProcessButton = new QPushButton("Stop Process", centralWidget);
    QPushButton *saveSessionButton = new QPushButton("Save Session", centralWidget);
    QPushButton *loadSessionButton = new QPushButton("Load Session", centralWidget);

    // Ví dụ cho PID, PGID, SID
    QLabel *pidLabel = new QLabel("PID: " + QString::number(getpid()), centralWidget);
    QLabel *pgidLabel = new QLabel("PGID: " + QString::number(getpgid(0)), centralWidget);
    QLabel *sidLabel = new QLabel("SID: " + QString::number(getsid(0)), centralWidget);

    layout->addWidget(pidLabel);
    layout->addWidget(pgidLabel);
    layout->addWidget(sidLabel);


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

void MainWindow::displaySelectedSessionName()
{
    int selectedIndex = m_sessionComboBox->currentIndex();
    if (selectedIndex >= 0 && selectedIndex < m_sessions.count())
    {
        QString selectedSessionName = m_sessions[selectedIndex].name;
        qDebug() << "Selected session: " << selectedSessionName;
    }
    else
    {
        qDebug() << "No session selected.";
    }
}


void MainWindow::onCreateSessionButtonClicked() {
    QString sessionName = QInputDialog::getText(this, "Create Session", "Enter session name:");
    if (!sessionName.isEmpty()) {
        Session newSession;
        newSession.name = sessionName;
        m_sessions.append(newSession);
        updateSessionList();
    }
}

void MainWindow::onSessionChanged(int index) {
    if (index >= 0 && index < m_sessions.count()) {
        qDebug() << "Session changed to: " << m_sessions[index].name;
        // Xóa danh sách tiến trình hiện tại trước khi hiển thị danh sách tiến trình mới
        m_processList->clear();

        // Dừng tất cả các tiến trình trong session trước (nếu có)
        for (const QList<QProcess*> &sessionProcesses : m_sessionProcesses)
        {
            for (QProcess *process : sessionProcesses)
            {
                process->terminate();
                process->waitForFinished();
                delete process;
            }
        }

        // Xóa danh sách tiến trình của phiên trước
        m_sessionProcesses.clear();

        // Lưu phiên hiện tại được chọn
        m_currentSessionIndex = index; // Update the current session index

        // Cập nhật danh sách tiến trình sau khi dừng
        updateProcessList();
    }
}

void MainWindow::onStartProcessButtonClicked()
{
    displaySelectedSessionName(); // Hiển thị tên phiên đã chọn

    QString command = QInputDialog::getText(this, "Enter Command", "Enter the command to execute:");
    if (!command.isEmpty())
    {
        int selectedIndex = m_sessionComboBox->currentIndex();
        if (selectedIndex >= 0 && selectedIndex < m_sessions.count())
        {
            QString sessionKey = m_sessions[selectedIndex].name;
            createProcess(command, sessionKey);
        }
        else
        {
            qDebug() << "No session selected.";
        }
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
        // Lấy index của phiên hiện tại từ combo box
        int selectedIndex = m_sessionComboBox->currentIndex();

        if (selectedIndex >= 0 && selectedIndex < m_sessions.count())
        {
            QString sessionKey = m_sessions[selectedIndex].name; // Sử dụng tên phiên hiện tại làm sessionKey
            createSession(sessionName, sessionKey);
        }
        else
        {
            qDebug() << "No session selected.";
        }
    }
}


void MainWindow::onLoadSessionButtonClicked()
{
    QString sessionName = QFileDialog::getOpenFileName(this, "Load Session", "", "Session Files (*.session)");
    if (!sessionName.isEmpty())
    {
        // Sử dụng tên phiên làm sessionKey
        loadSession(sessionName, sessionName);
    }
}

void MainWindow::createProcess(const QString &command, const QString &sessionKey)
{
    QProcess *newProcess = new QProcess(this);

    // Sử dụng môi trường hệ thống mặc định
    newProcess->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

    // Tạo PGID và SID mới cho phiên
    pid_t pgid = getpid();  // Lấy PGID của tiến trình hiện tại
    setsid();  // Tạo một phiên mới và trở thành người dẫn đầu phiên

    // Thiết lập PGID cho tiến trình mới
    setpgid(0, pgid);  // Đặt PGID cho tiến trình hiện tại

    if (newProcess)
    {
        newProcess->start(command);

        // Kết nối tín hiệu readyReadStandardOutput với hàm processFinished
        connect(newProcess, &QProcess::readyReadStandardOutput, [this, newProcess]() {
            QByteArray processOutput = newProcess->readAll();
            QString processList = QString::fromUtf8(processOutput);
            qDebug() << "Process output: " << processList;
            m_processList->addItem(processList);
        });

        m_sessionProcesses[sessionKey].append(newProcess); // Thêm tiến trình mới vào danh sách
        qDebug() << "Command: " << command;
        qDebug() << getpid();
    }
    else
    {
        qDebug() << "Failed to create QProcess.";
    }
}

void MainWindow::updateSessionList()
{
    m_sessionComboBox->clear(); // Xóa danh sách phiên hiện tại

    // Thêm tên các phiên vào danh sách phiên
    for (const Session &session : m_sessions)
    {
        m_sessionComboBox->addItem(session.name);
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

    // Kiểm tra xem tiến trình này thuộc về phiên hiện tại được chọn
    if (m_currentSessionIndex >= 0 && m_currentSessionIndex < m_sessions.count()) {
        m_processList->addItem(processList);
    }
}

void MainWindow::createSession(const QString &sessionName, const QString &sessionKey)
{
    // Đảm bảo rằng tất cả các tiến trình trong phiên hiện tại đã hoàn thành
    for (QProcess *process : m_sessionProcesses[sessionKey])
    {
        process->waitForFinished();
    }

    // Lấy danh sách lệnh và dữ liệu đầu ra từ danh sách tiến trình của phiên
    QStringList commandList;
    QStringList outputList;
    for (QProcess *process : m_sessionProcesses[sessionKey])
    {
        QString command = process->program();
        QByteArray processOutput = process->readAllStandardOutput();
        QString processOutputString = QString::fromUtf8(processOutput);

        commandList.append(command);
        outputList.append(processOutputString);
    }

    // Kiểm tra xem danh sách lệnh không rỗng trước khi lưu
    if (!commandList.isEmpty())
    {
        QFile file(sessionName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            for (int i = 0; i < commandList.size(); ++i)
            {
                out <<commandList[i] << "\n";
            }
            file.close();
        }
    }
    else
    {
        QMessageBox::information(this, "Save Session", "No processes in the current session to save.");
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

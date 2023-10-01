//mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QListWidget>
#include <QVector>
#include <QComboBox>

class Session {
public:
    QString name;
    QList<QProcess*> processes;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QListWidget *m_processList;
    QProcess *m_currentProcess;
    QMap<QString, QList<QProcess*>> m_sessionProcesses; // Sử dụng QMap để lưu trữ danh sách tiến trình cho từng session
    QList<Session> m_sessions;
    QComboBox* m_sessionComboBox; // Thêm biến m_sessionComboBox vào lớp
    int m_currentSessionIndex = -1; // Khai báo biến m_currentSessionIndex

private slots:
    void onStartProcessButtonClicked();
    void onStopProcessButtonClicked();
    void onSaveSessionButtonClicked();
    void onLoadSessionButtonClicked();
    void onCreateSessionButtonClicked();
    void onSessionChanged(int index);
    void displaySelectedSessionName();
    void createProcess(const QString &sessionName, const QString &sessionKey);
    void updateProcessList();
    void updateSessionList();
    void processFinished();
    void createSession(const QString &sessionName, const QString &sessionKey);
    void loadSession(const QString &sessionName, const QString &sessionKey);
};

#endif // MAINWINDOW_H

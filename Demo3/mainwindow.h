//mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QListWidget>
#include <QVector>

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

private slots:
    void onStartProcessButtonClicked();
    void onStopProcessButtonClicked();
    void onSaveSessionButtonClicked();
    void onLoadSessionButtonClicked();
    void createProcess(const QString &sessionName, const QString &sessionKey);
    void updateProcessList();
    void processFinished();
    void createSession(const QString &sessionName, const QString &sessionKey);
    void loadSession(const QString &sessionName, const QString &sessionKey);
};

#endif // MAINWINDOW_H

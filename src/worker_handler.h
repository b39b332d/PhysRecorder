#pragma once

#include <QObject>
#include <atomic>
#include <functional>
#include <QThread>
void setCursorBusy(bool busy);
class Worker : public QObject {
    Q_OBJECT

        QObject* context;
public:
    Worker(QObject* context);

    // Function to run a task in the worker thread
    void run_in_thread(const std::function<void()>& task);
    Q_SIGNAL void process_finished();

    void run_with_call_back(const std::function<void()>& run_in_thread, const std::function<void()>& call_back);
    void wait();
private:
    std::atomic<bool> m_running;
    QThread process_thread;
};
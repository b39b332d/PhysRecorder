#include <worker_handler.h>
#include <QApplication>
Worker::Worker(QObject *context) : m_running(false), context(context)
{
    this->moveToThread(&process_thread);
    process_thread.start();
}
void Worker::run_in_thread(const std::function<void()>& task) {

    task();
    m_running = false;
    m_running.notify_one();
    emit process_finished();
}
void Worker::run_with_call_back(const std::function<void()>& run_in_thread, const std::function<void()>& call_back = []() {}) {
    bool d = false;
    while (!m_running.compare_exchange_strong(d, true)) {
        m_running.wait(true);
    }
    connect(this, &Worker::process_finished,
        context, call_back, Qt::SingleShotConnection);
    QMetaObject::invokeMethod(this,
        &Worker::run_in_thread, Qt::QueuedConnection, run_in_thread);
}

void Worker::wait()
{
    m_running.wait(true);
}


void setCursorBusy(bool busy) {
    static int busy_count = 0;
    if (busy) {
        busy_count++;
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    else {
        busy_count--;
        if (busy_count <= 0) {
            QApplication::setOverrideCursor(Qt::ArrowCursor);
        }
    }
}

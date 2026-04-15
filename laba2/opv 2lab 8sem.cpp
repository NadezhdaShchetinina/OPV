#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <random>
#include <vector>
#include <map>
#include <sstream>

using namespace std;

class TaskQueue {
private:
    queue<int> tasks;
    mutex mtx;
    condition_variable cv;
    bool done = false;

public:
    void push(int task) {
        lock_guard<mutex> lock(mtx);
        tasks.push(task);
        cv.notify_one();
    }

    bool pop(int& task) {
        unique_lock<mutex> lock(mtx);

        cout << "Поток " << this_thread::get_id() << " ожидает задачу...\n";
        lock.unlock();
        lock.lock();

        cv.wait(lock, [this] { return !tasks.empty() || done; });

        if (tasks.empty() && done) {
            return false;
        }

        task = tasks.front();
        tasks.pop();
        return true;
    }

    void setDone() {
        lock_guard<mutex> lock(mtx);
        done = true;
        cv.notify_all();
    }
};


recursive_mutex logMtx;

void logMessage(const string& msg) {
    lock_guard<recursive_mutex> lock(logMtx);
    cout << msg << endl;
}


void producer(TaskQueue& queue) {
    logMessage("Поток " + [](const thread::id& id) {
        stringstream ss; ss << id; return ss.str();
        }(this_thread::get_id()) + " начал работу");

    for (int i = 1; i <= 20; ++i) {
        {
            lock_guard<recursive_mutex> lock(logMtx);
            cout << "Сгенерирована задача: " << i << endl;
        }
        queue.push(i);
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    queue.setDone();
    logMessage("Поток завершил работу");
}

void consumer(TaskQueue& queue, map<thread::id, int>& stats, mutex& statsMtx) {

    thread_local int processedCount = 0;

    thread::id threadId = this_thread::get_id();
    logMessage("Поток " + [](const thread::id& id) {
        stringstream ss; ss << id; return ss.str();
        }(threadId)+" начал работу");

    int task;
    while (queue.pop(task)) {

        int result = task * task;

        {
            lock_guard<recursive_mutex> lock(logMtx);
            cout << "Поток " << threadId
                << " обработал задачу " << task
                << " -> квадрат = " << result << endl;
        }

        processedCount++;
        this_thread::sleep_for(chrono::milliseconds(150));
    }

    {
        lock_guard<mutex> lock(statsMtx);
        stats[threadId] = processedCount;
    }
}


int main() {
    setlocale(LC_ALL, "Russian");


    TaskQueue queue;


    map<thread::id, int> consumerStats;
    mutex statsMtx;

    thread producerThread(producer, ref(queue));

    vector<thread> consumers;
    for (int i = 0; i < 3; ++i) {
        consumers.emplace_back(consumer, ref(queue),
            ref(consumerStats), ref(statsMtx));
    }


    producerThread.join();

    for (auto& consumer : consumers) {
        consumer.join();
    }

    cout << "\n=== Статистика обработки ===\n";
    int totalProcessed = 0;
    for (const auto& [threadId, count] : consumerStats) {
        cout << "Поток " << threadId << " обработал " << count << " задач\n";
        totalProcessed += count;
    }
    cout << "Всего обработано задач: " << totalProcessed << endl;

    return 0;
}
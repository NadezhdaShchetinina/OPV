#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <atomic>
#include <chrono>
#include <random>
#include <mutex>
#include <iomanip>

using namespace std;

// Вспомогательная функция для разделителей
void printSeparator(char ch = '-', int len = 70) {
    cout << string(len, ch) << endl;
}

// Вычислительная функция 
long long calculateFactorial(int n) {
    if (n < 0) return -1;
    if (n <= 1) return 1;

    long long result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    return result;
}

// Структура для хранения информации о задаче
struct TaskInfo {
    int id;
    int input;
    future<long long> future_result;
    atomic<bool> completed{ false };
    long long result = 0;
};


// Функция выполнения задачи 
void executeTask(packaged_task<long long()> task, atomic<bool>& completed) {
    task();                     // выполнение
    completed.store(true, memory_order_release);
}

// Поток-монитор 
void monitorThreadFunction(promise<void>&& completion_promise,
    const vector<TaskInfo>& tasks) {
    cout << "[МОНИТОР] Начинаю отслеживание завершения задач..." << endl;
    bool all_completed = false;
    while (!all_completed) {
        all_completed = true;
        for (const auto& task : tasks) {
            if (!task.completed.load(memory_order_acquire)) {
                all_completed = false;
                break;
            }
        }
        if (!all_completed) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    }
    cout << "[МОНИТОР] Все задачи завершены. Отправляю сигнал в основной поток." << endl;
    completion_promise.set_value();
}

int main() {
    setlocale(LC_ALL, "Russian");
    system("chcp 1251 > nul");
    auto start_time = chrono::steady_clock::now();

    printSeparator('=', 70);
    cout << "     СИСТЕМА АСИНХРОННОЙ ОБРАБОТКИ ЗАДАЧ (ФАКТОРИАЛ)" << endl;
    printSeparator('=', 70);
    cout << endl;

    const int N = 5;
    vector<TaskInfo> tasks(N);

    // Генератор случайных чисел от 5 до 15
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(5, 15);

    cout << " Создаю " << N << " задач на вычисление факториала..." << endl;
    cout << endl;

    vector<thread> worker_threads;

    // ----- 1. Запуск задач с использованием packaged_task -----
    for (int i = 0; i < N; ++i) {
        int input = dis(gen);
        tasks[i].id = i;
        tasks[i].input = input;

        packaged_task<long long()> task(
            [input]() { return calculateFactorial(input); }
        );
        tasks[i].future_result = task.get_future();

        worker_threads.emplace_back(executeTask, move(task), ref(tasks[i].completed));

        cout << "  [ЗАПУСК] Задача " << i << ": вычисление " << input << "!" << endl;
    }
    cout << endl;

    // ----- 2. Promise и future для монитора -----
    promise<void> completion_promise;
    future<void> completion_future = completion_promise.get_future();
    thread monitor_thread(monitorThreadFunction,
        move(completion_promise),
        cref(tasks));

    // ----- 3. Основной поток собирает результаты через future.get() -----
    cout << "[ОСНОВНОЙ ПОТОК] Начинаю сбор результатов" << endl;
    printSeparator('-', 70);
    cout << endl;

    int completed_count = 0;
    for (int i = 0; i < N; ++i) {
        cout << "  >> Ожидание результата задачи " << i << " ..." << endl;

        auto task_start = chrono::steady_clock::now();
        try {
            tasks[i].result = tasks[i].future_result.get();   // блокировка
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now() - task_start).count();

            completed_count++;
            cout << "  << Задача " << i << " завершена. "
                << tasks[i].input << "! = " << tasks[i].result
                << " (ожидание " << elapsed << " мс)" << endl;
            cout << "     Прогресс: " << completed_count << " из " << N << " задач обработано." << endl;
        }
        catch (const exception& e) {
            cout << "  !! Ошибка в задаче " << i << ": " << e.what() << endl;
        }
        cout << endl;
    }

    // ----- 4. Ожидание сигнала от монитора через future -----
    cout << "[ОСНОВНОЙ ПОТОК] Ожидание сигнала от монитора" << endl;
    completion_future.wait();   // блокировка до set_value()
    cout << "[ОСНОВНОЙ ПОТОК] Сигнал от монитора получен. Все задачи действительно завершены." << endl;
    cout << endl;

    // ----- 5. Завершение потоков -----
    for (auto& t : worker_threads) {
        if (t.joinable()) t.join();
    }
    if (monitor_thread.joinable()) monitor_thread.join();

    // ----- 6. Финальная сводка -----
    auto total_time = chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - start_time).count();
    printSeparator('=', 70);
    cout << "                      ИТОГОВАЯ СВОДКА" << endl;
    printSeparator('=', 70);
    for (const auto& task : tasks) {
        cout << "  Задача " << task.id << ": " << task.input << "! = "
            << setw(12) << task.result << endl;
    }
    printSeparator('-', 70);
    cout << "  Общее время выполнения программы: " << total_time << " мс" << endl;
    printSeparator('=', 70);
    cout << "Программа успешно завершена." << endl;

    return 0;
}
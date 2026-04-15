#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <chrono>
#include <iomanip>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]() {
                            return this->stop || !this->tasks.empty();
                            });

                        if (this->stop && this->tasks.empty()) {
                            return;
                        }

                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result_t<F, Args...>> {

        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();
        return result;
    }

    ~ThreadPool() {
        stop = true;
        condition.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    size_t size() const {
        return workers.size();
    }
};

// Функция для вычисления чисел Фибоначчи
long long fibonacci(int n) {
    if (n <= 1) return n;
    long long a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        long long temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// Вспомогательная функция для разделителей
void printSeparator(char ch = '-', int len = 70) {
    std::cout << std::string(len, ch) << std::endl;
}

int main() {
    setlocale(LC_ALL, "Russian");
    system("chcp 1251 > nul");
    auto start_time = std::chrono::steady_clock::now();

    try {
        printSeparator('=', 70);
        std::cout << "           ПУЛ ПОТОКОВ И ВЫЧИСЛЕНИЕ ЧИСЕЛ ФИБОНАЧЧИ" << std::endl;
        printSeparator('=', 70);

        const size_t pool_size = 4;
        std::cout << "\n[ИНИЦИАЛИЗАЦИЯ] Создаём пул из " << pool_size << " потоков." << std::endl;
        ThreadPool pool(pool_size);
        std::cout << "[ИНИЦИАЛИЗАЦИЯ] Пул успешно создан." << std::endl;

        const int num_tasks = 20;
        std::vector<std::future<long long>> results;
        results.reserve(num_tasks);

        std::cout << "\n--- Добавление задач в пул ---" << std::endl;
        printSeparator('-', 70);

        for (int i = 0; i < num_tasks; ++i) {
            int n = 10 + i;
            results.emplace_back(pool.enqueue(fibonacci, n));
            std::cout << "  Задача " << std::setw(2) << (i + 1) << ": Fibonacci(" << n << ") добавлена." << std::endl;
        }

        std::cout << "\n--- Получение результатов (блокирующий get) ---" << std::endl;
        printSeparator('-', 70);
        std::cout << "  №   |   n   |       Fibonacci(n)" << std::endl;
        printSeparator('-', 70);

        for (size_t i = 0; i < results.size(); ++i) {
            int n = 10 + static_cast<int>(i);
            long long result = results[i].get();   // блокировка
            std::cout << "  " << std::setw(2) << (i + 1) << "   |   "
                << std::setw(2) << n << "   |   "
                << std::setw(15) << result << std::endl;
        }

        printSeparator('-', 70);
        std::cout << "[ЗАВЕРШЕНИЕ] Все " << num_tasks << " задач выполнены." << std::endl;

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "\n[СТАТИСТИКА] Время выполнения программы: " << elapsed << " мс." << std::endl;

        printSeparator('=', 70);
        std::cout << "Программа успешно завершена." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "ОШИБКА: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
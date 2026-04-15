#include <iostream>
#include <thread>
#include <chrono>
#include <random>
using namespace std;

void printNumbers(int threadId, int delayMs) {

    for (int i = 1; i <= 5; ++i) {
        std::cout << "Поток " << threadId << ": " << i << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

    }
    std::cout << "Поток " << threadId << " завершил работу" << std::endl;
}

int main() {
    setlocale(LC_ALL, "Russian");
    std::cout << "Главный поток: запускаем потоки..." << std::endl;

    // Создаём потоки с разными задержками
    std::thread t1(printNumbers, 1, 200);
    std::thread t2(printNumbers, 2, 800);

    std::cout << "Главный поток: потоки созданы" << std::endl;

    t1.detach();
    std::cout << "Главный поток: поток 1 отсоединён (detach)  " << std::endl;

    for (int i = 1; i <= 5; ++i) {
        std::cout << "Работа главного потока" << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Главный поток: ожидаем завершения потока 2 (join)  " << std::endl;
    t2.join();

    std::cout << "Главный поток: дополнительная задержка  " << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "Главный поток: программа завершается" << std::endl;

    return 0;
}
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
using namespace std;

void printNumbers(int threadId) {
    // Генератор случайных задержек
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(200, 300);

    for (int i = 1; i <= 5; ++i) {
        std::cout << "Поток " << threadId << ": " << i << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

    }

}

int main() {
    setlocale(LC_ALL, "Russian");
    std::cout << "Главный поток: запускаем потоки..." << std::endl;

    std::thread t1(printNumbers, 1);
    std::thread t2(printNumbers, 2);

    std::cout << "Главный поток: потоки созданы, ожидаем их завершения..." << std::endl;

    for (int i = 1; i <= 3; ++i) {
        std::cout << "Главный поток: сообщение " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

    }

    t1.join();
    t2.join();

    std::cout << "Главный поток: все потоки завершены. Программа завершается." << std::endl;

    return 0;

}
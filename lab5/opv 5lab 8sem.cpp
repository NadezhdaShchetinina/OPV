#include <omp.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <ctime>

using namespace std;
using namespace chrono;

int main() {
    setlocale(LC_ALL, "rus");

    const long long ARRAY_SIZE = 10000000;

    cout << "Размер массива: " << ARRAY_SIZE << " элементов" << endl;

    int threadCount;
    cout << "Укажите число потоков: ";
    cin >> threadCount;
    omp_set_num_threads(threadCount);

    cout << "Задействовано потоков: " << threadCount << endl;
    cout << "========================================" << endl;

    vector<double> data(ARRAY_SIZE);

    for (long long idx = 0; idx < ARRAY_SIZE; idx++) {
        data[idx] = idx * 0.5;
    }

    // ПОСЛЕДОВАТЕЛЬНЫЙ РАСЧЁТ
    cout << endl << ">>> ПОСЛЕДОВАТЕЛЬНЫЙ РЕЖИМ <<<" << endl;

    auto startSeq = high_resolution_clock::now();

    double sumSequential = 0.0;
    for (long long idx = 0; idx < ARRAY_SIZE; idx++) {
        sumSequential += data[idx];
    }

    auto endSeq = high_resolution_clock::now();
    auto timeSeq = duration_cast<milliseconds>(endSeq - startSeq);

    cout << "Вычисленная сумма: " << fixed << setprecision(2) << sumSequential << endl;
    cout << "Затраченное время: " << timeSeq.count() << " мс" << endl;

    // ПАРАЛЛЕЛЬНЫЙ РАСЧЁТ
    cout << endl << ">>> ПАРАЛЛЕЛЬНЫЙ РЕЖИМ <<<" << endl;

    auto startPar = high_resolution_clock::now();

    double sumParallel = 0.0;
#pragma omp parallel for reduction(+:sumParallel)
    for (long long idx = 0; idx < ARRAY_SIZE; idx++) {
        sumParallel += data[idx];
    }

    auto endPar = high_resolution_clock::now();
    auto timePar = duration_cast<milliseconds>(endPar - startPar);

    cout << "Вычисленная сумма: " << fixed << setprecision(2) << sumParallel << endl;
    cout << "Затраченное время: " << timePar.count() << " мс" << endl;

    // СРАВНЕНИЕ РЕЗУЛЬТАТОВ
    cout << endl << "=== СРАВНЕНИЕ ===" << endl;
    cout << "Последовательно: " << timeSeq.count() << " мс" << endl;
    cout << "Параллельно:     " << timePar.count() << " мс" << endl;

    if (timePar.count() > 0) {
        double acceleration = (double)timeSeq.count() / timePar.count();
        cout << "Ускорение: " << fixed << setprecision(2) << acceleration << "x" << endl;
    }

    if (sumSequential == sumParallel) {
        cout << "Суммы совпадают!" << endl;
    }
    else {
        cout << "Ошибка: суммы не совпадают!" << endl;
    }

    return 0;
}
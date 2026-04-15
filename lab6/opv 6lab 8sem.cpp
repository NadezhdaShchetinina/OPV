#include <iostream>
#include <complex>
#include <chrono>
#include <vector>
#include <opencv2/opencv.hpp>
#include <omp.h>

class MandelbrotGenerator {
private:
    const int width;
    const int height;
    const int max_iter;
    const double x_min;
    const double x_max;
    const double y_min;
    const double y_max;

    int computeIterations(double x, double y) const {
        std::complex<double> c(x, y);
        std::complex<double> z(0, 0);
        int iter = 0;

        while (std::abs(z) <= 2.0 && iter < max_iter) {
            z = z * z + c;
            iter++;
        }

        return iter;
    }

    void pixelToComplex(int x, int y, double& cx, double& cy) const {
        cx = x_min + (double)x / width * (x_max - x_min);
        cy = y_min + (double)y / height * (y_max - y_min);
    }

    cv::Vec3b getColor(int iter) const {
        if (iter == max_iter) {
            return cv::Vec3b(0, 0, 0);   
        }

        float t = (float)iter / max_iter;  

        float hue = 240.0f * (1.0f - t); 
        float saturation = 1.0f;
        float value = 1.0f;

        // Преобразование HSV -> RGB
        int hi = (int)(hue / 60) % 6;
        float f = hue / 60 - hi;
        float p = value * (1 - saturation);
        float q = value * (1 - f * saturation);
        float v = value * (1 - (1 - f) * saturation);

        float r, g, b;
        switch (hi) {
        case 0: r = value; g = v; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = v; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = v; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
        }

        return cv::Vec3b((int)(b * 255), (int)(g * 255), (int)(r * 255));
    }

public:
    MandelbrotGenerator(int w = 1000, int h = 1000, int max_it = 1000)
        : width(w), height(h), max_iter(max_it),
        x_min(-2.5), x_max(1.0), y_min(-1.5), y_max(1.5) {
    }

    // Последовательная версия
    cv::Mat generateSequential() {
        cv::Mat image(height, width, CV_8UC3);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double cx, cy;
                pixelToComplex(x, y, cx, cy);
                int iter = computeIterations(cx, cy);
                image.at<cv::Vec3b>(y, x) = getColor(iter);
            }
        }

        return image;
    }

    // Параллельная версия с задачами (tasks)
    cv::Mat generateParallelTasks(int num_threads = 4) {
        cv::Mat image(height, width, CV_8UC3);

        omp_set_num_threads(num_threads);

#pragma omp parallel
        {
#pragma omp single
            {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
#pragma omp task
                        {
                            double cx, cy;
                            pixelToComplex(x, y, cx, cy);
                            int iter = computeIterations(cx, cy);
                            image.at<cv::Vec3b>(y, x) = getColor(iter);
                        }
                    }
                }
#pragma omp taskwait
            }
        }

        return image;
    }

    // Параллельная версия с collapse(2)
    cv::Mat generateParallelFor(int num_threads = 4) {
        cv::Mat image(height, width, CV_8UC3);

        omp_set_num_threads(num_threads);

#pragma omp parallel for collapse(2)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double cx, cy;
                pixelToComplex(x, y, cx, cy);
                int iter = computeIterations(cx, cy);
                image.at<cv::Vec3b>(y, x) = getColor(iter);
            }
        }

        return image;
    }

    // Параллельная версия по строкам
    cv::Mat generateParallelRows(int num_threads = 4) {
        cv::Mat image(height, width, CV_8UC3);

        omp_set_num_threads(num_threads);

#pragma omp parallel for
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double cx, cy;
                pixelToComplex(x, y, cx, cy);
                int iter = computeIterations(cx, cy);
                image.at<cv::Vec3b>(y, x) = getColor(iter);
            }
        }

        return image;
    }

    void displayImage(const cv::Mat& image, const std::string& window_name) {
        cv::imshow(window_name, image);
        cv::waitKey(0);
    }

    void saveImage(const cv::Mat& image, const std::string& filename) {
        cv::imwrite(filename, image);
    }

    template<typename Func>
    double measureTime(Func func, const std::string& description) {
        auto start = std::chrono::high_resolution_clock::now();
        cv::Mat result = func();
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << description << ": " << elapsed.count() << " секунд" << std::endl;

        return elapsed.count();
    }
};

int main() {
    setlocale(LC_ALL, "Russian");

    const int width = 1000;
    const int height = 1000;
    const int max_iter = 1000;

    MandelbrotGenerator generator(width, height, max_iter);

    std::cout << "=== Генерация множества Мандельброта ===" << std::endl;
    std::cout << "Размер изображения: " << width << "x" << height << std::endl;
    std::cout << "Максимум итераций: " << max_iter << std::endl;
    std::cout << "Область: x∈[-2.5, 1.0], y∈[-1.5, 1.5]" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    std::cout << "Измерение времени выполнения:" << std::endl;
    double seq_time = generator.measureTime(
        [&]() { return generator.generateSequential(); },
        "Последовательная версия"
    );

    std::cout << std::endl;

    std::vector<int> thread_counts = { 1, 2, 4, 8 };

    for (int threads : thread_counts) {
        std::cout << "--- " << threads << " поток(ов) ---" << std::endl;

        double parallel_time = generator.measureTime(
            [&]() { return generator.generateParallelFor(threads); },
            "Версия parallel for"
        );
        std::cout << "Ускорение (parallel for): " << seq_time / parallel_time << "x" << std::endl;

        double rows_time = generator.measureTime(
            [&]() { return generator.generateParallelRows(threads); },
            "Версия parallel rows"
        );
        std::cout << "Ускорение (parallel rows): " << seq_time / rows_time << "x" << std::endl;

        if (threads == 1) {
            double tasks_time = generator.measureTime(
                [&]() { return generator.generateParallelTasks(threads); },
                "Версия tasks"
            );
            std::cout << "Ускорение (tasks): " << seq_time / tasks_time << "x" << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << "Генерация изображения для отображения..." << std::endl;
    cv::Mat image = generator.generateParallelFor(4);

    generator.saveImage(image, "mandelbrot.png");
    std::cout << "Изображение сохранено как 'mandelbrot.png'" << std::endl;

    std::cout << "Нажмите любую клавишу, чтобы закрыть окно..." << std::endl;
    generator.displayImage(image, "Множество Мандельброта");

    return 0;
}
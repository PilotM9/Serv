#include <QCoreApplication>
#include "server.h"
#include <iostream>
#include <string>
#include <limits>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    quint16 port = 0;

    while (port == 0) {
        std::cout << "Enter a port number (1-65535): ";
        std::string input;
        std::cin >> input;

        // Проверка состояния потока ввода
        if (std::cin.fail()) {
            std::cin.clear(); // Сброс состояния ввода
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка оставшихся данных в буфере
        }

        try {
            int enteredPort = std::stoi(input);
            if (enteredPort > 0 && enteredPort <= 65535) {
                port = static_cast<quint16>(enteredPort);
            } else {
                std::cerr << "Invalid port number. Please try again." << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Invalid input. Please enter a valid number." << std::endl;
        }
    }

    // Создаем сервер и передаем ему порт
    Server server(port);

    std::cout << "Server is running on port " << port << ". Waiting for requests..." << std::endl;

    return a.exec();
}

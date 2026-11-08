#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cctype>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#endif

#define PORT 8888
#define BUFFER_SIZE 1024

#ifdef _WIN32
using SocketType = SOCKET;
static const SocketType INVALID_SOCKET_VALUE = INVALID_SOCKET;
static const int SOCKET_ERROR_VALUE = SOCKET_ERROR;

inline bool initializeSockets() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

inline void cleanupSockets() {
    WSACleanup();
}

inline void closeSocket(SocketType socketHandle) {
    closesocket(socketHandle);
}
#else
using SocketType = int;
static const SocketType INVALID_SOCKET_VALUE = -1;
static const int SOCKET_ERROR_VALUE = -1;

inline bool initializeSockets() {
    return true;
}

inline void cleanupSockets() {
}

inline void closeSocket(SocketType socketHandle) {
    close(socketHandle);
}
#endif

// Помощна функция за съвместимост със стари компилатори (замества std::to_string)
template <typename T>
std::string toString(T val) {
    std::stringstream ss;
    ss << val;
    return ss.str();
}

// Генериране на 4-цифрено число с уникални цифри
std::string generateSecretNumber() {
    std::vector<int> digits;
    for (int i = 0; i < 10; ++i) digits.push_back(i);
    
    std::string secret = "";
    for (int i = 0; i < 4; ++i) {
        int idx = rand() % digits.size();
        if (i == 0 && digits[idx] == 0) {
            i--;
            continue;
        }
        secret += toString(digits[idx]);
        digits.erase(digits.begin() + idx);
    }
    return secret;
}

// Проверка за валидност на въведеното число
bool isValidGuess(const std::string& guess) {
    if (guess.length() != 4) return false;
    for (int i = 0; i < 4; ++i) {
        if (!std::isdigit(static_cast<unsigned char>(guess[i]))) return false;
        for (int j = i + 1; j < 4; ++j) {
            if (guess[i] == guess[j]) return false;
        }
    }
    return true;
}

// Преброяване на бикове и крави
void getBullsAndCows(const std::string& secret, const std::string& guess, int& bulls, int& cows) {
    bulls = 0;
    cows = 0;
    for (int i = 0; i < 4; ++i) {
        if (guess[i] == secret[i]) {
            bulls++;
        } else {
            for (int j = 0; j < 4; ++j) {
                if (guess[i] == secret[j]) {
                    cows++;
                    break;
                }
            }
        }
    }
}

// Режим 1: Игра срещу компютъра
void playSinglePlayer() {
    std::string secret = generateSecretNumber();
    std::string guess;
    int attempts = 0;

    std::cout << "\n=== ИГРА СРЕЩУ КОМПЮТЪРА ===\n";
    std::cout << "Компютърът намисли 4-цифрено число с различни цифри.\n";

    while (true) {
        std::cout << "Въведете вашето предположение (4 различни цифри): ";
        std::cin >> guess;

        if (!isValidGuess(guess)) {
            std::cout << "Невалидно число! Трябва да съдържа точно 4 различни цифри.\n";
            continue;
        }

        attempts++;
        int bulls = 0, cows = 0;
        getBullsAndCows(secret, guess, bulls, cows);

        std::cout << "Результат: " << bulls << " Бика, " << cows << " Крави\n";

        if (bulls == 4) {
            std::cout << "\nЧестито! Познахте числото " << secret << " от " << attempts << " опита!\n";
            break;
        }
    }
}

// Режим 2: Мрежова игра (LAN / P2P)
void playMultiplayer() {
    if (!initializeSockets()) {
        std::cout << "Грешка при инициализация на сокет библиотеката.\n";
        return;
    }

    int mode;
    std::cout << "\n=== МРЕЖОВА ИГРА ===\n";
    std::cout << "1. Бъди Host (Създай игра)\n";
    std::cout << "2. Бъди Client (Свържи се към игра)\n";
    std::cout << "Избор: ";
    std::cin >> mode;

    SocketType sock = INVALID_SOCKET_VALUE;
    bool isHost = (mode == 1);

    if (isHost) {
        SocketType listenSock = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSock == INVALID_SOCKET_VALUE) {
            std::cout << "Грешка при създаване на listening socket.\n";
            cleanupSockets();
            return;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(listenSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
            std::cout << "Грешка при bind() на socket-а.\n";
            closeSocket(listenSock);
            cleanupSockets();
            return;
        }

        if (listen(listenSock, 1) == SOCKET_ERROR_VALUE) {
            std::cout << "Грешка при listen() на socket-а.\n";
            closeSocket(listenSock);
            cleanupSockets();
            return;
        }

        std::cout << "Очакване на втори играч да се свърже на порт " << PORT << "...\n";
        sock = accept(listenSock, nullptr, nullptr);
        std::cout << "Играчът се свърза!\n";
        closeSocket(listenSock);
    } else {
        std::string ipInput;
        std::cout << "Въведете IP адреса на Host-а (за същия компютър въведете 127.0.0.1): ";
        std::cin >> ipInput;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET_VALUE) {
            std::cout << "Грешка при създаване на socket.\n";
            cleanupSockets();
            return;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr(ipInput.c_str());

        if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
            std::cout << "Грешка при свързването към " << ipInput << "\n";
            closeSocket(sock);
            cleanupSockets();
            return;
        }
        std::cout << "Успешно свързване към играта!\n";
    }

    std::string mySecret;
    do {
        std::cout << "Въведете вашето тайно 4-цифрено число, което опонентът да познава: ";
        std::cin >> mySecret;
    } while (!isValidGuess(mySecret));

    std::cout << "\nИграта започва!\n";
    bool myTurn = isHost; // Host-ът започва пръв
    char buffer[BUFFER_SIZE];

    while (true) {
        if (myTurn) {
            std::string myGuess;
            do {
                std::cout << "\nВаш ред е! Въведете предположение: ";
                std::cin >> myGuess;
            } while (!isValidGuess(myGuess));

            send(sock, myGuess.c_str(), static_cast<int>(myGuess.length()), 0);

            int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) break;
            buffer[bytesReceived] = '\0';

            std::cout << "Отговор от опонента: " << buffer << "\n";

            if (std::string(buffer).find("4B") != std::string::npos) {
                std::cout << "\nПОБЕДА! Вие познахте числото на опонента първи!\n";
                break;
            }
            myTurn = false;
        } else {
            std::cout << "\nИзчакване опонентът да направи своето предположение...\n";
            int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) break;
            buffer[bytesReceived] = '\0';

            std::string opponentGuess = buffer;
            int bulls = 0, cows = 0;
            getBullsAndCows(mySecret, opponentGuess, bulls, cows);

            std::string response = toString(bulls) + "B " + toString(cows) + "C";
            send(sock, response.c_str(), static_cast<int>(response.length()), 0);

            std::cout << "Опонентът предполагаше: " << opponentGuess 
                      << " -> Бикове: " << bulls << ", Крави: " << cows << "\n";

            if (bulls == 4) {
                std::cout << "\nОпонентът позна вашето число! Вие загубихте.\n";
                break;
            }
            myTurn = true;
        }
    }

    closeSocket(sock);
    cleanupSockets();
}

int main() {
    srand(time(NULL));
    int choice;
    std::cout << "=== BULLS AND COWS ===" << std::endl;
    std::cout << "1. Play against computer" << std::endl;
    std::cout << "2. Two-player network mode" << std::endl;
    std::cout << "Choice: " << std::flush;
    std::cin >> choice;

    if (choice == 1) {
        playSinglePlayer();
    } else if (choice == 2) {
        playMultiplayer();
    } else {
        std::cout << "Невалиден избор.\n";
    }

    return 0;
}
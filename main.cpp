#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cctype>
#include <cstring>
#include <array>
#include <limits>

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

enum DigitState {
    AVAILABLE,
    ELIMINATED,
    CONFIRMED
};

struct GuessResult {
    std::string guess;
    int bulls;
    int cows;
};

const int UI_WIDTH = 81;
const std::array<int, 4> HISTORY_COLUMN_WIDTHS = {{8, 26, 21, 21}};

std::string repeatText(const std::string& text, int count) {
    std::string result;
    for (int i = 0; i < count; ++i) result += text;
    return result;
}

std::size_t displayWidth(const std::string& text) {
    std::size_t width = 0;
    for (unsigned char character : text) {
        if ((character & 0xC0) != 0x80) ++width;
    }
    return width;
}

void printTitleBar(const std::string& title) {
    int padding = UI_WIDTH - static_cast<int>(displayWidth(title));
    std::cout << repeatText("=", UI_WIDTH) << "\n"
              << std::string(padding / 2, ' ') << title
              << std::string(padding - padding / 2, ' ') << "\n"
              << repeatText("=", UI_WIDTH) << "\n";
}

void renderDigitBorder(const std::string& left, const std::string& separator,
                       const std::string& right) {
    std::cout << left;
    for (int digit = 0; digit < 10; ++digit) {
        std::cout << repeatText("─", 7) << (digit == 9 ? right : separator);
    }
    std::cout << "\n";
}

void clearScreen() {
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD consoleMode;
    if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &consoleMode)) {
        std::system("cls");
    }
#else
    if (isatty(STDOUT_FILENO)) std::system("clear");
#endif
}

void renderDigitTracker(const std::array<DigitState, 10>& digits) {
    renderDigitBorder("┌", "┬", "┐");
    const std::string title = "ЧЕРНОВКА ЗА ЦИФРИ (0-9)";
    int titlePadding = UI_WIDTH - 2 - static_cast<int>(displayWidth(title));
    std::cout << "│" << std::string(titlePadding / 2, ' ') << title
              << std::string(titlePadding - titlePadding / 2, ' ') << "│\n";
    renderDigitBorder("├", "┼", "┤");
    std::cout << "│   0   │   1   │   2   │   3   │   4   │   5   │   6   │   7   │   8   │   9   │\n";
    renderDigitBorder("├", "┼", "┤");
    std::cout << "│";
    for (DigitState state : digits) {
        if (state == ELIMINATED) std::cout << "  [✗]  │";
        else if (state == CONFIRMED) std::cout << "  [✓]  │";
        else std::cout << "  [ ]  │";
    }
    std::cout << "\n";
    renderDigitBorder("└", "┴", "┘");
    std::cout << "Легенда: [ ] свободна | [✓] сигурна | [✗] изключена\n";
}

void renderHistoryTable(const std::vector<GuessResult>& history) {
    const char* topLeft = "┌";
    const char* join = "┬";
    const char* topRight = "┐";
    const char* middleLeft = "├";
    const char* middleJoin = "┼";
    const char* middleRight = "┤";
    const char* bottomLeft = "└";
    const char* bottomJoin = "┴";
    const char* bottomRight = "┘";

    std::cout << topLeft;
    for (std::size_t column = 0; column < HISTORY_COLUMN_WIDTHS.size(); ++column) {
        std::cout << repeatText("─", HISTORY_COLUMN_WIDTHS[column]);
        std::cout << (column + 1 == HISTORY_COLUMN_WIDTHS.size() ? topRight : join);
    }

    const auto printRow = [](const std::array<std::string, 4>& cells, bool centered) {
        std::cout << "\n│";
        for (std::size_t column = 0; column < cells.size(); ++column) {
            std::string cell = cells[column];
            const int width = HISTORY_COLUMN_WIDTHS[column];
            int padding = width - static_cast<int>(displayWidth(cell));
            int leftPadding = centered ? padding / 2 : 0;
            int rightPadding = padding - leftPadding;
            std::cout << std::string(leftPadding, ' ') << cell
                      << std::string(rightPadding, ' ') << "│";
        }
    };

    printRow({{"Ход №", "Предположение", "Бикове", "Крави"}}, true);
    std::cout << "\n" << middleLeft;
    for (std::size_t column = 0; column < HISTORY_COLUMN_WIDTHS.size(); ++column) {
        std::cout << repeatText("─", HISTORY_COLUMN_WIDTHS[column]);
        std::cout << (column + 1 == HISTORY_COLUMN_WIDTHS.size() ? middleRight : middleJoin);
    }

    if (history.empty()) {
        printRow({{"", "Няма направени опити", "", ""}}, true);
    } else {
        for (std::size_t i = 0; i < history.size(); ++i) {
            printRow({{toString(i + 1), history[i].guess,
                       toString(history[i].bulls), toString(history[i].cows)}}, true);
            if (i + 1 < history.size()) {
                std::cout << "\n├";
                for (std::size_t column = 0; column < HISTORY_COLUMN_WIDTHS.size(); ++column) {
                    std::cout << repeatText("─", HISTORY_COLUMN_WIDTHS[column]);
                    std::cout << (column + 1 == HISTORY_COLUMN_WIDTHS.size() ? "┤" : "┼");
                }
            }
        }
    }
    std::cout << "\n" << bottomLeft;
    for (std::size_t column = 0; column < HISTORY_COLUMN_WIDTHS.size(); ++column) {
        std::cout << repeatText("─", HISTORY_COLUMN_WIDTHS[column]);
        std::cout << (column + 1 == HISTORY_COLUMN_WIDTHS.size() ? bottomRight : bottomJoin);
    }
    std::cout << "\n";
}

void renderUI(const std::string& title, const std::vector<GuessResult>& history,
              const std::array<DigitState, 10>& tracker) {
    clearScreen();
    printTitleBar(title);
    std::cout << "\n";
    std::cout << "Познай тайното 4-цифрено число с различни цифри.\n\n";
    renderDigitTracker(tracker);
    std::cout << "\nИСТОРИЯ НА ВАШИТЕ ОПИТИ:\n";
    renderHistoryTable(history);
    std::cout << "\n" << std::string(UI_WIDTH, '-') << "\n";
}

void manageTracker(std::array<DigitState, 10>& tracker) {
    int digit;
    std::cout << "\nВъведи цифра за маркиране (0-9): ";
    if (!(std::cin >> digit)) {
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::cout << "Моля, въведи цифра от 0 до 9.\n";
        return;
    }
    if (digit < 0 || digit > 9) {
        std::cout << "Невалидна цифра!\n";
        return;
    }

    std::cout << "Маркирай цифрата " << digit << ":\n";
    std::cout << "1. Свободна  [ ]\n";
    std::cout << "2. Изключена [✗]\n";
    std::cout << "3. Сигурна   [✓]\n";
    std::cout << "Избор: ";
    int state;
    if (!(std::cin >> state) || state < 1 || state > 3) {
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::cout << "Невалиден избор.\n";
        return;
    }
    tracker[digit] = state == 1 ? AVAILABLE : state == 2 ? ELIMINATED : CONFIRMED;
}

enum class PlayerAction {
    GuessReady,
    TrackerUpdated,
    InputEnded
};

PlayerAction readPlayerAction(const std::string& title, const std::vector<GuessResult>& history,
                              std::array<DigitState, 10>& tracker, std::string& guess) {
    renderUI(title, history, tracker);
    std::cout << "Команди: [1] Направи опит | [2] Маркирай цифра\n";
    std::cout << "Избор: ";
    int command;
    if (!(std::cin >> command)) {
        if (std::cin.eof()) return PlayerAction::InputEnded;
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
        std::cout << "Моля, избери 1 или 2.\n";
        return PlayerAction::TrackerUpdated;
    }

    if (command == 2) {
        manageTracker(tracker);
        return PlayerAction::TrackerUpdated;
    }
    if (command != 1) {
        std::cout << "Невалиден избор.\n";
        return PlayerAction::TrackerUpdated;
    }

    std::cout << "Въведи предположение (4 различни цифри): ";
    if (!(std::cin >> guess)) return PlayerAction::InputEnded;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    if (!isValidGuess(guess)) {
        std::cout << "Невалидно число! Трябва да съдържа точно 4 различни цифри.\n";
        return PlayerAction::TrackerUpdated;
    }
    return PlayerAction::GuessReady;
}

void waitForEnter() {
    std::cout << "\nНатиснете Enter, за да затворите прозореца..." << std::flush;
    std::string line;
    std::getline(std::cin, line);
}

// Режим 1: Игра срещу компютъра
void playSinglePlayer() {
    std::string secret = generateSecretNumber();
    std::array<DigitState, 10> tracker = {};
    std::vector<GuessResult> history;

    while (true) {
        std::string guess;
        PlayerAction action = readPlayerAction("ИГРА СРЕЩУ КОМПЮТЪРА", history, tracker, guess);
        if (action == PlayerAction::InputEnded) return;
        if (action == PlayerAction::TrackerUpdated) continue;

        int bulls = 0, cows = 0;
        getBullsAndCows(secret, guess, bulls, cows);
        history.push_back({guess, bulls, cows});

        if (bulls == 4) {
            renderUI("ИГРА СРЕЩУ КОМПЮТЪРА", history, tracker);
            std::cout << "\nЧестито! Позна числото " << secret << " от " << history.size() << " опита!\n";
            break;
        }
    }
}

// Режим 2: Мрежова игра в локална мрежа
void playMultiplayer() {
    if (!initializeSockets()) {
        std::cout << "Грешка при инициализация на сокет библиотеката.\n";
        return;
    }

    int mode;
    clearScreen();
    printTitleBar("МРЕЖОВА ИГРА (2 ИГРАЧИ)");
    std::cout << "1. Създай игра\n";
    std::cout << "2. Свържи се към игра\n";
    std::cout << "Избор: ";
    std::cin >> mode;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    SocketType sock = INVALID_SOCKET_VALUE;
    bool isHost = (mode == 1);

    if (isHost) {
        SocketType listenSock = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSock == INVALID_SOCKET_VALUE) {
            std::cout << "Грешка при създаване на сокет за връзки.\n";
            cleanupSockets();
            return;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(listenSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
            std::cout << "Грешка при свързване на сокета с мрежовия адрес.\n";
            closeSocket(listenSock);
            cleanupSockets();
            return;
        }

        if (listen(listenSock, 1) == SOCKET_ERROR_VALUE) {
            std::cout << "Грешка при настройване на сокета за входящи връзки.\n";
            closeSocket(listenSock);
            cleanupSockets();
            return;
        }

        std::cout << "Очакване на втори играч на порт " << PORT << "...\n" << std::flush;
        sock = accept(listenSock, nullptr, nullptr);
        std::cout << "Играчът се свърза!\n";
        closeSocket(listenSock);
    } else {
        std::string ipInput;
        std::cout << "Въведете мрежовия адрес на играча, създал играта (за същия компютър въведете 127.0.0.1): ";
        std::cin >> ipInput;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET_VALUE) {
            std::cout << "Грешка при създаване на мрежова връзка.\n";
            cleanupSockets();
            return;
        }

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr(ipInput.c_str());

        if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VALUE) {
            std::cout << "Грешка при свързване с " << ipInput << "\n";
            closeSocket(sock);
            cleanupSockets();
            return;
        }
        std::cout << "Успешно свързване с играта!\n";
    }

    std::string mySecret;
    do {
        std::cout << "Въведете вашето тайно 4-цифрено число, което опонентът да познава: ";
        std::cin >> mySecret;
    } while (!isValidGuess(mySecret));
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    std::cout << "\nИграта започва!\n";
    bool myTurn = isHost; // Играчът, създал играта, започва пръв
    char buffer[BUFFER_SIZE];
    std::array<DigitState, 10> tracker = {};
    std::vector<GuessResult> history;
    const std::string title = "МРЕЖОВА ИГРА СРЕЩУ ОПОНЕНТ";

    while (true) {
        if (myTurn) {
            std::string myGuess;
            PlayerAction action = readPlayerAction(title, history, tracker, myGuess);
            if (action == PlayerAction::InputEnded) break;
            if (action == PlayerAction::TrackerUpdated) continue;

            send(sock, myGuess.c_str(), static_cast<int>(myGuess.length()), 0);

            int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) break;
            buffer[bytesReceived] = '\0';

            int bulls = 0, cows = 0;
            char bullsMarker = '\0', cowsMarker = '\0';
            std::istringstream response(buffer);
            if (!(response >> bulls >> bullsMarker >> cows >> cowsMarker) ||
                bullsMarker != 'B' || cowsMarker != 'C' || bulls < 0 || bulls > 4 ||
                cows < 0 || cows > 4 || bulls + cows > 4) {
                std::cout << "Получен е невалиден отговор от опонента.\n";
                break;
            }
            history.push_back({myGuess, bulls, cows});

            if (bulls == 4) {
                renderUI(title, history, tracker);
                std::cout << "\nПОБЕДА! Позна числото на опонента първи!\n";
                break;
            }
            myTurn = false;
        } else {
            renderUI(title, history, tracker);
            std::cout << "Изчакване опонентът да направи своето предположение...\n";
            int bytesReceived = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytesReceived <= 0) break;
            buffer[bytesReceived] = '\0';

            std::string opponentGuess = buffer;
            int bulls = 0, cows = 0;
            getBullsAndCows(mySecret, opponentGuess, bulls, cows);

            std::string response = toString(bulls) + "B " + toString(cows) + "C";
            send(sock, response.c_str(), static_cast<int>(response.length()), 0);

            if (bulls == 4) {
                renderUI(title, history, tracker);
                std::cout << "\nОпонентът позна твоето число! Загуба.\n";
                break;
            }
            myTurn = true;
        }
    }

    closeSocket(sock);
    cleanupSockets();
    waitForEnter();
}

int main() {
    srand(time(NULL));
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    int choice;
    clearScreen();
    printTitleBar("БИКОВЕ И КРАВИ");
    std::cout << "1. Самостоятелна игра (срещу компютъра)" << std::endl;
    std::cout << "2. Игра за двама (през мрежа)" << std::endl;
    std::cout << "Избор: " << std::flush;
    std::cin >> choice;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    if (choice == 1) {
        playSinglePlayer();
    } else if (choice == 2) {
        playMultiplayer();
    } else {
        std::cout << "Невалиден избор.\n";
    }

    return 0;
}
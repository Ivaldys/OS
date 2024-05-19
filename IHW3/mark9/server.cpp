#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unordered_set>
#include <set>

std::vector<int> database;
std::mutex db_mutex;
std::condition_variable cv;
bool writer_active = false;
std::unordered_set<int> monitors;
std::mutex monitors_mutex;
std::set<int> clients;

void notify_monitors(const std::string &message) {
    std::lock_guard<std::mutex> lock(monitors_mutex);
    for (auto it = monitors.begin(); it != monitors.end();) {
        if (write(*it, message.c_str(), message.length()) < 0) {
            close(*it);
            it = monitors.erase(it);
        } else {
            ++it;
        }
    }
}

void handle_client(int client_socket) {
    {
        std::lock_guard<std::mutex> lock(db_mutex);
        clients.insert(client_socket);
    }

    char buffer[256];
    while (true) {
        bzero(buffer, 256);
        int n = read(client_socket, buffer, 255);
        if (n <= 0) break;

        std::string request(buffer);

        if (request.substr(0, 4) == "READ") {
            std::unique_lock<std::mutex> lock(db_mutex);
            while (writer_active) {
                cv.wait(lock);
            }
            int index = rand() % database.size();
            int value = database[index];
            lock.unlock();
            // Calculate Fibonacci (example, not optimized)
            int fib = 0, a = 0, b = 1;
            for (int i = 0; i < value; ++i) {
                fib = a + b;
                a = b;
                b = fib;
            }
            std::string response = "Reader " + std::to_string(client_socket) + ": index " + std::to_string(index) + ", value " + std::to_string(value) + ", Fibonacci " + std::to_string(fib) + "\n";
            write(client_socket, response.c_str(), response.length());
            notify_monitors(response);
        } else if (request.substr(0, 5) == "WRITE") {
            std::unique_lock<std::mutex> lock(db_mutex);
            writer_active = true;
            int index = rand() % database.size();
            int old_value = database[index];
            int new_value = rand() % 100 + 1; // Random value between 1 and 100
            database[index] = new_value;
            std::string response = "Writer " + std::to_string(client_socket) + ": index " + std::to_string(index) + ", old value " + std::to_string(old_value) + ", new value " + std::to_string(new_value) + "\n";
            write(client_socket, response.c_str(), response.length());
            notify_monitors(response);
            writer_active = false;
            cv.notify_all();
        } else if (request.substr(0, 6) == "MONITOR") {
            {
                std::lock_guard<std::mutex> lock(monitors_mutex);
                monitors.insert(client_socket);
            }
            std::string response = "Monitor " + std::to_string(client_socket) + " connected\n";
            write(client_socket, response.c_str(), response.length());
        }
    }
    {
        std::lock_guard<std::mutex> lock(db_mutex);
        clients.erase(client_socket);
    }
    {
        std::lock_guard<std::mutex> lock(monitors_mutex);
        monitors.erase(client_socket);
    }
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[1]);
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return 1;
    }

    sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error on binding" << std::endl;
        return 1;
    }

    listen(server_socket, 5);
    std::cout << "Server listening on port " << port << std::endl;

    // Initialize database
    for (int i = 1; i <= 10; ++i) {
        database.push_back(i);
    }

    while (true) {
        sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(server_socket, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            std::cerr << "Error on accept" << std::endl;
            continue;
        }
        std::thread(handle_client, newsockfd).detach();
    }

    close(server_socket);
    return 0;
}

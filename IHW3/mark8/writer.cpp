#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        return 1;
    }
    
    const char* server_ip = argv[1];
    int port = std::stoi(argv[2]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening socket" << std::endl;
        return 1;
    }
    
    sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(port);
    
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error connecting" << std::endl;
        return 1;
    }

    std::string request = "WRITE";
    write(sockfd, request.c_str(), request.length());

    char buffer[256];
    bzero(buffer, 256);
    int n = read(sockfd, buffer, 255);
    if (n < 0) {
        std::cerr << "Error reading from socket" << std::endl;
        return 1;
    }
    
    std::cout << buffer << std::endl;

    close(sockfd);
    return 0;
}

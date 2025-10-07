#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <atomic>
#include <csignal>

std::atomic<bool> running(true);

const size_t BUFFER_SIZE = 1024;

void signal_handler(int signal){
    if (signal == SIGINT){
        running = false;
    }
}

int main(int argc, char* argv[]){
    signal(SIGINT, signal_handler);
    if (argc < 2){
        std::cerr << "Not enough arguments" << std::endl;
        return 1;
    }

    std::string mode = argv[1];
    if(mode == "send"){
        // Format should be like this:
        // argv[2] = address ex. 127.0.0.1
        // argv[3] = port ex. 9000
        // argv[4] = message ex. "Hello, World!"
        if(argc != 5){
            std::cerr << "Usage: " << argv[0] << " send <ip> <port> <message>" << std::endl;
            return 1;
        }
        int sock_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET = IPv4, DGRAM = UDP, 0 = Let OS pick protocol
        // sock_fd is the id of the socket
        if (sock_fd < 0){
            std::cerr << "Failed to create socket." << std::endl;
            return 1;
        }

        const char* address = argv[2];
        int port;
        try{
            port = std::stoi(argv[3]);
        } catch (const std::exception& e){
            std::cerr << "Invalid Port number: " << argv[3] << std::endl;
            return 1;
        }
        const char* message = argv[4];
        sockaddr_in dest_address;

        memset(&dest_address, 0, sizeof(dest_address));
        dest_address.sin_family = AF_INET;
        dest_address.sin_port = htons(port);
        inet_pton(AF_INET, address, &dest_address.sin_addr);
        ssize_t bytes_sent = sendto(sock_fd, message, strlen(message), 0, reinterpret_cast<sockaddr*>(&dest_address), sizeof(dest_address));

        if (bytes_sent < 0){
            std::cerr << "Failed to send" << std::endl;
        }
        std::cout << "Waiting for echo." << std::endl;

        char response[BUFFER_SIZE];
        sockaddr_in server_address;
        socklen_t addr_len = sizeof(server_address);

        ssize_t bytes = recvfrom(sock_fd, response, sizeof(response) - 1, 0, reinterpret_cast<sockaddr*>(&server_address), &addr_len);
        if (bytes > 0){
            response[bytes] = '\0';
            std::cout << "Echo Received: " << response << std::endl;
        }

        close(sock_fd);
        std::cout << "Socket Closed" << std::endl;
    }else if(mode == "receive"){
        // Format should be like this:
        // argv[2] = port ex. 9000
        if(argc != 3){
            std::cerr << "Usage: " << argv[0] << " receiver <port>" << std::endl;
            return 1;
        }
        int sock_fd = socket(AF_INET, SOCK_DGRAM, 0); //AF_INET = IPv4, DGRAM = UDP, 0 = Let OS pick protocol
        // sock_fd is the id of the socket
        if (sock_fd < 0){
            std::cerr << "Failed to create socket." << std::endl;
            return 1;
        }
        int port;
        try{
            port = std::stoi(argv[2]);
        } catch (const std::exception& e){
            std::cerr << "Invalid Port number: " << argv[3] << std::endl;
            return 1;
        }
        sockaddr_in address; // Internet format
        memset(&address, 0, sizeof(address));

        address.sin_family = AF_INET;
        address.sin_port = htons(port); // Host TO Network Short - converts port number to network byte order
        address.sin_addr.s_addr = INADDR_ANY; // Any Network interface

        int result = bind(sock_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));

        if (result < 0){
            std::cerr << "Failed to bind socket" << std::endl;
            close(sock_fd);
            return 1;
        }

        std::cout << "Listening on port " << ntohs(address.sin_port) << std::endl; // Network to Host Short. Coverts to readable number

        char buffer[BUFFER_SIZE];
        while (running){

            sockaddr_in sender_address;
            socklen_t sender_len = sizeof(sender_address);

            ssize_t bytes_received = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&sender_address), &sender_len);
            if (bytes_received < 0){
                std::cerr << "Failed to receive. " << std::endl;
            } else{
                buffer[bytes_received] = '\0'; // Null-terminate for string
                std::cout << "Received " << buffer << std::endl;

                char sender_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sender_address.sin_addr, sender_ip, INET_ADDRSTRLEN);
                int sender_port = ntohs(sender_address.sin_port);

                std::cout << "From: " << sender_ip << ":"<< sender_port << std::endl;

                ssize_t bytes_sent = sendto(sock_fd, buffer, bytes_received, 0, reinterpret_cast<sockaddr*>(&sender_address), sender_len);
                if(bytes_sent < 0){
                    std::cerr << "Failed to echo back" << std::endl;
                }
            }
        }
        close(sock_fd);
        std::cout << "Socket Closed" << std::endl;
    } else{
        std::cerr << mode << " not a valid command." << std::endl;
        return 1;
    }






    return 0;
}
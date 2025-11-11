#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <csignal> //Required for signal handling
#include <atomic>
#include "client.hpp"

std::atomic_bool sigint_flag(false);

std::array<uint32_t, 2> resolution_user_prompt()
{
    std::cout << "Please enter width of images to generate: ";
    uint32_t files_width;

    while (!(std::cin >> files_width) || (files_width < 1 || files_width > 7680))
        std::cout << "\nInvalid input. Width must be between 1 and 7680. Try again.";

    std::cout << "\nPlease enter height of images to generate: ";
    uint32_t files_height;

    while (!(std::cin >> files_height) || (files_height < 1 || files_height > 4320))
        std::cout << "\nInvalid input. Height must be between 1 and 4320. Try again.";

    return std::array<uint32_t, 2>{files_width, files_height};
}

void server_handshake(int socket, sockaddr_in server_addr)
{
    Packet initial_pkt = Packet::create_connect_packet();
    std::vector<uint8_t> buff = Packet::serialize_packet(initial_pkt);

    ssize_t initial_message = sendto(socket, reinterpret_cast<char *>(buff.data()), buff.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    std::cout << "Sent initial ACK to server.\n";

    std::vector<uint8_t> buffer;
    buffer.resize(512);
    ssize_t bytes_received = recvfrom(socket, reinterpret_cast<char *>(buffer.data()), buffer.size(), 0, NULL, NULL);

    uint8_t type = buffer.at(0);
    int size = deserialize_size(buffer);

    if (!(type == 0 && size == 0))
        throw std::runtime_error("Unknown ACK from Server.");
}

void signal_handler(int signum)
{
    sigint_flag = true;
    std::cout << "\n\nCaught signal " << signum << " (SIGINT). Sending Server disconnect packet..." << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << argv[0] << "<server_ip> <server_port>\n";
        return 1;
    }
    int server_port = atoi(argv[2]);
    char *server_ip = argv[1];

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 500000; // microseconds (0.5 s)
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("setsockopt");
        return 1;
    }

    // Create server info
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);          // Server port
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // Server IP
    socklen_t addr_len = sizeof(&server_addr);

    server_handshake(sock, server_addr);
    std::cout << "Initial ACK from Server received. IP: " + std::string(server_ip) + ":" + std::to_string(server_port) + "\n";

    // Enable Sigint To terminate server connection gracefully
    std::signal(SIGINT, signal_handler);

    // Prompt user size to generate rgb files
    std::array<uint32_t, 2> resolution = resolution_user_prompt();

    // Create files with prompted resolution
    std::array<File, 3> files_array = files_creator(resolution.at(0), resolution.at(1), "client/files");

    // Prepare sending and receiving buffers
    std::vector<uint8_t> rec_buffer;
    std::vector<uint8_t> send_buffer;
    rec_buffer.resize(512);

    // Send Resolution packet

    Packet res_packet = Packet::create_res_packet();

    const uint8_t *width_ptr = reinterpret_cast<const uint8_t *>(&resolution.at(0));
    res_packet.get_payload().insert(res_packet.get_payload().end(), width_ptr, width_ptr + sizeof(uint32_t)); // width
    const uint8_t *height_ptr = reinterpret_cast<const uint8_t *>(&resolution.at(1));
    res_packet.get_payload().insert(res_packet.get_payload().end(), height_ptr, height_ptr + sizeof(uint32_t)); // height

    send_buffer = Packet::serialize_packet(res_packet);
    std::cout << send_buffer.size() << std::endl;
    ssize_t initial_message = sendto(sock, reinterpret_cast<char *>(send_buffer.data()), send_buffer.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    send_buffer.clear();

    // Wait for Resolution ACK
    ssize_t bytes_read = recvfrom(sock, reinterpret_cast<char *>(rec_buffer.data()), rec_buffer.size(), 0, (struct sockaddr *)&server_addr, &addr_len);
    int res_ack_type = deserialize_type(rec_buffer);
    if (res_ack_type)
        std::cout << "Received Resolution_ACK from Server..\n";
    std::fill(rec_buffer.begin(), rec_buffer.end(), 0);

    // Create file packets
    while (!sigint_flag)
    {
        for (File file : files_array)
        {
            // Starting packet to signal server
            Packet initial_pkt = Packet::create_starting_file_packet(color_to_byte(file.get_color()));
            send_buffer = Packet::serialize_packet(initial_pkt);
            ssize_t initial_message = sendto(sock, reinterpret_cast<char *>(send_buffer.data()), send_buffer.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            send_buffer.clear();

            // Create all packets to send
            std::vector<Packet> file_packets = Packet::create_file_packets(file);

            auto start_time = std::chrono::high_resolution_clock::now();
            int a = 0;
            for (Packet packet : file_packets)
            {
                send_buffer = Packet::serialize_packet(packet);
                sendto(sock, reinterpret_cast<char *>(send_buffer.data()), send_buffer.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                send_buffer.clear();
                usleep(100);
            }
            // Final packet to signal server
            Packet final_pkt = Packet::create_ending_file_packet();
            send_buffer = Packet::serialize_packet(final_pkt);
            ssize_t final_message = sendto(sock, reinterpret_cast<char *>(send_buffer.data()), send_buffer.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

            // Receive Server Ack
            ssize_t bytes_read = recvfrom(sock, reinterpret_cast<char *>(rec_buffer.data()), rec_buffer.size(), 0, (struct sockaddr *)&server_addr, &addr_len);

            // Wait until 1 sec has elapsed from 1st packet
            auto finish_time = std::chrono::high_resolution_clock::now();
            auto duration = finish_time - start_time;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            auto time_dif_to_1sec = 1000000 - ms;
            if (time_dif_to_1sec > 0)
                usleep(time_dif_to_1sec);

            send_buffer.clear();
            std::fill(rec_buffer.begin(), rec_buffer.end(), 0);
        }
    }

    // Send Server Disconnect packet
    Packet disconnect_packet = Packet::create_disconnect_packet();
    send_buffer = Packet::serialize_packet(disconnect_packet);
    ssize_t final_message = sendto(sock, reinterpret_cast<char *>(send_buffer.data()), send_buffer.size(), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    std::cout << "Sent disconnect signal to Server.\n";

    std::cout << "Exiting...\n";
    return 0;
}
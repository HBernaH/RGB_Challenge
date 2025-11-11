#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include "server.hpp"


int main(int argc, char *argv[])
{
    // Assert arguments
    if (argc != 2)
    {
        std::cerr << argv[0] << "<server_port>\n";
        return 1;
    }
    int server_port = atoi(argv[1]);

    // Create socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // Server address setup
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    // Bind socket to address
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Bind failed.\n";
        close(sock);
        return 1;
    }

    std::cout << "UDP Server listening on port " << std::to_string(server_port) << std::endl;

    //Get path for files directory "/server/files/"
    std::string files_path = get_files_directory("server/files");

    File current_file;
    int global_file_number=0;


    // Current client connected to (Prevent reading packets from other clients)
    struct sockaddr_in current_client;
    bool client_authenticated = false;
    int current_width=-1, current_height=-1;

    // Prepare read and write buffers
    std::vector<uint8_t> read_buffer;
    std::vector<uint8_t> write_buffer;
    read_buffer.resize(512);

    // Prepare client details from datagrams received
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);   

    while (true)
    {
        ssize_t bytes_read = recvfrom(sock, reinterpret_cast<char*>(read_buffer.data()), read_buffer.size(), 0, (struct sockaddr *)&client_addr, &addr_len);
        if(bytes_read < 3)
            std::cout << "Incoming packet less than 3 bytes. Datagram dropped.\n";

        // First Client Case
        else if (!client_authenticated)
        {
            current_client = client_addr;
            std::cout << "Client authenticated\n";
            client_authenticated=true;
            uint8_t packet_type = read_buffer[0];
            if (packet_type != 0)
                std::cout << "Error. Client Authenticated with unexpected packet type " << std::to_string(packet_type) << ".\n"; 
            
                //1st Client Connection ACK
                Packet connect_resp_packet = Packet::create_connect_packet();
                write_buffer = Packet::serialize_packet(connect_resp_packet);
                sendto(sock,reinterpret_cast<char*>(write_buffer.data()),write_buffer.size(),0,(struct sockaddr *)&current_client,sizeof(current_client));
        }

        else if (current_client.sin_addr.s_addr == client_addr.sin_addr.s_addr && current_client.sin_port == client_addr.sin_port)
        {
            uint8_t packet_type = read_buffer[0];
            switch(packet_type){
                case 1: //Client Disconnected
                {
                    client_authenticated=false;
                    current_height=-1;
                    current_width=-1;

                    std::cout << "Client with ip: " << current_client.sin_addr.s_addr << " disconnected.. \nOpen to new client.\n";
                    break;
                }
                case 2: //Resolution
                {
                    //Deserialize
                    int size = deserialize_size(read_buffer);

                    std::vector<uint8_t> payload = deserialize_payload(read_buffer,size);

                    uint32_t width;
                    std::memcpy(&width, payload.data(), sizeof(uint32_t));

                    uint32_t height;
                    std::memcpy(&height, payload.data()+ sizeof(uint32_t), sizeof(uint32_t));

                    current_width=width;
                    current_height=height;

                    Packet res_ack_packet = Packet::create_res_packet();
                    write_buffer = Packet::serialize_packet(res_ack_packet);

                    sendto(sock,reinterpret_cast<char*>(write_buffer.data()),write_buffer.size(),0,(struct sockaddr *)&current_client,sizeof(current_client));
                    break;
                }
                case 4: //Write to created File
                {
                    int size = deserialize_size(read_buffer);
                    std::vector<uint8_t> payload = deserialize_payload(read_buffer,size);
                    std::ofstream output_stream(current_file.get_fullpath(), std::ios::binary | std::ios::app);
                    
                    if(output_stream.is_open()){
                        output_stream.write(reinterpret_cast<const char*>(payload.data()), payload.size());
                        output_stream.close();
                    }
                    else{
                        std::cout << "Couldn't write incoming fragment of file " << current_file.get_filename() << std::endl;
                    }
                    break;
                }
                case 5: //New File incoming
                {
                    int size = deserialize_size(read_buffer);

                    std::vector<uint8_t> payload = deserialize_payload(read_buffer,size);

                    //Get color from UDP datagram
                    uint8_t color;
                    std::memcpy(&color, payload.data(), sizeof(uint8_t));

                    //Create new file with global_file_number
                    current_file = File(current_width,current_height,byte_to_color(color),files_path,global_file_number);
                    
                    //Create new empty file in directory
                    std::ofstream outputFile(current_file.get_fullpath(),std::ios::binary);
                    if(!outputFile.is_open()){
                        std::cout << "Failed to write initial starting empty file: " << current_file.get_filename() << std::endl;
                    }
                    outputFile.close();
                    break;
                }
                case 6: //Finished receiving File
                {
                    if(File::check_file_size(current_file))//check if File has expected size
                    {
                    //Send ACK datagram
                    Packet complete_file_ack_packet = Packet::create_file_ack_packet();
                    write_buffer = Packet::serialize_packet(complete_file_ack_packet);
                    sendto(sock,reinterpret_cast<char*>(write_buffer.data()),write_buffer.size(),0,(struct sockaddr *)&current_client,sizeof(current_client));

                    std::cout << "File: " << current_file.get_filename() << " complete.\n";
                    }

                    current_file = File();
                    global_file_number++;
                    break;
                }
                
                default:
                    std::cout << "Unknown packet type received.. Datagram dropped..\n";
                    break;
            }
        }
        else{
            std::cout << "Unauthorized client - dropping datagram..\n";
        }
        
        //Clear read and write buffers
        std::fill(read_buffer.begin(), read_buffer.end(), 0);
        write_buffer.clear();
    }
    return 0;
}
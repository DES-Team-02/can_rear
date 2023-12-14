#include "CanReceiver.hpp"

CanReceiver::CanReceiver() : raw_rpm(0), running(false) {}

CanReceiver::~CanReceiver() {}

int CanReceiver::openPort(const char* iface) {
    struct ifreq ifr;           // Interface request structure for socket ioctls
    struct sockaddr_can addr;   // Address structure for the CAN socket

    // Open a CAN socket
    soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);  
    // Check if socket is created successfully
    if (soc < 0) 
    {
	    std::cout << "Socket Creation Error!" << std::endl;
        return (-1);
    }
    // Set the family type for the address to CAN
    addr.can_family = AF_CAN;  
    // Copy the port name to the ifreq structure
    strcpy(ifr.ifr_name, CAN_INTERFACE);  
    // Fetch the index of the network interface into the ifreq structure using ioctl
    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0) 
    {
	    std::cout << "ioctl Error." << std::endl;
        return (-1);
    }
    // Get the interface index from the ifreq structure
    addr.can_ifindex = ifr.ifr_ifindex;  
    // Set the socket to non-blocking mode
    fcntl(soc, F_SETFL, O_NONBLOCK);  
    // Bind the socket to the CAN interface
    if (bind(soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    {
	    std::cout << "Binding Error." << std::endl;
        return (-1);
    }
    return 0;
}

void CanReceiver::readData() {
    struct can_frame frame;
    // int recvbytes = 0;
    // Set the data length code (DLC)
    // struct timeval timeout = {1, 0};
    frame.can_dlc = 4;      
    while(running) {

        /*
        usleep(100000);  // Sleep for 100ms
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(soc, &readSet);
        if (select((soc + 1), &readSet, NULL, NULL, &timeout) >= 0) {
            // If there is data availible on socket, read the data from the socket.
            if (FD_ISSET(soc, &readSet)) {
                recvbytes = read(soc, &frame, sizeof(struct can_frame));
                if (recvbytes) {
                    switch(frame.can_id){
                        case 0x100: 
                            // read raw data from frame & store
                            int received_raw_rpm = (frame.data[0] << 8) + frame.data[1];
                            std::lock_guard<std::mutex> lock(dataMutex);
                            raw_rpm = received_raw_rpm; 
                    }
                }
            }
        }      
        */

        // receive data
        ssize_t nbytes = recv(soc, &frame, sizeof(struct can_frame), MSG_WAITALL);

        // save timestamp
        if(nbytes == sizeof(struct can_frame)) {
            last_received_time = std::chrono::steady_clock::now();
        }

        // read raw data from frame & store
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            std::memcpy(&raw_rpm, frame.data, sizeof(int));
        }
    }
}

void CanReceiver::processAndFilterData() {

    MovingAverageFilter rpmFilter(10, 5); 
    
    double PI = M_PI;
    double filtered_rpm;
    double filtered_speed;

    while(running) {
        int current_rpm;
        // save current rpm
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            current_rpm = raw_rpm;
        }
        // filtered and calculated rpm and speed
        filtered_rpm = rpmFilter.filter(current_rpm);
        filtered_speed = (((filtered_rpm * FACTOR) / WHEEL_RADIUS) * PI) * WHEEL_RADIUS;
        usleep(100000);
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Received RPM      : " << raw_rpm          << std::endl;
        std::cout << "Filtered RPM      : " << filtered_rpm     << std::endl; 
        std::cout << "Filtered Speed    : " << filtered_speed   << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        // send values to vSOME/IP
        dataRegister.sendDataToVSomeIP(static_cast<uint32_t>(filtered_rpm), static_cast<uint32_t>(filtered_speed));
        // pause for 100ms
        usleep(100000); 
    }
}

void CanReceiver::closePort() {
    close(soc);
}

int CanReceiver::run() {
    if (openPort(CAN_INTERFACE) < 0) {
        return -1;
    }

    running = true;
    last_received_time = std::chrono::steady_clock::now();

    std::thread readerThread(&CanReceiver::readData, this);
    std::thread processorThread(&CanReceiver::processAndFilterData, this);

    readerThread.join();
    processorThread.join();

    closePort();

    return 0;
}

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
    // Get the interface index of the CAN device
    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0) 
    {
	    std::cout << "I/O Control Error." << std::endl;
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
    while(running) {
        // receive data
        ssize_t nbytes = recv(soc, &frame, sizeof(struct can_frame), MSG_WAITALL);
        // save timestamp if frame received
        if(nbytes == sizeof(struct can_frame)) {
            last_received_time = std::chrono::steady_clock::now();
        }
        switch(frame.can_id){
            case 0x100: 
            {
            // read raw data from frame & store
            std::lock_guard<std::mutex> lock(dataMutex);
            int received_raw_rpm = frame.data[0] << 8 | frame.data[1]; 
            raw_rpm = received_raw_rpm;
            }
        }
        // sleep for 100ms
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void CanReceiver::processAndFilterData() {

    MovingAverageFilter rpmFilter(10, 5); 
    
    double PI = M_PI;
    double filtered_rpm;
    double filtered_speed;

    while(running) {
        int current_rpm;
        // buffer current rpm
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            current_rpm = raw_rpm;
        }
        // filtered rpm
        filtered_rpm = rpmFilter.filter(current_rpm);
        // calculated speed (sensor wheel on wheel outer-diameter )
        //filtered_speed = (((filtered_rpm * FACTOR) / WHEEL_RADIUS) * PI) * WHEEL_RADIUS;
        // calculated speed (sensor wheel on wheel shaft )
        filtered_speed = ((filtered_rpm) / (WHEEL_RADIUS * 2 * PI)) / 3;
        
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Received RPM      : " << raw_rpm          << std::endl;
        std::cout << "Filtered RPM      : " << filtered_rpm     << std::endl; 
        std::cout << "Filtered Speed    : " << filtered_speed   << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        // send values to vSOME/IP
        dataRegister.sendDataToVSomeIP(static_cast<uint32_t>(filtered_rpm), static_cast<uint32_t>(filtered_speed));
        // 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
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

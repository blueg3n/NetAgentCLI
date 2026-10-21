#pragma one

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <algorithm>
using namespace std;
namespace fs = filesystem;

vector<string> output = {
  "--------------------",
  "Net Agent Analyser",
  "--------------------",
  "Interface: ",
  "Speed: ",
  "Total Traffic: ",
  "Press Ctrl + C to quit"  
};

struct InterPacket {
    string interface;
    unsigned long long download;
    unsigned long long upload;
    unsigned long long downloadTraffic = 0;
    unsigned long long uploadTraffic = 0;
};

string getActiveInterface() {
    for (const auto& dir : fs::directory_iterator("/sys/class/net") ) {
        string interfaceName = dir.path().filename().string();
        string line;
        ifstream operstate("/sys/class/net/" + interfaceName + "/operstate");
        
        while (getline(operstate, line)) {
            if (line == "up") {
                return interfaceName; 
            }
        }
    }
}


InterPacket getNetInfo() {
    InterPacket pck;
    pck.interface = getActiveInterface();

    ifstream netInfo("/proc/net/dev");
    if (!netInfo.is_open()) {
        cerr << "Error: Can't open file!\n";
    }

    string line;
    string interface;
    string d;
    unsigned long long download, upload;

    // Skips the header
    getline(netInfo, line);
    getline(netInfo, line);

    pck.interface = getActiveInterface();

    while (getline(netInfo, line)) {    
        // Remove the colon from the interface name.
        replace(line.begin(), line.end(), ':', ' ');

        stringstream ss(line);
        ss >> interface >> download;

        for (int i = 0; i < 7; i ++) {
            ss >> d;
        }    

        ss >> upload;

        if (interface == pck.interface) {
            pck.downloadTraffic = download;
            pck.uploadTraffic = upload;
            break;
        }
    }

    return pck;
}

double getByteSpeed(unsigned long long oldBytes, unsigned long long newBytes) {
    // Measure speed in Mbps by comparing old and new values
    return (newBytes - oldBytes) * 8.0 / 1024.0;
}

vector<double> monitorSpeed(InterPacket& pck, ifstream& file) {
    unsigned long long newDownload;
    unsigned long long newUpload;

    string line;
    string interface;
    string d;

    // Skip headers
    getline(file, line);
    getline(file, line);
    while (getline(file, line)) {
        // Find active Interface
        if (line.find(pck.interface) != string::npos) {    

            // Remove collon from interface
            replace(line.begin(), line.end(), ':', ' ');
            stringstream ss(line);

            ss >> interface >> newDownload;
            
            for (int i = 0; i < 7; i ++) {
                ss >> d;
            }

            ss >> newUpload;
            
        }
    }

    double downloadSpeed = getByteSpeed(pck.download, newDownload);
    pck.download = newDownload;

    double uploadSpeed = getByteSpeed(pck.upload, newUpload);
    pck.upload = newUpload;

    return {downloadSpeed, uploadSpeed};    
}

void totalBytes(InterPacket& pck, double& addDown, double& addUp) {
    pck.downloadTraffic += addDown;
    pck.uploadTraffic += addUp;
} 

void runMonitoring(int time = 999, ofstream* outputFile = nullptr) {
    InterPacket pck = getNetInfo();
    cout << output[0] << "\n";
    cout << output[1] << "\n";
    cout << output[2] << "\n";
    cout << output[3] << pck.interface << "\n";

    while (true) {
        this_thread::sleep_for(chrono::milliseconds(time));

        ifstream file("/proc/net/dev");
        vector<double> speed = monitorSpeed(pck, file);
        totalBytes(pck, speed[0], speed[1]);

        cout << "\033[2K\r" << output[4] 
        << speed[0] << " | " << speed[1] << "\n";
        
        cout << "\033[2K\r" << output[5] 
        << pck.downloadTraffic << " | " << pck.uploadTraffic << "\033[1A" << flush;
        
        if (outputFile != nullptr && outputFile->is_open()) {
            *outputFile << "Download: "<< speed[0] << "\n";
            *outputFile << "Upload: "<< speed[1] << "\n";
            outputFile->flush();
        }
        
    }
}


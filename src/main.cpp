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

struct InterPacket {
    string interface;
    unsigned long long rxBytes;
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
    unsigned long long rxBytes;

    // Skips the header
    getline(netInfo, line);
    getline(netInfo, line);

    pck.interface = getActiveInterface();

    while (getline(netInfo, line)) {    
        replace(line.begin(), line.end(), ':', ' ');

        stringstream ss(line);
        ss >> interface >> rxBytes;

        // Remove the colon from the interface name.

        if (interface == pck.interface) {
            pck.rxBytes = rxBytes;
            break;
        }
    }

    return pck;
}

double getByteSpeed(unsigned long long oldBytes, unsigned long long newBytes) {
    return (newBytes - oldBytes) * 8.0 / 1024.0;
}

double monitorSpeed(InterPacket& pck, ifstream& file) {
    unsigned long long oldBytes = pck.rxBytes;
    unsigned long long newBytes;

    string line;
    string interface;

    getline(file, line);
    getline(file, line);
    while (getline(file, line)) {
        if (line.find(pck.interface) != string::npos) {    
            replace(line.begin(), line.end(), ':', ' ');
            stringstream ss(line);
            ss >> interface >> newBytes;
        }
    }

    double speed = getByteSpeed(oldBytes, newBytes);
    pck.rxBytes = newBytes;

    return speed;    
}

int main (int argc, char** argv) {
    InterPacket pck = getNetInfo();
    cout << pck.interface << "\n";
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));

        ifstream file("/proc/net/dev");
        double speed = monitorSpeed(pck, file);
        cout << to_string(speed) << "\n";
    }

    return 0;
};
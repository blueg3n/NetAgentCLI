#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
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
        stringstream ss(line);
        ss >> interface >> rxBytes;

        // Remove the colon from the interface name.
        if (!interface.empty() && interface.back() == ':') {
            interface.pop_back();
        }    

        if (interface == pck.interface) {
            pck.rxBytes = rxBytes;
        }
        
    }

    return pck;
}

int main (int argc, char** argv) {
    cout << getNetInfo().interface << "\n";
    cout << getNetInfo().rxBytes << "\n";
    
    return 0;
};
#include "netagent.h"

int main (int argc, char** argv) {
    ofstream debug("debug.txt", ios::app);
    
    runMonitoring(&debug);
    return 0;
};
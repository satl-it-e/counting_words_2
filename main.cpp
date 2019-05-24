#include <thread>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <list>
#include <functional>
#include <map>

#include "time_measure.h"
#include "config.h"
#include "additional.h"

int main(int argc, char *argv[])
{
    MyConfig mc;
    try{

    } catch(std::string &err)
    {std::cerr << err + "\n Sorry, unpredicted error. We will fix it in the next version." << std::endl;
        return -2;}

    std::cout << "\nOK\n" << std::endl;
    return 0;
}
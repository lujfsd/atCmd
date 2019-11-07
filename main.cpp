#include <iostream>
#include "atCmd.h"

#define AT_DEV_PATH "/dev/ttyUSB1"

void requstCsqLoop()
{
    std::vector<std::string> out;
    std::string cmd("at+csq");
    std::string expectResp("+CSQ:");
    while (1)
    {
        out.clear();
        if (atCmdObj.sendAtCmd(cmd, expectResp, out))
        {
            std::cout << "Request Csq Success:[" << out.front() << "]";
            std::vector<std::string> splitRet;
            boost::algorithm::split(splitRet, out.front(), boost::is_any_of(":,"));
            int rssi = std::stoi(splitRet.at(1));
            int ber = std::stoi(splitRet.at(2));
            std::cout << "RSSI:" << rssi 
                      << " ber:" << ber
                      <<std::endl;
        }
        else
        {
            std::cout << "Request Csq Error:[" << out.front() << "]" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[])
{
    std::string uartDev;
    if (argc != 2)
    {
        std::cout << "use default uartDev:[" << AT_DEV_PATH << "]";
        uartDev = std::string(AT_DEV_PATH);
    }
    else
    {
        uartDev = std::string(argv[1]);
    }

    try
    {
        atCmdObj.initUart(uartDev);
        requstCsqLoop();
    }
    catch (const std::exception &e)
    {
        std::cout  << "Get exception:" << e.what();
    }
    return 0;
}
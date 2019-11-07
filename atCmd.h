/**
 * @file atCmd.h
 * @author 
 * @brief 
 * @version 0.1
 * @date 
 * 
 * @copyright Copyright () 2019
 * 
 */
#ifndef _ATCMD_H
#define _ATCMD_H
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/asio/serial_port.hpp>
#include <boost/serialization/singleton.hpp>

typedef std::vector<std::string> AtResp;

class AtCmd : public boost::serialization::singleton<AtCmd>
{
private:
    AtResp resp;
//用于读串口数据
    std::thread readThread;
    std::condition_variable cv;
    std::mutex cmdMutex;
    std::mutex writeMutex;

    boost::asio::io_context ioContext;
    boost::asio::serial_port uart;

    static const std::vector<std::string> smsUnsoliciteds;
    static const std::vector<std::string> finalResponsesSuccess;
    static const std::vector<std::string> finalResponsesError;

private:
    bool findString(std::string &line, const std::vector<std::string> &testV);
    bool isSMSUnsolicited(std::string &line);
    bool isFinalResponse(std::string &line);
    bool isFinalResponseSuccess(std::string &line);
    bool isFinalResponseError(std::string &line);
    void setUart();
    void initModem();

public:
    AtCmd();
    ~AtCmd();
    bool initUart(std::string &uartDev);
    void readLoop();
    bool sendAtCmd(const char *cmd, AtResp &outResp);
    bool sendAtCmd(const std::string &cmd, AtResp &outResp);
    bool sendAtCmd(const std::string &cmd, const std::string &expectResp, AtResp &outResp);
};

#define atCmdObj AtCmd::get_mutable_instance()

#endif

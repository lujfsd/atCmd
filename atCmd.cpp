/**
 * @file atCmd.cpp
 * @author 
 * @brief 
 * @version 0.1
 * @date 
 * 
 * @copyright Copyright () 2019
 * 
 */
 #include <iostream>
#include <chrono>
#include <condition_variable>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "atCmd.h"

using serial_port = boost::asio::serial_port;

const std::vector<std::string> AtCmd::smsUnsoliciteds = {"+CMT:", "+CDS:", "+CBM:"};
const std::vector<std::string> AtCmd::finalResponsesSuccess = {"OK", "CONNECT"};
const std::vector<std::string> AtCmd::finalResponsesError = {"ERROR", "+CMS ERROR:",
                                                             "+CME ERROR:", "NO CARRIER",
                                                             "NO ANSWER", "NO DIALTONE"};

AtCmd::AtCmd() : ioContext(), uart(ioContext)
{
}
AtCmd::~AtCmd()
{
    uart.close();
}
void AtCmd::initModem()
{
    const char *ateCmd = "ate0";
    AtResp out;
    sendAtCmd(ateCmd, out);
}
bool AtCmd::initUart(std::string &uartDev)
{
    try
    {
        uart.open(uartDev);
        setUart();
    }
    catch (const std::exception &e)
    {
        std::cout << "Open Uart [" << uartDev << "] error:" << e.what()<< std::endl;
        return false;
    }
    resp.clear();
    readThread = std::thread(&AtCmd::readLoop, this);
    readThread.detach();
    initModem();
    return true;
}

void AtCmd::setUart()
{
    uart.set_option(serial_port::baud_rate(115200));
    uart.set_option(serial_port::flow_control(serial_port::flow_control::none));
    uart.set_option(serial_port::parity(serial_port::parity::none));
    uart.set_option(serial_port::stop_bits(serial_port::stop_bits::one));
}

void AtCmd::readLoop()
{
    boost::asio::streambuf recvBuf;
    std::cout << "Start readLoop" << std::endl;
    while (1)
    {
        size_t ret = boost::asio::read_until(uart, recvBuf, "\r\n");
        auto bufs = recvBuf.data();
        std::string line(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + ret - 2);
        recvBuf.consume(ret);

        if (!line.empty())
        {
            std::cout << "Read line :" << line << std::endl; 
            std::lock_guard<std::mutex> lk(cmdMutex);
            resp.push_back(line);
        }
        if (recvBuf.size() == 0)
        {
            if (writeMutex.try_lock())
            {
                std::cout << "Not cmd wait resp:" << std::endl;
                for (auto &urc : resp)
                {
                    std::cout << "Urc:" << urc << std::endl;
                }
                resp.clear();
                writeMutex.unlock();
            }
            else
            {
                cv.notify_all();
            }
        }
    }
    std::cout << "End readLoop" << std::endl;
}
bool AtCmd::sendAtCmd(const std::string &cmd, AtResp &outResp)
{
    std::string execCmd(cmd);
    execCmd.append("\r\n");
    LOG_Debug() << "Start execCmd[" << cmd << "]";
    std::lock_guard<std::mutex> writeLock(writeMutex);
    boost::system::error_code ec;
    size_t ret = uart.write_some(boost::asio::buffer(execCmd), ec);
    if (ec)
    {
        std::cout << "Write cmd[" << cmd << "] Failed:" << ec.message()<< std::endl;
        return false;
    }

    std::unique_lock<std::mutex> lk(cmdMutex);
    auto status = cv.wait_for(lk, std::chrono::milliseconds(300));
    if (status == std::cv_status::timeout)
    {
        std::cout << "exec Cmd[" << cmd << "] failed,wait resp time out" << std::endl;
        return false;
    }
    if (isSMSUnsolicited(resp.front()))
    {
        std::cout << "Get SMS Urc:[" << resp.front() << "]" << std::endl;
        return false;
    }
    bool result = isFinalResponse(resp.back());
    std::cout << "exec Cmd[" << cmd << "] get Resp[" << resp.front() << "]" << std::endl;
    if (result)
    {
        resp.erase(resp.end());
    }
    outResp = resp;
    resp.clear();
    return result;
}
bool AtCmd::sendAtCmd(const char *cmd, AtResp &outResp)
{
    std::string execCmd(cmd);
    return sendAtCmd(execCmd, outResp);
}

bool AtCmd::sendAtCmd(const std::string &cmd, const std::string &expectResp, AtResp &outResp)
{
    bool result = sendAtCmd(cmd, outResp);
    if (result)
    {
        for (auto &respRet : outResp)
        {
            if (respRet.find(expectResp) == 0)
            {
                result = true;
                break;
            }
            else
            {
                result = false;
            }
        }
        if (!result)
        {
            std::cout << "execCmd[" << cmd << "] Success,but not get expectResp[" << expectResp
                       << "].resp[" << outResp.front() << "]" << std::endl;
        }
    }
    return result;
}

bool AtCmd::findString(std::string &line, const std::vector<std::string> &testV)
{
    for (auto &test : testV)
    {
        if (line.find(test) == 0)
        {
            return true;
        }
    }
    return false;
}

bool AtCmd::isSMSUnsolicited(std::string &line)
{
    return findString(line, smsUnsoliciteds);
}
bool AtCmd::isFinalResponse(std::string &line)
{
    return isFinalResponseSuccess(line) || isFinalResponseError(line);
}
bool AtCmd::isFinalResponseSuccess(std::string &line)
{
    return findString(line, finalResponsesSuccess);
}
bool AtCmd::isFinalResponseError(std::string &line)
{
    return !findString(line, finalResponsesError);
}

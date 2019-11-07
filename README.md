# atCmd
用于发送接收at命令，依赖于[boost::asio::serial_port](https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/reference/serial_port.html) 和 [boost::serialization::singleton](https://www.boost.org/doc/libs/1_70_0/libs/serialization/doc/singleton.html) 库。
依赖于c++17

# 编译
```
g++ -o main main.c atCmd.cpp -lpthread -std=c++17
```

# 运行
```
./main /dev/ttyUSB1
```

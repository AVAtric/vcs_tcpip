# Bulletin Board

## Prerequisite
```
/usr/local/bin/simple_message_server_logic
```

## Download and install C-Compiler + Doxygen
```
Fedora/CentOS:
$ sudo yum install gcc doxygen

Ubuntu:
$ sudo apt-get install gcc doxygen
```

## Download and install cmake
```
$ cd ~
$ wget https://cmake.org/files/v3.13/cmake-3.13.1-Linux-x86_64.sh
$ sh ./cmake-3.13.1-Linux-x86_64.sh
$ ./cmake-3.13.1-Linux-x86_64/bin/cmake -version
$ cmake -version
```

## Build
```
$ git clone https://github.com/AVAtric/vcs_tcpip.git
$ cd vcs_tcpip
$ mkdir release
$ cd release
$ ~/cmake-3.13.1-Linux-x86_64/bin/cmake ..
$ make
```

## Use
```
$ ./simple_message_client -s server -p port -u user [-i image URL] -m message [-v] [-h]
$ ./simple_message_server -p port [-h]
```

# yunji_sdk
Yunji robot sdk version 2.

### Prebuild environment
* OS  (Ubuntu 20.04 LTS)  
* CPU  (aarch64 and x86_64)   
* Compiler  (gcc version 9.4.0) 

### Build examples

To build the examples inside this repository:

```bash
mkdir build
cd build
cmake ..
make
```

### Installation

To build your own application with the SDK, you can install the yunji_sdk to your system directory:

```bash
mkdir build
cd build
cmake ..
sudo make install
```

Or install yunji_sdk to a specified directory:

```bash
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/unitree_robotics
sudo make install
```

You can refer to `example/cmake_sample` on how to import the yunji_sdk into your CMake project. 

Note that if you install the library to other places other than `/opt/unitree_robotics`, you need to make sure the path is added to "${CMAKE_PREFIX_PATH}" so that cmake can find it with "find_package()".

### Notice
For more reference information, please go to [Yunji Document Center](https://support.unitree.com/home/zh/developer).

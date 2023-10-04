#!/bin/bash
set -e -o pipefail

# Add repositories first
# For Kitware's CMake package
sudo add-apt-repository -y -n "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
# For Ubuntu test toolchain for g++-11
sudo add-apt-repository -y -n ppa:ubuntu-toolchain-r/test

# For Vulkan SDK
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-$(lsb_release -cs).list http://packages.lunarg.com/vulkan/lunarg-vulkan-$(lsb_release -cs).list

# Import all necessary keys
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc

# Add repositories based on the CPU and GPU
cpu=$(lscpu)
gpu=$(lspci | grep -i --color 'vga\|3d\|2d')

if echo "$gpu" | grep -q -i "nvidia\|intel"; then
    # For updated open-source graphics drivers (mesa-vulkan-drivers)
    sudo add-apt-repository -y -n ppa:oibaf/graphics-drivers
elif echo "$gpu" | grep -q -i "Microsoft Corporation Device"; then
    # NVidia drivers are now already per default included in the WSL 2 installation:
    # Once a Windows NVIDIA GPU driver is installed on the system, CUDA becomes available within WSL 2.
    # The CUDA driver installed on Windows host will be stubbed inside the WSL 2 as libcuda.so, therefore
    # users must not install any NVIDIA GPU Linux driver within WSL 2. One has to be very careful here as
    # the default CUDA Toolkit comes packaged with a driver, and it is easy to overwrite the WSL 2 NVIDIA
    # driver with the default installation.
    # https://docs.nvidia.com/cuda/wsl-user-guide/index.html#getting-started-with-cuda-on-wsl
    echo "script has been invoked from inside WSL? --> not installing drivers and assuming nvidia gpu!"
elif echo "$cpu" | grep -q -i "amd"; then
    # For proprietary AMD graphics driver (AMDVLK)
    sudo wget -qO - http://repo.radeon.com/amdvlk/apt/debian/amdvlk.gpg.key | sudo apt-key add -
    sudo sh -c 'echo deb [arch=amd64,i386] http://repo.radeon.com/amdvlk/apt/debian/ bionic main > /etc/apt/sources.list.d/amdvlk.list'
    sudo apt-get remove -y amdvlk || true
fi

# Update and install packages
sudo apt update
sudo apt install -y wget g++ gdb make ninja-build rsync zip software-properties-common libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev lsb-release kitware-archive-keyring cmake libassimp-dev g++-11 dpkg-dev vulkan-sdk vulkan-utils libvulkan-dev libvulkan1 libvulkan1-dbgsym --fix-missing

if echo "$gpu" | grep -q -i "nvidia\|intel"; then
    # For updated open-source graphics drivers (mesa-vulkan-drivers)
    sudo apt install -y mesa-vulkan-drivers libgl1-mesa-dev
elif echo "$gpu" | grep -q -i "Microsoft Corporation Device"; then
    # NVidia drivers are now already per default included in the WSL 2 installation:
    # Once a Windows NVIDIA GPU driver is installed on the system, CUDA becomes available within WSL 2.
    # The CUDA driver installed on Windows host will be stubbed inside the WSL 2 as libcuda.so, therefore
    # users must not install any NVIDIA GPU Linux driver within WSL 2. One has to be very careful here as
    # the default CUDA Toolkit comes packaged with a driver, and it is easy to overwrite the WSL 2 NVIDIA
    # driver with the default installation.
    # https://docs.nvidia.com/cuda/wsl-user-guide/index.html#getting-started-with-cuda-on-wsl
    echo "script has been invoked from inside WSL? --> not installing drivers and assuming nvidia gpu!"
elif echo "$cpu" | grep -q -i "amd"; then
    # For proprietary AMD graphics driver (AMDVLK)
    sudo apt-get install -y amdvlk
fi


sudo apt upgrade -y
sudo apt clean all

# Set gcc/g++ version
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60 --slave /usr/bin/g++ g++ /usr/bin/g++-11

# Test Vulkan installation
echo ""
echo "Now running \"vulkaninfo\" to see if vulkan has been installed successfully:"
vulkaninfo


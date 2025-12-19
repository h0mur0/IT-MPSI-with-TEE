# SRB-MPSI

This project implements SRB-MPSI, an information-theoretically secure multi-party private set intersection protocol using Intel SGX for trusted execution environment.

## Building the Project

The project requires Intel SGX to be properly configured on your system. Follow these steps to set up the environment:

### 1. System Requirements
- A CPU with SGX support (check with `grep -q sgx /proc/cpuinfo`)
- Ubuntu 18.04 or later
- GCC and Make

### 2. Install Intel SGX Driver
```bash
# Download the driver
wget https://download.01.org/intel-sgx/sgx-linux/2.13/distro/ubuntu18.04-server/sgx_linux_x64_driver_2.11.0_0373e2e.bin

# Install the driver
sudo chmod +x sgx_linux_x64_driver_2.11.0_0373e2e.bin
sudo ./sgx_linux_x64_driver_2.11.0_0373e2e.bin
```

### 3. Install Intel SGX SDK
```bash
# Install dependencies
sudo apt-get install build-essential ocaml ocamlbuild automake autoconf libtool wget python libssl-dev git cmake perl

# Clone and build SGX SDK
git clone https://github.com/intel/linux-sgx.git
cd linux-sgx
make preparation
make sdk
make sdk_install_pkg

# Install the SDK package
cd linux/installer/bin
sudo ./sgx_linux_x64_sdk_*.bin
# When prompted, install to /opt/intel/
```

### 4. Install Intel PSW
```bash
# Add repository and install PSW
echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu bionic main' | sudo tee /etc/apt/sources.list.d/intel-sgx.list
wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add -
sudo apt-get update
sudo apt-get install libsgx-launch libsgx-urts libsgx-epid libsgx-quote-ex libsgx-dcap-ql
```

### 5. Build the Application
```bash
cd [project-directory]
source /opt/intel/sgxsdk/environment
make
```

## Running the Code

### Method 1: Direct Execution
Use the following instruction to run the application:
```bash
./app -m [positive_integer_a] -t [a_filenames] -n [a_integers] -K [positive_integer]
```

**Required parameters:**
- `-m [value]`: Number of clients (M)
- `-t [values]`: List of client data filenames
- `-n [values]`: List of database sizes for each client (currently only supports equal sizes for all clients)
- `-K [value]`: Universe size

**Example:**
```bash
./app -m 5 -t data1.txt data2.txt data3.txt data4.txt data5.txt -n 5 5 5 5 5 -K 256
```

### Method 2: Using Test Script
```bash
python3 test1.py
```

## Environment Setup

Before running the application, ensure the SGX environment is loaded:
```bash
source /opt/intel/sgxsdk/environment
```


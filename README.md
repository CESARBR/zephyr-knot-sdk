# KNoT Network of Things

Zephyr KNoT SDK provides libraries, tools and samples to make a KNoT application for Zephyr environment.

## How to Build it

### KNoT Requirements
---
#### In order to build a KNoT application, some requirements are needed:

**1 - Source KNoT environment configuration file.**
The environment configuration file is used to set up KNOT_BASE path.
```bash
$ cd zephyr-knot-sdk
$ source knot-env.sh
```
**2 - Zephyr**
KNoT uses a fork of Zephyr repository.

- Clone KNoT Zephyr fork
	```bash
	$ git clone https://github.com/CESARBR/zephyr.git
	```
- Checkout to KNoT branch
	```bash
	$ git checkout -b ot-setup
	```
- Set up a Zephyr development environment on your system
	> Follow this [instructions](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system)

- Set up zephyr environment variables
	```bash
	$ cd <zephyr-dir>
	$ source zephyr-env.sh
	```

**3 - Openthread**
KNoT uses Openthread stack.

- Clone Openthread Github repository:
	```bash
	$ git clone https://github.com/openthread/openthread.git
	```
- Checkout to `2a75d30684654e0348bce9327c6f45f4e9ea71a5` hash
	```bash
	$ git checkout -b knot_hash 2a75d30684654e0348bce9327c6f45f4e9ea71a5
	```

**4 - nRF5x Command Line Tools**
To download and more info [check this link.](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools)

**5 - MCUBOOT**
- Requirements
	- python 3
	- virtualenv
	- Zephyr repository
	- nrfjprog (from nRF5x Command Line Tools)

- Clone MCUBOOT repository
	```bash
	$ git clone https://github.com/JuulLabs-OSS/mcuboot.git
	```
- Create and activate virtual environment
	```bash
	$ virtualenv -p `which python3` venv
	```
- Install python dependencies
	```bash
	$ cd <mcuboot_dir>
	$ pip3 install -r scripts/requirements.txt
	$ pip3 install pyyaml pyelftools
	```
- Set up zephyr environment variables
	```bash
	$ source <zephyr_dir>/zephyr-env.sh
	```
- Generate MCUBOOT Makefile
	```bash
	$ cd <mcuboot_dir>/boot/zephyr
	$ mkdir build && cd build
	$ cmake -DBOARD=nrf52840_pca10056 ..
	```
- Use config interface to set up signature settings
	```bash
	$ make menuconfig
	> MCUBoot settings > Signature type and PEM key file
	```
	> For test purpose, KNoT use ECDSA_P256 and root-ec-p256.pem file, available on MCUBOOT root directory.

- Build MCUBOOT for zephyr
	```bash
	$ make
	```
- Flash MCUBOOT
	```bash
	$ make flash
	```

---
### Build KNoT application
---
**1 - Application folder**
```bash
$ cd zephyr-knot-sdk/<app-dir>
```
**2 - Create build directory**
```bash
$ mkdir build && cd build
```
**3 - Generate Makefile**
```bash
$ cmake -DEXTERNAL_PROJECT_PATH_OPENTHREAD=<path-to-openthread> -DOVERLAY_CONFIG=${KNOT_BASE}/core/overlay-knot-ot.conf ..
```
> The default (and only supported) board used by KNoT is nrf52840_pca10056.

**4 - Build it!**
```bash
$ make
```

---
### Build KNoT Setup App
---
**1 - Application folder**
```bash
$ cd zephyr-knot-sdk/<setup-dir>
```
**2 - Create build directory**
```bash
$ mkdir build && cd build
```
**3 - Generate Makefile**
```bash
$ cmake -DBOARD=nrf52840_pca10056 ..
```
**4 - Build it!**
```bash
$ make
```


## How to flash

**1 - Flash MCUBOOT**
```bash
$ cd <mcuboot-dir>/boot/zephyr/build
$ make flash
```
> For more details check How To Build it -> KNoT Requirements -> MCUBOOT

**2 - Merge KNoT Setup and KNoT App images**
```bash
$ mergehex -m ${KNOT_BASE}/setup/build/zephyr/zephyr.hex ${KNOT_BASE}/apps/hello/build/zephyr/zephyr.hex -o ${KNOT_BASE}/build/slot0_merged.hex
```
**3 - Sign image**
```bash
$ <mcuboot-dir>/scripts/imgtool.py sign --key <mcuboot-dir>/root-ec-p256.pem --header-size 0x200 --align 8 --version 1.5 --slot-size 0x69000 --pad ${KNOT_BASE}/build/slot0_merged.hex  ${KNOT_BASE}/build/slot0_merged_signed.hex
```
> Remember to activate MCUBOOT virtualenv

**4 - Flash signed image**
```bash
$ nrfjprog --program ${KNOT_BASE}/build/slot0_merged_signed.hex --sectorerase -f nrf52 --reset
```
**5 - Use serial monitor**
```bash
$ minicom -D /dev/ttyACM0 -b 115200
```
> You can install minicom using:
> ``
> $ sudo apt-get install minicom
> ``

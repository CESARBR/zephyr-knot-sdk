# KNoT Network of Things

Zephyr KNoT SDK provides libraries, tools and samples to make a KNoT application for Zephyr environment.

---
## KNoT Requirements

### In order to build a KNoT application, some requirements are needed:

#### 1 - Zephyr

KNoT uses a fork of Zephyr repository.

- Set up a Zephyr development environment on your system
	> Follow these [instructions](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system)
- Install the west binary and bootstrapper
	```bash
	$ pip3 install --user west
	```
- Create an enclosing folder and change directory
	```bash
	$ mkdir zephyrproject
	$ cd zephyrproject
	```
- Clone KNoT Zephyr fork
	```bash
	$ git clone https://github.com/CESARBR/zephyr.git
	```
- Initialize west, inside zephyrproject directory run
	```bash
	$ west init -l zephyr/
	$ west update
	```
- Checkout to KNoT branch
	```bash
	$ cd <zephyr-dir>
	$ git checkout -b zephyr-port-1.14-rcx origin/zephyr-port-1.14-rcx
	```
- Set up zephyr environment variables
	```bash
	$ cd <zephyr-dir>
	$ source zephyr-env.sh
	```
	> It is necessary to source zephyr-env.sh every time a new terminal is opened.

#### 2 - Openthread

KNoT uses Openthread stack.

- Clone Openthread Github repository:
	```bash
	$ git clone https://github.com/openthread/openthread.git
	```
- Checkout to `f9d757a161fea4775d033a1ce88cf7962fe24a93` hash
	```bash
	$ cd <openthread-dir>
	$ git checkout -b knot_hash f9d757a161fea4775d033a1ce88cf7962fe24a93
	```

#### 3 - nRF5x Command Line Tools

- Download and install nrfjprog and mergehex cli applications.

> To download and more info [check this link.](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools)

> Make sure that you have the nrfjprog and the mergehex in your path.

#### 4 - Source KNoT environment configuration file

The environment configuration file is used to set up KNOT_BASE path.
```bash
$ cd zephyr-knot-sdk
$ source knot-env.sh
```
> It is necessary to source knot-env.sh every time a new terminal is opened.

> There are two ways to build and run a KNoT Application [Using cli script](#using-cli-script) or [Not using cli script](#not-using-cli-script).

---
## Using cli script
#### Installing dependencies
- Use pip to install requirements
	```bash
	$ pip3 install --user -r ${KNOT_BASE}/scripts/requirements.txt
	```

#### Flash MCUBOOT
- Flash stock MCUBOOT image using signature type ECDSA_P256
	```bash
	$ python3 ${KNOT_BASE}/scripts/cli.py mcuboot
	```

#### Build KNoT App
- Build KNoT Core and KNoT Setup applications
	```bash
	$ python3 ${KNOT_BASE}/scripts/cli.py make --ot_path=<path-to-openthread>
	```

#### Flash KNoT App
- Build and flash KNoT Core and KNoT Setup applications
	```bash
	$ python3 ${KNOT_BASE}/scripts/cli.py make flash
	```
#### Monitor the output
- You can use minicon to monitor the serial logger.
The device may be different depending on your system.
	```bash
	$ minicom -D <your-device> -b 115200
	```
	> If you are using debian the device usually is /dev/ttyACM0 and you can install minicom using:
	> ``
	> $ sudo apt-get install minicom
	> ``
---
## Not using cli script

### Build MCUBOOT
- Requirements
	- python 3
	- virtualenv
	- Zephyr repository (installed at step 2)
	- nrfjprog (installed at step 3)

#### 1 - Clone MCUBOOT repository
```bash
$ git clone https://github.com/JuulLabs-OSS/mcuboot.git
```
#### 2 - Checkout to knot_hash
```bash
$ cd <mcuboot-dir>
$ git checkout -b knot_hash fda937ab02296f6fd8e7195e2846d631f3d70559
```
#### 3 - Create and activate virtual environment
```bash
$ virtualenv -p `which python3` venv
$ source venv/bin/activate
```
#### 4 - Install python dependencies
```bash
$ pip3 install asn1crypto==0.24.0 cffi==1.11.5 Click==7.0 colorama==0.4.1 cryptography==2.4.2 docopt==0.6.2 idna==2.8 intelhex==2.2.1 pycparser==2.19 pyelftools==0.25 pykwalify==1.7.0 python-dateutil==2.7.5 PyYAML==3.013 six==1.11.0
```
#### 5 - Set up zephyr environment variables
```bash
$ source <zephyr_dir>/zephyr-env.sh
```
#### 6 - Generate MCUBOOT Makefile
```bash
$ cd <mcuboot_dir>/boot/zephyr
$ mkdir build && cd build
$ cmake -DBOARD=nrf52840_pca10056 ..
```
#### 7 - Use config interface to set up signature settings
```bash
$ make menuconfig
> MCUBoot settings
> Signature type
	> Select ECDSA_P256
> PEM key file
	> root-ec-p256.pem
```
> KNoT uses ECDSA_P256 type and root-ec-p256.pem file, available on MCUBOOT root directory. You can change this to your own settings.

#### 8 - Build MCUBOOT for zephyr
```bash
$ make
```

### Build KNoT application

#### 1 - Application folder
```bash
$ cd zephyr-knot-sdk/apps/<app-dir>
```
> You can use the KNoT hello app as an example

#### 2 - Create build directory
```bash
$ mkdir build && cd build
```
#### 3 - Generate Makefile
```bash
$ cmake -DEXTERNAL_PROJECT_PATH_OPENTHREAD=<path-to-openthread> -DOVERLAY_CONFIG=${KNOT_BASE}/core/overlay-knot-ot.conf ..
```
> The default (and only supported) board used by KNoT is nrf52840_pca10056.

#### 4 - Build it!
```bash
$ make
```

### Build KNoT Setup App
#### 1 - Application folder
```bash
$ cd zephyr-knot-sdk/<setup-dir>
```
#### 2 - Create build directory
```bash
$ mkdir build && cd build
```
#### 3 - Generate Makefile
```bash
$ cmake -DBOARD=nrf52840_pca10056 ..
```
#### 4 - Build it!
```bash
$ make
```


### How to flash

#### 1 - Flash MCUBOOT
```bash
$ cd <mcuboot-dir>/boot/zephyr/build
$ make flash
```
> For more details check How To Build it -> KNoT Requirements -> MCUBOOT

#### 2 - Merge KNoT Setup and KNoT App images
```bash
$ mergehex -m ${KNOT_BASE}/setup/build/zephyr/zephyr.hex ${KNOT_BASE}/apps/hello/build/zephyr/zephyr.hex -o ${KNOT_BASE}/build/slot0_merged.hex
```
#### 3 - Sign image

> For this step, make sure MCUBOOT virtualenv is activated.

```bash
$ <mcuboot-dir>/scripts/imgtool.py sign --key <mcuboot-dir>/root-ec-p256.pem --header-size 0x200 --align 8 --version 1.5 --slot-size 0x69000 --pad ${KNOT_BASE}/build/slot0_merged.hex  ${KNOT_BASE}/build/slot0_merged_signed.hex
```
#### 4 - Flash signed image
```bash
$ nrfjprog --program ${KNOT_BASE}/build/slot0_merged_signed.hex --sectorerase -f nrf52 --reset
```
#### 5 - Monitor the output
- You can use minicon to monitor the serial logger.
The device may be different depending on your system.
	```bash
	$ minicom -D <your-device> -b 115200
	```
	> If you are using debian the device usually is /dev/ttyACM0 and you can install minicom using:
	> ``
	> $ sudo apt-get install minicom
	> ``

# KNoT Network of Things

Zephyr KNoT SDK provides libraries, tools and samples to make a KNoT application for Zephyr environment.

## How to Build it

### KNoT Requirements
---
#### In order to build a KNoT application, some requirements are needed:

#### 1 - Zephyr

KNoT uses a fork of Zephyr repository.

- Clone KNoT Zephyr fork
	```bash
	$ git clone https://github.com/CESARBR/zephyr.git
	```
- Checkout to KNoT branch
	```bash
	$ git checkout -b ot-setup origin/ot-setup
	```
- Set up a Zephyr development environment on your system
	> Follow these [instructions](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system)

- Set up zephyr environment variables
	```bash
	$ cd <zephyr-dir>
	$ source zephyr-env.sh
	```

#### 2 - Openthread

KNoT uses Openthread stack.

- Clone Openthread Github repository:
	```bash
	$ git clone https://github.com/openthread/openthread.git
	```
- Checkout to `2a75d30684654e0348bce9327c6f45f4e9ea71a5` hash
	```bash
	$ git checkout -b knot_hash 2a75d30684654e0348bce9327c6f45f4e9ea71a5
	```

#### 3 - nRF5x Command Line Tools

- Download and install nrfjprog and mergehex cli applications .

> To download and more info [check this link.](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools)

#### 4 - MCUBOOT
- Requirements
	- python 3
	- virtualenv
	- Zephyr repository (installed at step 2)
	- nrfjprog (installed at step 3)

- Clone MCUBOOT repository
	```bash
	$ git clone https://github.com/JuulLabs-OSS/mcuboot.git
	```
- Checkout to `5e9078f1e0bfed493b4265dd435cccb856cf2d6f` hash
	```bash
	$ cd <mcuboot-dir>
	$ git checkout -b knot_hash 5e9078f1e0bfed493b4265dd435cccb856cf2d6f
	```
- Create and activate virtual environment
	```bash
	$ virtualenv -p `which python3` venv
	$ source venv/bin/activate
	```
- Install python dependencies
	```bash
	$ pip3 install asn1crypto==0.24.0 cffi==1.11.5 Click==7.0 colorama==0.4.1 cryptography==2.4.2 docopt==0.6.2 idna==2.8 intelhex==2.2.1 pkg-resources==0.0.0 pycparser==2.19 pyelftools==0.25 pykwalify==1.7.0 python-dateutil==2.7.5 PyYAML==3.13 six==1.11.0
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
	> MCUBoot settings
	> Signature type
		> Select ECDSA_P256
	> PEM key file
		> root-ec-p256.pem
	```
	> KNoT uses ECDSA_P256 type and root-ec-p256.pem file, available on MCUBOOT root directory. You can change this to your own settings.

- Build MCUBOOT for zephyr
	```bash
	$ make
	```
- Flash MCUBOOT
	```bash
	$ make flash
	```

#### 5 - Source KNoT environment configuration file.

The environment configuration file is used to set up KNOT_BASE path.
```bash
$ cd zephyr-knot-sdk
$ source knot-env.sh
```

#### 6 - Create KNoT build directory.

KNoT image will be generated on this directory.
```bash
$ cd zephyr-knot-sdk
$ mkdir build
```

---
### Build KNoT application
---
#### 1 - Application folder
```bash
$ cd zephyr-knot-sdk/<app-dir>
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

---
### Build KNoT Setup App
---
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


## How to flash

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
#### 5 - Use serial monitor
```bash
$ minicom -D /dev/ttyACM0 -b 115200
```
> You can install minicom using:
> ``
> $ sudo apt-get install minicom
> ``

# KNoT Network of Things

Zephyr KNoT SDK provides libraries, tools and samples to make a KNoT application for Zephyr environment.

---
## KNoT Requirements

### In order to build a KNoT application, some requirements are needed:

#### 1 - Linux based OS
The actual version of KNoT Zephyr SDK is only available for Linux based systems.
You may consider using a Virtual Machine running a Linux distribution on other systems.


#### 2 - Zephyr
KNoT uses a fork of Zephyr repository.

- Set up a Zephyr development environment by following these [instructions](https://docs.zephyrproject.org/latest/getting_started/index.html#set-up-a-development-system).
- Program the `$HOME/.profile` to always set ZEPHYR_TOOLCHAIN_VARIANT and ZEPHYR_SDK_INSTALL_DIR.
	```bash
	$ echo "export ZEPHYR_TOOLCHAIN_VARIANT=zephyr" >> $HOME/.profile
	$ echo "export ZEPHYR_SDK_INSTALL_DIR=`readlink -f <SDK-INSTALLATION-DIRECTORY>`" >> $HOME/.profile
	```
	> *Replace **`<SDK-INSTALLATION-DIRECTORY>`** by the actual Zephyr SDK install path*

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
	$ git clone -b zephyr-knot-v1.14.0 https://github.com/CESARBR/zephyr.git
	```
- Initialize west. Inside zephyrproject directory run:
	```bash
	$ west init -l zephyr/
	$ west update
	```
	> If the system can't find west, try logging out and in again.
- Set up zephyr environment variables
	```bash
	$ source zephyr/zephyr-env.sh
	```
- Program the `$HOME/.profile` to always source zephyr-env.sh when you log in.
	```bash
	$ echo "source $ZEPHYR_BASE/zephyr-env.sh" >> $HOME/.profile
	```
	> If you skip this step, it will be necessary to manually source zephyr-env.sh every time a new terminal is opened.


#### 3 - nRF5x Command Line Tools

- Download and install **nrfjprog** and **mergehex** cli applications at [nRF5 Command Line Tools](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools).

- Add the nrfjprog and mergehex executables to the `~/.local/bin/` folder.
	```bash
	$ ln -s `readlink -f <MERGEHEX-PATH>` >> $HOME/.local/bin/mergehex
	$ ln -s `readlink -f <NRFJPROG-PATH>` >> $HOME/.local/bin/nrfjprog
	```
	> *Replace `<MERGEHEX-PATH>` and `<NRFJPROG-PATH>` by path to the downloaded apps*

#### 4 - Install Segger JLink
> This is only necessary if you're going to use the [DK board](https://docs.zephyrproject.org/latest/boards/arm/nrf52840_pca10056/doc/index.html).

- Download the appropriate package from the [J-Link Software](https://www.segger.com/downloads/jlink/) and install it.

#### 5 - Source KNoT environment configuration file

- Download the zephyr-knot-sdk repository to a folder you prefer.
	```bash
	$ git clone https://github.com/cesarbr/zephyr-knot-sdk/
	```
- The environment configuration file is used to set up KNOT_BASE path.
	```bash
	$ source zephyr-knot-sdk/knot-env.sh
	```

- Program the `$HOME/.profile` to always source knot-env.sh when you log in.
	```bash
	$ echo "source $KNOT_BASE/knot-env.sh" >> $HOME/.profile
	```


#### 6 - Add support to the KNoT command line interface

- Add cli.py to the path files.
	```bash
	$ ln -s $KNOT_BASE/scripts/cli.py $HOME/.local/bin/knot
	```
	> This will allow you to call the knot command line interface from any folder.

- Use pip to install cli requirements
	```bash
	$ pip3 install --user -r ${KNOT_BASE}/scripts/requirements.txt
	```
	> If you skip this step, it will be necessary to manually source knot-env.sh every time a new terminal is opened.

#### 7 - KNoT protocol
- Follow the instructions to install the [KNoT protocol library](https://github.com/CESARBR/knot-protocol-source).

#### 8 - Add USB access to your user
- Add your user to the dialout group.
	```bash
	$ sudo usermod -a -G dialout `whoami`
	```

#### 9 - Apply changes to profile
- In order to apply the changes to your user, you must log out and log in again.
---
## Building and flashing a KNoT App
> Make sure that you have sourced the knot-env.sh, zephyr-env.sh and defined the values for ZEPHYR_SDK_INSTALL_DIR and ZEPHYR_TOOLCHAIN_VARIANT.

#### Connect and set the target board
You may be using one of the two supported boards: DK (nrf52840_pca10056) or Dongle (nrf52840_pca10059).
- If using the [DK](https://docs.zephyrproject.org/latest/boards/arm/nrf52840_pca10056/doc/index.html):
	- Connect the board to a USB port
	- Set the DK as the default target board
		```bash
		$ knot board dk
		```

- If using the [Dongle](https://docs.zephyrproject.org/latest/boards/arm/nrf52840_pca10059/doc/index.html):
	- Connect the board to a USB port
	- Press the reset button. The red led will blink.
	- Set the Dongle as the default target board
		```bash
		$ knot board dongle
		```

#### Build KNoT App
- Go to the KNoT App file.
	```bash
	$ cd <path-to-app>
	```
	> If you don't have an app, you may use the example at $KNOT_BASE/apps/hello

- Build and flash apps
	```bash
	$ knot make --mcuboot
	```
	> The option 'mcuboot' flashes the compiled program and the mcuboot booloader at the end of building.

#### Monitor the output
> You can use minicom or any other serial port reader to monitor the app output.
- Install minicom
	> You can install minicom on debian based systems by using:
	> ``
	> $ sudo apt-get install minicom
	> ``

	```bash
	$ minicom -D <your-device>
	```
	> If you are using debian the device usually is something like /dev/ttyACM0.

---
## Building configure
There are many commands that you can call when using the knot script (cli.py). Some of their functions are:

#### Set default target board
- You can set the default target board with the command:
	```bash
	$ knot board <BOARD>
	```
	Where `<BOARD>` can be 'dk' or 'dongle'

#### Set external OpenThread path
To avoid downloading the OpenThread repo on every new build, set an external path:
- Clone Openthread Github repository:
	```bash
	$ git clone https://github.com/openthread/openthread.git
	```

- Checkout to the compatible hash and set it as default
	```bash
	$ cd openthread
	$ git checkout -b knot_hash f9d757a161fea4775d033a1ce88cf7962fe24a93
	$ knot ot-path `pwd`
	```

#### See Log messages through USB
It is possible to get the logging messages through the USB connector when using
the Dongle by enabling the USB stack:
- Compile with USB stack:
	```bash
	$ knot make --usb
	```

#### Clear project before building
The user can delete old building files before compiling again:
> This is especially useful when the project had important changes like different target board or dependency repository.
- Clear old files before compiling:
	```bash
	$ knot make --clean
	```

#### Flash board when done
The user can flash the board just after building it:
- Flash board after building:
	```bash
	$ knot make --flash
	```
	> This option also flashes the bootloader when targeting the Dongle.

#### Flash bootloader
When using the DK, it's possible for the board to be flashed without the bootloader.
	To fix that, the user should flash it separately.
- Flash bootloader to board.
	```bash
	$ knot mcuboot
	```
	> This option also erases the main app when targeting the Dongle.

#### Other commands
These and the other commands are described when using the command:
- Read help
	```bash
	$ knot --help
	```

# Zephyr KNoT SDK Docker environment

The Docker image can be used to ease building new apps using a pre-configured
environment.

It supports Docker for Mac, Linux and Windows.

To build or run the image, you have to [install Docker](https://docs.docker.com/install/).

## Building image

Build it by the tag `knot-zephyr-sdk`.

```shell
docker build --tag=knot-zephyr-sdk .
```

## Running the container

Run the latest image version at CESAR's Docker Hub and pass the current working
directory as the project folder.
This folder shall contain all your project files.

At your project folder, run:

```shell
docker run -ti -v $(pwd)/:/workdir cesarbr/knot-zephyr-sdk:latest
```

If you want to run it from an image you built, replace `cesarbr/knot-zephyr-sdk:latest`
by the tag you used.

Compile for your target board

```shell
container> $ knot make -b {BOARD}
```

## Exporting generated files

Export the generated files to your project's directory
```shell
container> $ knot export /workdir/output
```

The generated files can now be flashed to your device by using the
[nRF Connect for Desktop](https://www.nordicsemi.com/?sc_itemid=%7B49D2264D-62FD-4C16-811F-88B477833C5D%7D) and its
[Programmer app](https://infocenter.nordicsemi.com/topic/ug_nc_programmer/UG/nrf_connect_programmer/ncp_introduction.html).

## Flashing on Linux

If using a Linux device with the necessary drivers for flashing the boards,
you may give USB access to the container.

Before proceeding, make sure that you added your user to the dialout group.

```shell
sudo usermod -a -G dialout `whoami`
```

You can now access the container using the host `/dev` directory.

```shell
docker run -ti --privileged -v /dev:/dev -v $(pwd)/:/workdir cesarbr/knot-zephyr-sdk:latest
```

This will allow you to use the `--flash` flag to flash after building the project.

```shell
container> $ knot make -b {BOARD} --flash
```

## Using other knot commands

When inside the Docker container, you may use any KNoT command from the command line interface.

To get a list of all available commands, run:

```
container> $ knot --help
```

More info is available at the [Zephy KNoT SDK README](https://github.com/CESARBR/zephyr-knot-sdk/).

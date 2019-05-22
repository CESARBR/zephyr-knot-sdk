#!/usr/bin/env python3
#
# Copyright (c) 2019, CESAR. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

from configparser import ConfigParser

import os
import shutil
import errno
import sys
import click
import subprocess
import logging
import coloredlogs
import serial.tools.list_ports


class Singleton(type):
    """
    Singleton class to guarantee that a single instance will be used for
    its inhereted classes
    """
    __instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls.__instances:
            cls.__instances[cls] = super(Singleton,
                                         cls).__call__(*args, **kwargs)
        return cls.__instances[cls]


class App(object):
    name = ""  # App name
    root_path = None  # Path to root folder
    build_path = None  # Path to build folder
    hex_path = None  # Path to generated hex file

    class Constants:
        BUILD_PATH = "build"
        HEX_PATH = "zephyr/zephyr.hex"

    def __init__(self, name, path):
        self.name = name
        self.root_path = path
        self.build_path = os.path.join(path, self.Constants.BUILD_PATH)
        self.hex_path = os.path.join(self.build_path, self.Constants.HEX_PATH)

    def gen_make(self, options=''):
        """
        Generate make files if build directory missing or empty
        """
        # Create build directory if doesn't exist
        if not os.path.exists(self.build_path):
            logging.info('Creating dir: {}'.format(self.build_path))
            os.makedirs(self.build_path)

        # Ignore if build directory is not empty
        if os.listdir(self.build_path):
            return

        # Exit if no board defined
        if KnotSDK().board is None:
            logging.critical('Error: No target board defined')
            logging.info('You need to provide a target board')
            exit()

        logging.warn("Build directory is empty")
        logging.info("Creating make files for {} App".format(self.name))

        # Get board Id from alias
        board_id = KnotSDK().Constants.BOARD_IDS[KnotSDK().board]
        cmd = 'cmake -DBOARD={} {} {}'.format(board_id,
                                              options,
                                              self.root_path)
        run_cmd(cmd, workdir=self.build_path)
        logging.info('Created make files for {} App'.format(self.name))

    def make(self, options):
        # Generate build environment
        self.gen_make(options=options)

        # make
        logging.info('Building {} App ...'.format(self.name))
        cmd = 'make -C {}'.format(self.build_path)
        run_cmd(cmd, workdir=self.build_path)
        logging.info('{} App built'.format(self.name))

        # Check for hex file
        if not os.path.isfile(self.hex_path):
            logging.critical(
                'Error: No hex file found at {}'.format(self.hex_path))
            exit()
        else:
            logging.info('Hex file generated at {}'.format(self.hex_path))

    def menuconfig(self, options):
        # Generate build environment
        self.gen_make(options=options)

        # Open menuconfig
        logging.info('Opening {} App menuconfig ...'.format(self.name))
        cmd = 'make menuconfig -C {}'.format(self.build_path)
        run_cmd(cmd, workdir=self.build_path)


    def clean(self):
        logging.info('Clearing {} App'.format(self.name))
        if os.path.exists(self.build_path):
            shutil.rmtree(self.build_path)


class KnotSDK(metaclass=Singleton):
    """
    Specific operations related to the KNoT Zephyr SDK
    """
    knot_path = ""  # Common directory and files used by images
    ext_ot_path = None  # External OpenThread repository
    cwd = ""  # Current working directory from where cli was called
    setup_app = None  # Setup app
    main_app = None  # Main app
    merged_hex_path = None  # Path to hex file merged from main and setup app
    signed_hex_path = None  # Path to signed merged hex file
    board = None  # Target board
    quiet = False  # Suppress command sub-process successful outputs if set
    debug_app = None  # Compile app in debug mode

    class Constants:
        KNOT_BASE_VAR = "KNOT_BASE"
        CORE_PATH = "core"
        SETUP_PATH = "setup"
        BUILD_PATH = "build"
        MERGED_HEX_PATH = "apps.hex"
        SIGNED_HEX_PATH = "sgn_apps.hex"
        FULL_HEX_PATH = "boot_sgn_apps.hex"
        DFU_PACKAGE_PATH = "dfu_package.zip"
        IMG_PATH = "img"
        BOARD_DK_ALIAS = 'dk'
        BOARD_DONGLE_ALIAS = 'dongle'
        MCUBOOT_STOCK_FILES = {BOARD_DK_ALIAS:     'mcuboot-ec-p256-dk.hex',
                               BOARD_DONGLE_ALIAS: 'mcuboot-ec-p256-dongle.hex'
                               }
        # Board Ids used for compiling
        BOARD_IDS = {BOARD_DK_ALIAS:      'nrf52840_pca10056',
                     BOARD_DONGLE_ALIAS:  'nrf52840_pca10059'}
        NRFUTIL_PATH = "third_party/pc-nrfutil/nrfutil"
        SIGN_SCRIPT_PATH = "third_party/mcuboot/scripts/imgtool.py"
        SIGN_KEY_PATH = "third_party/mcuboot/root-ec-p256.pem"
        SIGN_HEADER_SIZE = "0x200"
        SIGN_IMG_ALIGN = "8"
        SIGN_SCRIPT_VERSION = "1.5"
        SIGN_IMG_SLOT_SIZE = "0x69000"
        CMAKE_KNOT_DEBUG = "KNOT_DEBUG"
        CONFIG_PATH = "config"
        USB_HID_RE = "VID:PID={}:{}"  # USB Vendor Id and Product Id regex
        USB_HID_NORDIC_OPEN_DFU = ('1915', '521F')

    def __init__(self):
        self.check_env()
        self.__def_paths()

    def apps_init(self):
        # Setup
        setup_path = os.path.join(self.knot_path, self.Constants.SETUP_PATH)
        self.setup_app = App("Setup", setup_path)
        # Main
        main_path = self.cwd
        self.main_app = App("Main", main_path)

    def check_env(self):
        if self.Constants.KNOT_BASE_VAR not in os.environ:
            logging.critical(
                '"${}" not found'.format(self.Constants.KNOT_BASE_VAR))
            logging.info('Run: $ source <knot-zephyr-sdk-path>/knot-env.sh')
            exit()
        self.knot_path = os.environ[self.Constants.KNOT_BASE_VAR]
        logging.info('Using KNoT base path: ' + self.knot_path)

        # Get current working directory
        self.cwd = os.getcwd()

    def __def_paths(self):
        """
        Define SDK required paths
        """
        self.signed_hex_path = os.path.join(self.knot_path,
                                            self.Constants.BUILD_PATH,
                                            self.Constants.SIGNED_HEX_PATH)
        self.merged_hex_path = os.path.join(self.knot_path,
                                            self.Constants.BUILD_PATH,
                                            self.Constants.MERGED_HEX_PATH)
        self.full_hex_path = os.path.join(self.knot_path,
                                          self.Constants.BUILD_PATH,
                                          self.Constants.FULL_HEX_PATH)
        self.dfu_package_path = os.path.join(self.knot_path,
                                             self.Constants.BUILD_PATH,
                                             self.Constants.DFU_PACKAGE_PATH)
        self.nrfutil_path = os.path.join(self.knot_path,
                                         self.Constants.NRFUTIL_PATH)

    def set_ext_ot_path(self, ot_path=None):
        """
        Use default external OpenThread path if none specified.
        In case of no default path defined, keep it as None.
        """
        # Use default is none is passed
        if ot_path is not None:
            self.ext_ot_path = ot_path
        else:
            self.ext_ot_path = Config().get(Config().KEY_OT_PATH)

        # Log in case of valid OT path passed
        if self.ext_ot_path is not None:
            logging.info('Using OT path: {}'.format(self.ext_ot_path))
        else:
            logging.info('No OpenThread path passed')

    def set_board(self, board=None):
        """
        Use default board if none specified.
        In case of no default board defined, an "Undefined board" error will be
        raised.
        """
        logging.info('Selected board: {}'.format(board))

        # Use default board if none passed
        if board is None:
            self.board = Config().get(Config().KEY_BOARD)
        else:
            self.board = board

        # Abort in case of no board set
        if self.board in [None, '']:
            logging.critical('Error: No target board defined')
            logging.info("To define a board, use '--board <TARGET BOARD>'")
            exit()

        # Abort in case of invalid board set
        valid_boards = self.Constants.BOARD_IDS.keys()
        if self.board not in valid_boards:
            logging.critical('Error: Invalid board set')
            logging.info("The valid boards are: " +
                         ', '.join(list(valid_boards)))
            exit()

    def set_quiet(self, quiet):
        self.quiet = quiet

    def set_debug(self, debug):
        self.debug_app = debug

    def find_flash_dev(self, vid, pid):
        """
        Return device port based on Vendor Id and Product Id.
        Abort if no board or more than one found.
        """
        # Get ports that match the desired Hardware Id
        re = self.Constants.USB_HID_RE.format(vid, pid)
        ports = list(serial.tools.list_ports.grep(re))

        # Abort if no board found
        if not ports:
            logging.critical('Error: No target board connected!')
            logging.info('Make sure the board is connected and programable')
            exit()

        # Abort if more than one boards found
        if len(ports) > 1:
            logging.critical('Error: Too many target boards connected!')
            logging.info("Provide the target device with '--port <PORT>' \
or remove the other boards")
            exit()

        # Return single device
        return ports[0].device

    def __flash(self, file_path, port):
        # Use nrfjprog for DK and nrfutil for Dongle
        if self.board == self.Constants.BOARD_DK_ALIAS:
            cmd = 'nrfjprog --program ' + file_path + \
                  ' --sectorerase -f nrf52 --reset'

            logging.info('Flashing file ' + file_path)
            run_cmd(cmd)

        elif self.board == self.Constants.BOARD_DONGLE_ALIAS:
            # Abort if device port is not set
            if port is None:
                port = self.find_flash_dev(
                                       *self.Constants.USB_HID_NORDIC_OPEN_DFU)

            # Create DFU package
            cmd = '{} pkg generate --hw-version 52 --sd-req=0x00  \
--application {} --application-version 1 {}'.format(
                   self.nrfutil_path, file_path, self.dfu_package_path)
            logging.info('Creating DFU package for file ' + file_path)
            run_cmd(cmd, quiet=True)

            # Flash DFU package
            logging.info('Flashing DFU package')
            cmd = '{} dfu usb-serial -pkg {} -p {}'.format(
                   self.nrfutil_path, self.dfu_package_path, port)
            run_cmd(cmd)

    def flash_mcuboot(self, port):
        logging.info('Flashing stock MCUBOOT...')
        # Get target MCUBOOT file based on board
        mcuboot_file = self.Constants.MCUBOOT_STOCK_FILES[self.board]
        mcuboot_path = os.path.join(self.knot_path,
                                    self.Constants.IMG_PATH,
                                    mcuboot_file)
        self.__flash(mcuboot_path, port)
        logging.info('MCUBOOT flashed')

    def flash_prj(self, full, port):
        """
        Flash signed image to board. If full flag is set, flash
        mcuboot + signed image merged file.
        """
        # Flash full image if board is dongle
        if full or self.board == self.Constants.BOARD_DONGLE_ALIAS:
            logging.info("Flashing apps with mcuboot")
            target_path = self.full_hex_path
        else:
            logging.info("Flashing apps")
            target_path = self.signed_hex_path

        # Abort if no flash image found
        if not os.path.exists(target_path):
            logging.critical('Target image not found!')
            logging.info("To create an image, run 'cli.py make'")
            exit()

        logging.info('Flashing device...')
        self.__flash(target_path, port)
        logging.info('Board flashed')

    def get_setup_options(self):
        """
        Return build options for setup app
        """
        # Debug mode
        if self.debug_app:
            return '-D{}=y'.format(KnotSDK().Constants.CMAKE_KNOT_DEBUG)
        else:
            return ''

    def make_setup(self):
        """
        Generate build environment if doesn't exist and Make setup app
        """
        opt = self.get_setup_options()
        self.setup_app.make(opt)

    def get_main_options(self):
        """
        Create build folder for main app
        """
        # Use external OpenThread path if provided
        if self.ext_ot_path is not None:
            opt =\
                '-DEXTERNAL_PROJECT_PATH_OPENTHREAD={}'.format(
                    self.ext_ot_path)
        else:
            opt = ''

        # Debug mode
        if self.debug_app:
            opt += ' -D{}=y'.format(KnotSDK().Constants.CMAKE_KNOT_DEBUG)

        return opt

    def make_main(self):
        """
        Generate build environment if doesn't exist and Make setup app
        """
        opt = self.get_main_options()
        self.main_app.make(opt)

    def clear_core_dir(self):
        """
        Clear build directory
        """
        core_dir = os.path.join(self.knot_path,
                                self.Constants.BUILD_PATH)
        logging.info('Deleting {}'.format(core_dir))
        if os.path.exists(core_dir):
            shutil.rmtree(core_dir)

        core_build_dir = os.path.join(self.knot_path,
                                self.Constants.CORE_PATH,
                                self.Constants.BUILD_PATH)
        logging.info('Deleting {}'.format(core_build_dir))
        if os.path.exists(core_build_dir):
            shutil.rmtree(core_build_dir)


    def make_core_dir(self):
        core_dir = os.path.join(self.knot_path,
                                self.Constants.BUILD_PATH)
        logging.info('Creating {}'.format(core_dir))
        os.mkdir(core_dir)

    def merge_imgs(self, input_1, input_2, output):
        """
        Merge hex inputs to hex output
        """
        # Output hex at merge_hex_path
        cmd = 'mergehex -m {} {} -o {}'.format(input_1, input_2, output)
        run_cmd(cmd)
        if not os.path.isfile(output):
            logging.critical('Error: No hex file found at {}'.format(output))
            logging.info('Clear the building files and try again')
            exit()
        else:
            logging.info('Hex file generated at {}'.format(output))

    def merge_setup_main(self):
        """
        Merge main and setup apps
        """
        logging.info('Merging main and setup apps')
        self.merge_imgs(self.setup_app.hex_path,
                        self.main_app.hex_path,
                        self.merged_hex_path)

    def sign_merged(self):
        """
        Sign merged hex file
        """
        # Sign merged image using imgtool script
        imgtool_path = os.path.join(self.knot_path,
                                    self.Constants.SIGN_SCRIPT_PATH)
        key_path = os.path.join(self.knot_path,
                                self.Constants.SIGN_KEY_PATH)

        cmd = ('{} sign'.format(imgtool_path) +
               ' --key {}'.format(key_path) +
               ' --header-size {}'.format(self.Constants.SIGN_HEADER_SIZE) +
               ' --align {}'.format(self.Constants.SIGN_IMG_ALIGN) +
               ' --version {}'.format(self.Constants.SIGN_SCRIPT_VERSION) +
               ' --slot-size {}'.format(self.Constants.SIGN_IMG_SLOT_SIZE) +
               ' --pad' +
               ' {}'.format(self.merged_hex_path) +
               ' {}'.format(self.signed_hex_path))
        logging.info('Signing merged hex file')
        run_cmd(cmd)
        if not os.path.isfile(self.signed_hex_path):
            exit('Error: No hex file found at {}'.format(self.signed_hex_path))
        else:
            logging.info('Hex file generated at {}'.format(
                self.signed_hex_path))

    def gen_full_img(self):
        """
        Generate full image by merging signed apps image and mcuboot.
        """
        # Get mcuboot path based on target board
        mcuboot_file = self.Constants.MCUBOOT_STOCK_FILES[self.board]
        mcuboot_path = os.path.join(self.knot_path,
                                    self.Constants.IMG_PATH,
                                    mcuboot_file)

        logging.info('Merging apps to mcuboot')
        self.merge_imgs(self.signed_hex_path,
                        mcuboot_path,
                        self.full_hex_path)

    def erase_thing(self):
        """
        Clear flash
        """
        cmd = 'nrfjprog --eraseall'
        logging.info('Erasing KNoT Thing memory')
        run_cmd(cmd)
        logging.info('KNoT Thing memory erased')

    def clean(self):
        logging.info('Deleting building files...')
        self.setup_app.clean()
        self.main_app.clean()
        self.clear_core_dir()


class Config(metaclass=Singleton):
    """
    Stored configuration proxy and handler.
    """
    config_path = None
    parser = None

    # Constants
    SECTION = "DEFAULT"
    KEY_BOARD = "board"
    KEY_OT_PATH = "ot_path"

    def __init__(self):
        self.load()

    def load(self):
        try:
            self.config_path = os.path.join(KnotSDK().knot_path,
                                            KnotSDK().Constants.CONFIG_PATH)
            self.parser = ConfigParser()
            self.parser.read(self.config_path)
        except Exception as err:
            logging.critical("Failed to read config file with exception:")
            logging.info(str(err))
            exit()

    def get(self, key):
        """
        Read value with specified key provided by config file.
        Use default section for all keys.
        Return None if not set.
        """
        return self.parser.get(self.SECTION, key, fallback=None)

    def set(self, key, value):
        """
        Set value with specified key to config file as a string.
        Use default section for all keys.
        """
        self.parser[self.SECTION][key] = str(value)

        with open(self.config_path, 'w') as configfile:
            self.parser.write(configfile)


def run_cmd(cmd, workdir=KnotSDK().cwd, quiet=False):
    """
    Run subprocess and get raise error if any
    """
    # Pipe outputs if quiet selected
    if KnotSDK().quiet or quiet:
        out_pipe = subprocess.PIPE
        err_pipe = subprocess.PIPE
    else:
        out_pipe = None
        err_pipe = None

    logging.info('Executing command: "{}"'.format(cmd))
    logging.info('Used working directory: "{}"'.format(workdir))
    process = subprocess.Popen(cmd.split(),
                               stdout=out_pipe,
                               stderr=err_pipe,
                               cwd=workdir)

    # Wait for process to run
    out, err = process.communicate()

    rc = process.returncode

    # Successful execution
    if rc == 0:
        return

    # Log error
    logging.critical('Process execution failed')
    logging.info('cmd: {}'.format(cmd))
    logging.info('Return code: {}'.format(rc))

    # Log process output if not printed already
    if KnotSDK().quiet or quiet:
        if out is not None:
            logging.info('Process stdout: \n{}'.format(out.decode('utf_8')))
        if err is not None:
            logging.info('Process stderr: \n{}'.format(err.decode('utf_8')))

    exit()


@click.group()
def cli():
    """
    Command line interface components
    """
    try:
        KnotSDK()
        Config()
    except KeyError as err:
        sys.exit(err)


@cli.group(help='Build setup and main apps', invoke_without_command=True)
@click.pass_context
@click.option('--ot_path', help='Define OpenThread repository path')
@click.option('-q', '--quiet', help='Suppress successful sub-command messages',
              is_flag=True)
@click.option('-b', '--board', help='Target board')
@click.option('-d', '--debug', help='Build app in debug mode', is_flag=True)
@click.option('-f', '--flash', help='Flash apps after build', is_flag=True)
@click.option('-c', '--clean', help='Clean before build', is_flag=True)
@click.option('-m', '--mcuboot', help='Flash apps and mcuboot after build',
              is_flag=True)
@click.option('-p', '--port', help='Device port')
def make(ctx, ot_path, quiet, board, debug, flash, clean, mcuboot, port):
    if quiet:
        KnotSDK().set_quiet(True)

    if debug:
        KnotSDK().set_debug(True)

    # Initialize App objects
    KnotSDK().apps_init()

    # Clean before build if flagged to
    if clean:
        KnotSDK().clean()

    # Don't build apps if using any subcommand
    if ctx.invoked_subcommand:
        return

    # Optional external OpenThread path
    KnotSDK().set_ext_ot_path(ot_path=ot_path)

    # Defined board required
    KnotSDK().set_board(board=board)

    # Make apps
    KnotSDK().make_setup()
    KnotSDK().make_main()

    # Merge and Sign apps
    KnotSDK().clear_core_dir()
    KnotSDK().make_core_dir()
    KnotSDK().merge_setup_main()
    KnotSDK().sign_merged()
    KnotSDK().gen_full_img()

    # Flash if flagged to
    if flash or mcuboot:
        KnotSDK().flash_prj(mcuboot, port)


@make.command(help='Open menuconfig for main app')
@click.option('-b', '--board', help='Target board')
def menuconfig(board):
    # Defined board required
    KnotSDK().set_board(board=board)

    opt = KnotSDK().get_main_options()
    KnotSDK().main_app.menuconfig(opt)


@cli.command(help='Delete building files')
def clean():
    # Initialize App objects so they can be cleared
    KnotSDK().apps_init()

    # Clear all building files
    KnotSDK().clean()


@cli.command(help='Erase flash memory')
def erase():
    KnotSDK().erase_thing()


@cli.command(help='Flash stock MCUBOOT')
@click.option('-b', '--board', help='Target board')
@click.option('-p', '--port', help='Device port')
def mcuboot(board, port):
    # Defined board required
    KnotSDK().set_board(board)

    KnotSDK().flash_mcuboot(port)


@cli.command(help='Flash Setup and Main apps')
@click.option('-m', '--mcuboot', help='Flash mcuboot too', is_flag=True)
@click.option('-b', '--board', help='Target board')
@click.option('-p', '--port', help='Device port')
def flash(mcuboot, board, port):
    # Defined board required
    KnotSDK().set_board(board)

    KnotSDK().flash_prj(mcuboot, port)


@cli.group(help='Set config default values')
def set():
    pass


@set.command(help='Default target board')
@click.argument('board')
def board(board):
    KnotSDK().set_board(board)
    Config().set(Config().KEY_BOARD, board)


@set.command(help='Default external OpenThread path')
@click.argument('ot_path')
def ot_path(ot_path):
    Config().set(Config().KEY_OT_PATH, ot_path)


if __name__ == '__main__':
    """
    Run cli
    """
    # Use logging
    log_format = '%(asctime)s [%(levelname)s]: %(message)s'
    coloredlogs.install(fmt=log_format)  # Enable colored logging

    cli()  # Run command line interface

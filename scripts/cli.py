#!/usr/bin/env python3
#
# Copyright (c) 2019, CESAR. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os
import shutil
import errno
import sys
import click
import subprocess
import logging
import coloredlogs


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

    def make(self, options=''):
        # Create build directory if doesn't exist
        if not os.path.exists(self.build_path):
            logging.info('Creating dir: {}'.format(self.build_path))
            os.makedirs(self.build_path)

        # Cmake
        if not os.listdir(self.build_path):
            logging.warn("Build directory is empty")
            logging.info("Creating make files for {} App".format(self.name))
            cmd = 'cmake -DBOARD={} {} {}'.format(KnotSDK().Constants.BOARD,
                                                  options,
                                                  self.root_path)
            run_cmd(cmd, workdir=self.build_path)
            logging.info('Created make files for {} App'.format(self.name))

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
    quiet = False  # Suppress command sub-process successful outputs if set

    class Constants:
        KNOT_BASE_VAR = "KNOT_BASE"
        BOARD = "nrf52840_pca10056"
        CORE_PATH = "core"
        SETUP_PATH = "setup"
        BUILD_PATH = "build"
        MERGED_HEX_PATH = "merged.hex"
        SIGNED_HEX_PATH = "signed.hex"
        IMG_PATH = "img"
        OT_CONFIG_PATH = "overlay-knot-ot.conf"
        MCUBOOT_STOCK_FILE = "mcuboot-ec-p256.hex"
        SIGN_SCRIPT_PATH = "third_party/mcuboot/scripts/imgtool.py"
        SIGN_KEY_PATH = "third_party/mcuboot/root-ec-p256.pem"
        SIGN_HEADER_SIZE = "0x200"
        SIGN_IMG_ALIGN = "8"
        SIGN_SCRIPT_VERSION = "1.5"
        SIGN_IMG_SLOT_SIZE = "0x69000"

    def __init__(self):
        self.check_env()

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

    def set_ext_ot_path(self, ot_path):
        logging.info('Using OT path: {}'.format(ot_path))
        self.ext_ot_path = ot_path

    def set_quiet(self, quiet):
        self.quiet = quiet

    def __flash(self, file_path):
        logging.info('Flashing file ' + file_path)
        cmd = 'nrfjprog --program ' + \
              file_path + \
              ' --sectorerase -f nrf52 --reset'
        run_cmd(cmd)

    def flash_mcuboot(self):
        logging.info('Flashing stock MCUBOOT...')
        self.__flash(os.path.join(self.knot_path,
                                  self.Constants.IMG_PATH,
                                  self.Constants.MCUBOOT_STOCK_FILE))
        logging.info('MCUBOOT flashed')

    def flash_signed(self):
        logging.info('Flashing signed image...')
        self.__flash(self.signed_hex_path)
        logging.info('Signed image flashed')

    def make_setup(self):
        """
        Create build folder and make setup app
        """
        self.setup_app.make()

    def make_main(self):
        """
        Create build folder and make main app
        """
        # Define overlay file to be used
        overlay_path = os.path.join(self.knot_path,
                                    self.Constants.CORE_PATH,
                                    self.Constants.OT_CONFIG_PATH)
        overlay_config = '-DOVERLAY_CONFIG={}'.format(overlay_path)

        # Use external OpenThread path if provided
        if self.ext_ot_path is not None:
            external_ot =\
                '-DEXTERNAL_PROJECT_PATH_OPENTHREAD={}'.format(
                    self.ext_ot_path)
        else:
            external_ot = ''

        # Build main app with compiling options
        opt = '{} {}'.format(overlay_config, external_ot)
        self.main_app.make(options=opt)

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

    def merge_setup_main(self):
        """
        Merge main and setup apps
        """
        self.merged_hex_path = os.path.join(self.knot_path,
                                            self.Constants.BUILD_PATH,
                                            self.Constants.MERGED_HEX_PATH)

        # Output hex at merge_hex_path
        cmd = 'mergehex -m {} {} -o {}'.format(self.setup_app.hex_path,
                                               self.main_app.hex_path,
                                               self.merged_hex_path)
        logging.info('Merging main and setup apps')
        run_cmd(cmd)
        if not os.path.isfile(self.merged_hex_path):
            exit('Error: No hex file found at {}'.format(self.merged_hex_path))
        else:
            logging.info('Hex file generated at {}'.format(
                self.merged_hex_path))

    def sign_merged(self):
        """
        Sign merged hex file
        """
        self.signed_hex_path = os.path.join(self.knot_path,
                                            self.Constants.BUILD_PATH,
                                            self.Constants.SIGNED_HEX_PATH)

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

    def erase_thing(self):
        """
        Clear flash
        """
        cmd = 'nrfjprog --eraseall'
        logging.info('Erasing KNoT Thing memory')
        run_cmd(cmd)
        logging.info('KNoT Thing memory erased')


def run_cmd(cmd, workdir=KnotSDK().cwd):
    """
    Run subprocess and get raise error if any
    """
    # Pipe outputs if quiet selected
    if KnotSDK().quiet:
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
    if KnotSDK().quiet:
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
    except KeyError as err:
        sys.exit(err)


@cli.group(help='Build setup and main apps', invoke_without_command=True)
@click.pass_context
@click.option('--ot_path', help='Define OpenThread repository path')
@click.option('-q', '--quiet', help='Suppress successful sub-command messages',
              is_flag=True)
def make(ctx, ot_path, quiet):
    if quiet:
        KnotSDK().set_quiet(True)

    # Initialize App objects
    KnotSDK().apps_init()

    if ctx.invoked_subcommand == "clean":
        return

    if ot_path is not None:
        KnotSDK().set_ext_ot_path(ot_path)

    # Make apps
    KnotSDK().make_setup()
    KnotSDK().make_main()

    # Merge and Sign apps
    KnotSDK().clear_core_dir()
    KnotSDK().make_core_dir()
    KnotSDK().merge_setup_main()
    KnotSDK().sign_merged()


@make.command(help='Flash setup and main apps')
def flash():
    KnotSDK().flash_signed()


@make.command(help='Delete building files')
def clean():
    logging.info('Deleting building files...')
    KnotSDK().setup_app.clean()
    KnotSDK().main_app.clean()
    KnotSDK().clear_core_dir()


@cli.command(help='Erase flash memory')
def erase():
    KnotSDK().erase_thing()


@cli.command(help='Flash stock MCUBOOT')
def mcuboot():
    KnotSDK().flash_mcuboot()

if __name__ == '__main__':
    """
    Run cli
    """
    # Use logging
    log_format = '%(asctime)s [%(levelname)s]: %(message)s'
    coloredlogs.install(fmt=log_format)  # Enable colored logging

    cli()  # Run command line interface

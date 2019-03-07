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


"""
 Singleton class to garrantee that a single instance will be used for
 its inhereted classes
"""
class Singleton(type):
    __instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls.__instances:
            cls.__instances[cls] = super(Singleton,
                                         cls).__call__(*args, **kwargs)
        return cls.__instances[cls]


"""
 Specific operations related to the KNoT Zephyr SDK
"""
class KnotSDK(metaclass=Singleton):
    knot_path = "" # Common directory and files used by images
    ext_ot_path = None # External OpenThread repository
    cwd = "" # Current working directory from where cli was called
    setup_hex_path = None # Path to generated hex path for setup app
    main_hex_path = None # Path to generated hex path for main app
    merged_hex_path = None # Path to hex file merged from main and setup app
    signed_hex_path = None # Path to signed merged hex file

    class Constants:
        KNOT_BASE_VAR = "KNOT_BASE"
        BOARD = "nrf52840_pca10056"
        CORE_PATH = "core"
        SETUP_PATH = "setup"
        BUILD_PATH = "build"
        HEX_PATH = "zephyr/zephyr.hex"
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

    def check_env(self):
        if self.Constants.KNOT_BASE_VAR not in os.environ:
            print('KNoT base path not found.')
            print('Run: $ source <knot-zephyr-sdk-path>/knot-env.sh')
            exit('"$' + self.Constants.KNOT_BASE_VAR +'" not found')
        self.knot_path = os.environ[self.Constants.KNOT_BASE_VAR]
        print('Using KNoT base path: ' + self.knot_path)

        # Get current working directory
        self.cwd = os.getcwd()

    def set_ext_ot_path(self, ot_path):
        print('Using OT path: {}'.format(ot_path))
        self.ext_ot_path = ot_path

    def __flash(self, file_path):
        # TODO
        print('Flashing file ' + file_path)
        cmd = 'nrfjprog --program ' + \
              file_path + \
              ' --sectorerase -f nrf52 --reset'
        run_cmd(cmd)


    def flash_mcuboot(self):
        print('Flashing stock MCUBOOT...')
        self.__flash(os.path.join(self.knot_path,
                                  self.Constants.IMG_PATH,
                                  self.Constants.MCUBOOT_STOCK_FILE))
        print('MCUBOOT flashed')

    """
    Create build folder at path if it doesn't exists.
    Returs True if directory already exists.
    """
    def create_build(self, path):
        if not os.path.exists(path):
            os.makedirs(path)
            print('Creating dir: {}'.format(path))

    def make_app(self, app_path, app_name, options=''):
        build_path = os.path.join(app_path, self.Constants.BUILD_PATH)

        # Build directory
        self.create_build(build_path)

        # Cmake
        if not os.listdir(build_path):
            print("Build directory is empty")
            print("Creating make files for {} App".format(app_name))
            cmd = 'cmake -DBOARD={} {} {}'.format(KnotSDK().Constants.BOARD,
                                               options,
                                               app_path)
            run_cmd(cmd, workdir=build_path)
            print('Created make files for {} App'.format(app_name))

        # make
        print('Building {} App ...'.format(app_name))
        cmd = 'make -C {}'.format(build_path)
        run_cmd(cmd, workdir=build_path)
        print('{} App built'.format(app_name))

        # Check for hex file
        hex_path = os.path.join(build_path, self.Constants.HEX_PATH)
        if not os.path.isfile(hex_path):
            exit('Error: No hex file found at {}'.format(hex_path))
        else:
            print('Hex file generated at {}'.format(hex_path))
            return hex_path


    """
    Create build folder and make setup app
    """
    def make_setup(self):
        setup_path = os.path.join(self.knot_path, self.Constants.SETUP_PATH)
        self.setup_hex_path = self.make_app(setup_path, "Setup")

    """
    Create build folder and make main app
    """
    def make_main(self):
        # Define overlay file to be used
        overlay_path = os.path.join(self.knot_path,
                                    self.Constants.CORE_PATH,
                                    self.Constants.OT_CONFIG_PATH)
        overlay_config = '-DOVERLAY_CONFIG={}'.format(overlay_path)

        # Use external OpenThread path if provided
        if self.ext_ot_path is not None:
            external_ot =\
                '-DEXTERNAL_PROJECT_PATH_OPENTHREAD={}'.format(self.ext_ot_path)
        else:
            external_ot = ''

        # Build main app with compiling options
        opt = '{} {}'.format(overlay_config, external_ot)
        self.main_hex_path = self.make_app(self.cwd, "Main", options=opt)

    """
    Clear build directory
    """
    def clear_core_work(self):
        core_work_path = os.path.join(self.knot_path,
                                      self.Constants.BUILD_PATH)
        print('Deleting {}'.format(core_work_path))
        if os.path.exists(core_work_path):
            shutil.rmtree(core_work_path)

        print('Creating {}'.format(core_work_path))
        os.mkdir(core_work_path)

    """
    Merge main and setup apps
    """
    def merge_setup_main(self):
        self.merged_hex_path = os.path.join(self.knot_path,
                                       self.Constants.BUILD_PATH,
                                       self.Constants.MERGED_HEX_PATH)

        # Output hex at merge_hex_path
        cmd = 'mergehex -m {} {} -o {}'.format(self.setup_hex_path,
                                            self.main_hex_path,
                                            self.merged_hex_path)
        print('Merging main and setup apps')
        run_cmd(cmd)
        if not os.path.isfile(self.merged_hex_path):
            exit('Error: No hex file found at {}'.format(self.merged_hex_path))
        else:
            print('Hex file generated at {}'.format(self.merged_hex_path))

    """
    Sign merged hex file
    """
    def sign_merged(self):
        self.signed_hex_path = os.path.join(self.knot_path,
                                            self.Constants.BUILD_PATH,
                                            self.Constants.SIGNED_HEX_PATH)

        # Sign merged image using imgtool script
        imgtool_path = os.path.join(self.knot_path,
                                    self.Constants.SIGN_SCRIPT_PATH)
        key_path = os.path.join(self.knot_path,
                                self.Constants.SIGN_KEY_PATH)

        cmd = ('{} sign'.format(imgtool_path)
               + ' --key {}'.format(key_path)
               + ' --header-size {}'.format(self.Constants.SIGN_HEADER_SIZE)
               + ' --align {}'.format(self.Constants.SIGN_IMG_ALIGN)
               + ' --version {}'.format(self.Constants.SIGN_SCRIPT_VERSION)
               + ' --slot-size {}'.format(self.Constants.SIGN_IMG_SLOT_SIZE)
               + ' --pad'
               + ' {}'.format(self.merged_hex_path)
               + ' {}'.format(self.signed_hex_path))
        print('Signing merged hex file')
        run_cmd(cmd)
        if not os.path.isfile(self.signed_hex_path):
            exit('Error: No hex file found at {}'.format(self.signed_hex_path))
        else:
            print('Hex file generated at {}'.format(self.signed_hex_path))

"""
 Run subprocess and get raise error if any
"""
def run_cmd(cmd, workdir=KnotSDK().cwd):
    print('Executing command: "{}"\nfrom: {}\n'.format(cmd, workdir))
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE,
                                            cwd=workdir)

    # Wait for process to run
    out, err = process.communicate()

    rc = process.returncode

    # Successful execution
    if rc == 0:
        return

    # Log error
    proc_log = ''
    if out is not None:
        proc_log += 'stdout: {}'.format(out.decode('utf_8'))
    if err is not None:
        proc_log += 'stderr: {}'.format(err.decode('utf_8'))

    err_msg = 'Process execution failed\n' + \
              'cmd: {}\n' + \
              'Return code: {}\n' + \
              'Process output: \n{}\n'
    exit(err_msg.format(cmd, rc, proc_log))


"""
 Command line interface components
"""
@click.group()
def cli():
    try:
        KnotSDK()
    except KeyError as err:
        sys.exit(err)

@cli.group(help='Build setup and main apps', invoke_without_command=True)
@click.pass_context
@click.option('--ot_path', help='Define OpenThread repository path')
def make(ctx, ot_path):
    if ctx.invoked_subcommand == "clean":
        return

    if ot_path is not None:
        KnotSDK().set_ext_ot_path(ot_path)

    # Make apps
    KnotSDK().make_setup()
    KnotSDK().make_main()

    # Merge and Sign apps
    KnotSDK().clear_core_work()
    KnotSDK().merge_setup_main()
    KnotSDK().sign_merged()

@make.command(help='Flash setup and main apps')
def flash():
    # TODO
    print('Flashing setup and main apps...')

@make.command(help='Delete building files')
def clean():
    # TODO
    print('Deleting building files...')

@cli.command(help='Erase flash memory')
def erase():
    # TODO
    click.echo('Erasing KNoT Thing memory...')

@cli.command(help='Flash stock MCUBOOT')
def mcuboot():
   KnotSDK().flash_mcuboot()


"""
 Run cli
"""
if __name__ == '__main__':
    cli()

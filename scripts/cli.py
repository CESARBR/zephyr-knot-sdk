#!/usr/bin/env python3
#
# Copyright (c) 2019, CESAR. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os
import errno
import sys
import click
import subprocess


"""
 Custom exception for error during command execution
"""
class ProcExecErr(Exception):
    def __init__(self, cmd, message, error_code):
        super(ProcExecErr, self).__init__(message)
        self.cmd = cmd
        self.error = error_code
        self.message = message

    def __str__(self):
        ret = '<{}>\nProcess execution fail\n' + \
              'cmd: {}\n' + \
              'Return code: {}\n' + \
              'Process output: \n{}\n'
        return ret.format(type(self).__name__,
                          self.cmd ,self.error, self.message)


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
    cwd = "" # Current working directory from where cli was called

    class Constants:
        KNOT_BASE_VAR = "KNOT_BASE"
        BOARD = "nrf52840_pca10056"
        SETUP_PATH = "setup"
        BUILD_PATH = "build"
        IMG_PATH = "img"
        MCUBOOT_STOCK_FILE = "mcuboot-ec-p256.hex"

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

    def __flash(self, file_path):
        # TODO
        print('Flashing file ' + file_path)
        cmd = 'nrfjprog --program ' + \
              file_path + \
              ' --sectorerase -f nrf52 --reset'
        run_cmd(cmd)


    def flash_mcuboot(self):
        print('Flashing stock MCUBOOT...')
        try:
            self.__flash(os.path.join(self.knot_path,
                                    self.Constants.IMG_PATH,
                                    self.Constants.MCUBOOT_STOCK_FILE))
            print('MCUBOOT flashed')
        except ProcExecErr as e:
            print('Failed to flash MCUBOOT.\nError: {}'.format(e))

    """
    Create build folder at path if it doesn't exists.
    Returs True if directory already exists.
    """
    def create_build(self, path):
        try:
            os.makedirs(path)
            print('Create dir: {}'.format(path))
            return False
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
            return True

    """
    Create build folder and make setup app
    """
    def make_setup(self):
        setup_path = os.path.join(self.knot_path, self.Constants.SETUP_PATH)
        build_path = os.path.join(setup_path, self.Constants.BUILD_PATH)

        # Build directory
        self.create_build(build_path)

        # Cmake
        if not os.listdir(build_path):
            print("Build directory is empty")
            print("Creating make files for Setup App")
            cmd = 'cmake -DBOARD={} {}'.format(KnotSDK().Constants.BOARD,
                                               setup_path)
            try:
                run_cmd(cmd, workdir=build_path)
                print('Created make files for Setup App')
            except ProcExecErr as e:
                print('Failed to create make file for Setup App')
                print('Error: {}'.format(e))
                exit(e.error)
        #TODO
        print('Building setup app...')


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

    if rc == 0:
        return

    # Raise exception if error
    excep_msg = ''
    if out is not None:
        excep_msg += 'stdout: {}'.format(out.decode('ascii'))
    if err is not None:
        excep_msg += 'stderr: {}'.format(err.decode('ascii'))

    raise ProcExecErr(cmd, excep_msg, rc)


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
def make(ctx):
    if ctx.invoked_subcommand == "clean":
        return

    KnotSDK().make_setup()

    # TODO
    print('Building main app...')

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

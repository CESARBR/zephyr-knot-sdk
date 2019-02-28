#!/usr/bin/env python3
#
# Copyright (c) 2019, CESAR. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

from os import environ

import sys
import click


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

    class Constants:
        KNOT_BASE_VAR = "KNOT_BASE"

    def __init__(self):
        self.check_env()

    def check_env(self):
        if self.Constants.KNOT_BASE_VAR not in environ:
            print('KNoT base path not found.')
            print('Run: $ source <knot-zephyr-sdk-path>/knot-env.sh')
            raise KeyError('"$' + self.Constants.KNOT_BASE_VAR +'" not found')
        self.knot_path = environ[self.Constants.KNOT_BASE_VAR]
        print('Using KNoT base path: ' + self.knot_path)


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

    # TODO
    print('Building setup app...')

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
    # TODO
    click.echo('Flashing stock MCUBOOT...')


"""
 Run cli
"""
if __name__ == '__main__':
    cli()

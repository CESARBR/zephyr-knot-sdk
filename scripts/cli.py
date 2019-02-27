#!/usr/bin/env python3
#
# Copyright (c) 2019, CESAR. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import click


"""
 Command line interface components
"""
@click.group()
def cli():
    pass

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

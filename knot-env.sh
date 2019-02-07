#
# Copyright (c) 2018, CESAR. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

if [ -n "$ZSH_VERSION" ]; then
	DIR="${(%):-%N}"
	if [ $options[posixargzero] != "on" ]; then
		setopt posixargzero
		NAME=$(basename -- "$0")
		setopt posixargzero
	else
		NAME=$(basename -- "$0")
	fi
else
	DIR="${BASH_SOURCE[0]}"
	NAME=$(basename -- "$0")
fi

if [ "X$NAME" "==" "Xknot-env.sh" ]; then
    echo "Source this file (do NOT execute it!) to set the KNoT SDK environment."
    exit
fi

export KNOT_BASE=$( builtin cd "$( dirname "$DIR" )" > /dev/null && pwd )

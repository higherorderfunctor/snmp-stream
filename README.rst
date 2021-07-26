.. role:: bash(code)
   :language: bash

.. _snmp_fetch: https://github.com/higherorderfunctor/snmp-fetch

steam-stream
============

This package is an in-progress rewrite/refactor of the snmp_fetch_ SNMPv2 library.  While this package includes an improved implementation, it is currently missing critical components for a production library including documentation, testing with coverage, CI/CD, and packaging.

Quick Start
-----------

.. code:: bash

   # clone the repository
   git clone --recurse-submodules -j8 https://github.com/higherorderfunctor/snmp-stream.git
   cd snmp-stream

   # build the docker image
   docker build --target snmp-stream -t snmp-stream:latest .

   # start the docker container
   docker run \
     -it --name snmp-stream \
     # example: include any dotfiles from host
     -v $HOME/Documents/dotfiles:/root/dotfiles \
     # /example
     snmp-stream:latest /bin/sh

   # example: setup a development environment from host
   ln -s ~/dotfiles/.zshrc ~/
   ln -s ~/dotfiles/.tmux ~/
   ln -s ~/dotfiles/.tmux.conf ~/
   ln -s ~/dotfiles/.vim ~/
   ln -s ~/dotfiles/.vimrc ~/
   sh -c "$(curl -fsSL https://raw.github.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"
   exit
   sed -i 's/ash/zsh/' /etc/passwd
   tmux
   # /example

   # upgrade PIP and init the python virtual environment
   poetry run python -m pip install --upgrade pip

   # install dependencies and compile the module
   poetry install

   # optional: bootstrap environment for vim
   source bin/activate

   # testing
   poetry run pytest -v tests

   # linting
   poetry run isort snmp_stream tests
   poetry run pylint snmp_stream tests
   poetry run flake8 snmp_stream tests
   poetry run mypy -p snmp_stream -p tests
   poetry run bandit -r snmp_stream

   # start a virtual device
   snmpsimd.py --agent-udpv4-endpoint=127.0.0.1:1161 --process-user=root --process-group=root

   # query the virtual device
   "import numpy as np;from snmp_stream import *;response = walk('127.0.0.1:1161',('recorded/linux-full-walk', 'V2C'),['1.3.6.1.2.1.2.2.1.1','1.3.6.1.2.1.2.2.1.2'],req_id='abc',config={'retries': 1, 'timeout': 3});print(np.array2string(response.results.reshape(response.results.size >> 3, 8)))"

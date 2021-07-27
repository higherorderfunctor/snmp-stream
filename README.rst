.. role:: bash(code)
   :language: bash

.. _snmp_fetch: https://github.com/higherorderfunctor/snmp-fetch

steam-stream
============

This python package is an in-progress rewrite/refactor of the snmp_fetch_ SNMPv2 library.  While this package includes an improved implementation, it is currently missing critical components for a production library including documentation, testing with coverage, CI/CD, and packaging.

Quick Start
-----------

This tutorial is for setting up a developer environment to build and test the package in alpine linux.  The only hard requirement is docker.

.. code:: bash

   # clone the repository
   git clone --recurse-submodules -j8 https://github.com/higherorderfunctor/snmp-stream.git
   cd snmp-stream

   # build the docker image
   docker build --target snmp-stream -t snmp-stream:latest .

   # start the docker container
   docker run \
     -it --name snmp-stream \
     # example: expose dotfiles from host
     -v $HOME/Documents/dotfiles:/root/dotfiles \
     # /example
     snmp-stream:latest /bin/sh

   # example: symlink development environment from host
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

   # optional: bootstrap shell environment for editor
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
   poetry run snmpsimd.py --agent-udpv4-endpoint=127.0.0.1:1161 --process-user=root --process-group=root

   # query the virtual device
   poetry run python -c "import numpy as np;from snmp_stream import *;response = walk('127.0.0.1:1161',('recorded/linux-full-walk', 'V2C'),['1.3.6.1.2.1.2.2.1.1','1.3.6.1.2.1.2.2.1.2'],req_id='abc',config={'retries': 1, 'timeout': 3});print(np.array2string(response.results.reshape(response.results.size >> 3, 8)))"

Design Philosophy
-----------------

:bash:`snmp-stream` is designed to be a component for collecting large amounts of SNMP data in a distributed system.  It uses a C++ extension to accomplish this goal with a python interface for ease of use.  This allows it to be combined with other python frameworks that excel at building distributed pipelines.  Other python SNMP packages suffer from exposing each record as a python object which is memory inefficient as well as computationally inefficient when moving the data to another node in a distributed system.  Instead, :bash:`snmp-stream` builds a compact contiguous block of memory with the following wireline format:

+--------------+----------------------------------------------------+
| Header Bytes | Description                                        |
+==============+====================================================+
| 0            | System byte order (0 = little, 1 = big)            |
+--------------+----------------------------------------------------+
| 1            | System WORD size (e.g. 8 for a 64 bit system)      |
+--------------+----------------------------------------------------+
| 2            | NetSNMP Octet Size (usually 8 for a 64 bit system) |
+--------------+----------------------------------------------------+
| 3-15         | RESERVED                                           |
+--------------+----------------------------------------------------+

The next portion of the header is described in System WORDs.

+--------------------+--------------------------------------------------------+
| System WORDs       | Description                                            |
+====================+========================================================+
| 1                  | Size of user configurable request ID in bytes          |
+--------------------+--------------------------------------------------------+
| sizeof(request ID) | Optional request ID padded to system WORD size         |
+--------------------+--------------------------------------------------------+
| 1                  | Count of root OIDs                                     |
+--------------------+--------------------------------------------------------+

Following is a repeating data structure for every root OID.

+--------------------+----------------------------------------------------------+
| System WORDs       | Description                                              |
+====================+==========================================================+
+ 1                  | Size of root OID in system WORDs                         |
+--------------------+----------------------------------------------------------+
+ sizeof(OID)        | Root OID padded to system WORD size (one octet per WORD) |
+--------------------+----------------------------------------------------------+

Last are the results in the following repeating data structure.

+--------------------+----------------------------------------------------------+
| System WORDs       | Description                                              |
+====================+==========================================================+
| 1                  | Size of record                                           |
+--------------------+----------------------------------------------------------+
| 1                  | Timestamp                                                |
+--------------------+----------------------------------------------------------+
| 1                  | Index of the Root ID from the section above.             |
+--------------------+----------------------------------------------------------+
| 1                  | Value type from SNMP                                     |
+--------------------+----------------------------------------------------------+
| 1                  | Size of the index portion of the OID (OID - root OID)    |
+--------------------+----------------------------------------------------------+
| sizeof(index)      | Index portion of the OID padded to system WORD size      |
|                    | (octets will be NetSNMP Octet Size)                      |
+--------------------+----------------------------------------------------------+
| 1                  | Size of the value                                        |
+--------------------+----------------------------------------------------------+
| sizeof(value)      | Value padded to system WORD size                         |
+--------------------+----------------------------------------------------------+

The complete record is designed to be efficiently transmitted to another node in a data pipeline for processing and reassembly.  Data is copied into this record from NetSNMP without ever needing to be pickled.  Basic usage is as follows:

.. code::

   import snmp_stream._snmp_stream as snmp

   # the SessionManager is a basic queue for holding request tasks and state
   session = snmp.SessionManager(snmp.Config(...))

   # add 1 or more requests to the queue
   session.add_request(snmp.SnmpRequest(...))

   # Run until one or more requests has completed.  Returns None if the
   # queue is empty, otherwise it returns a list of SnmpResponses.
   while True:
     responses = session.run()
     if responses is not None:
         for response in responses:
             # do something with response; when response is out of scope, memory is freed

         # optionally add more requests to be processed
         session.add_request(snmp.SnmpRequest(...))
     else:
         # queue is empty
         break

The :bash:`Config` object exposes the typical parameters seen in most SNMP libraries.

+--------------------------------+------------------------------------------------------------+
| Member                         | Description                                                |
+================================+============================================================+
| retries                        | Number of retries for the same PDU before the request is   |
|                                | terminated (default = 3)                                   |
+--------------------------------+------------------------------------------------------------+
| timeout                        | Number of seconds to wait for a response PDU (default = 3) |
+--------------------------------+------------------------------------------------------------+
| max_response_var_binds_per_pdu | Maximum number of results in a single PDU (default = 10)   |
+--------------------------------+------------------------------------------------------------+
| max_async_sessions             | Maximum number of concurrent sessions (default = 10)       |
+--------------------------------+------------------------------------------------------------+

:bash:`max_response_var_binds_per_pdu` controls max repetitions in the SNMPv2 protocol.  The number of repetitions is adjusted based on the number of OIDs in the request.  For the best performance, the number of OIDs should be a multiple of :bash:`max_response_var_binds_per_pdu`.  In testing, some devices can set this value arbitrarily high and the remote device will fill the entire PDU.  Other devices won't respond if the result set doesn't fit in a single PDU.

:bash:`max_async_sessions` controls the number of concurrent sessions in a single thread.  This package has not been tested for multithreading.  Instead, it is recommended to use one of python's multiprocessing libraries to take advantage of additional system cores.  Memory usage will scale with the number of concurrent sessions.

The :bash:`SnmpRequest` object has the following parameters:

+--------------------------------+------------------------------------------------------------+
| Member                         | Description                                                |
+================================+============================================================+
| type                           | GET_REQUEST or WALK_REQUEST                                |
+--------------------------------+------------------------------------------------------------+
| host                           | Target host                                                |
+--------------------------------+------------------------------------------------------------+
| community                      | Community string [ tuple of (TEXT, V1 | V2C) ]             |
+--------------------------------+------------------------------------------------------------+
| oids                           | List of root OIDs                                          |
+--------------------------------+------------------------------------------------------------+
| ranges                         | Range of additional octets to be appended to root OIDs     |
+--------------------------------+------------------------------------------------------------+
| req_id                         | Optional request ID to aid in reassembly in a distributed  |
|                                | system                                                     |
+--------------------------------+------------------------------------------------------------+
| config                         | Default and/or SessionManager overrides                    |
+--------------------------------+------------------------------------------------------------+

Most of these fields are self explanatory within an SNMP context.  :bash:`oids` and :bash:`ranges` are the exception.  OIDs have the format :bash:`<column identifier or root OID>.<index fields>`.  If all root OIDs are from the same table, the index fields are predictable.  :bash:`ranges` is a start/stop tuple of acceptable full or partial index values.

This has two uses to improve collection.  First, it can be used to reduce the memory footprint when collecting from a large table (e.g. full internet routing table).  The index can be capped to only collect a single /8 of the table at a time.  When it completes, it will be emitted to be sent to the next stage of the pipeline so the memory can be freed.  Second, for a sufficiently powerful router, multiple index ranges can be collected concurrently.

Distribution and data reassembly are out-of-scope for this package.  Something like :bash:`dask` or :bash:`kafka` are candidates for distribution of requests.  Parsing the wireline data could be done directly in python or a library like :bash:`protobuf`.  :bash:`pandas` is an excellent tool for reassembling records into tabular format, a process that can also be distributed using distributed DataFrames.

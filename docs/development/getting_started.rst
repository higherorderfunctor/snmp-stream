Getting Started
===============


virtualenv -p python3.8 .env
source .env/bin/activate
source env.sh


Linting
'''''''

.. code:: bash

   poetry run isort -rc --atomic .

   poetry run pylint snmp_fetch tests
   poetry run flake8 netframe_snmp tests
   poetry run bandit -r netframe_snmp
   poetry run mypy -m netframe_snmp -m tests

Testing
'''''''

.. code:: bash

   poetry run pytest -v -x --ff tests

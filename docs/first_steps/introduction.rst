Introduction
============

Snmp-stream is an opinionated SNMPv1/v2c client for python that is designed for large-scale distributed network data mining.  It's primary use case is reading requests off a message bus, collecting and parsing the results, and then pushing the results back onto a message bus or directly into a database.

This package uses modern software tooling and therefore has some high dependency requirements.  SNMP collection is performed in a source distributed C++17 extension that wraps the latest version of NET-SNMP and targets Linux only.  Building the extension is fully automated, but it requires a number of developer tools to be installed.  The python package only supports python 3.7+ due to its liberal use of static typing.  Finally, the parser is written with numba and therefore requires LLVM.   Due to these high requirements, the ideal runtime environment is in a docker container.  Dockerfiles are provided in the examples directory as a starting point.

Collecting and parsing the results can be split into two distinct tasks.  The C++ extension returns a binary blob (numpy uint8 array) with the encoded response variable bindings.  Parsing the binary blob is performed using a functional parser combinator.  This allows for workflows that separate these tasks as part of a distributed data pipeline where lower-end systems, possibly distributed within different routing domains, focus on collecting results, and higher-end systems assemble and post-process the results.

Why it is not
'''''''''''''

A MIB Processor
---------------

This package does not perform any MIB processing.  Existing MIB processing libraries are often slow, strictly enforce MIB constraints not respected by all vendors, and struggle with extracting complex index fields.  This is not ideal in a large-scale network data mining tool.  Therefore, the user will need to parse the results using the included functional parser combinator or by some other means.

Fully Thread-Safe
-----------------

NOTE: Thread safety is not currently tested and these are only assumptions until proven otherwise.

The single-use and streaming APIs should be relatively thread-safe.  The low-level API exposes a SessionManager which is not a thread-safe object.  If using the low-level API in a multi-threaded application, each thread will need its own dedicated instance.

NET-SNMP Binding
----------------

While snmp-stream uses NET-SNMP under the hood, it does not expose the library directly to python.  It is designed to spend as much time in the C++ extension as possible in order to maximize performance.  The interface only exposes a generalized GET and WALK requests and only returns once one or more requests have completed.




The C++ extension can asynchronously process multiple requests in parallel.  However, it will only hand back execution to python after an entire request is complete.

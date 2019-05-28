# ACVP Proxy

:toc:
:homepage: https://github.com/smuellerDD/acvpproxy

The ACVP Proxy allows the retrieving of test vectors from the ACVP
servers. In addition, it allows the submission of test results to the ACVP
servers and to retrieve the verdict of the test results.

The obtained test vectors stored in the files `testvector-request.json`
are intended to be transferred to the test system hosting the cryptographic
module to be tested. The JSON file must be inserted into the cryptographic
module to produce the `testvector-response.json` file holding the responses
according to the ACVP protocol specification. An example implementation
that parses these JSON files, invokes the cryptographic implementation
and generates the test response files, see the ACVP Parser.

The ACVP Proxy is proudly supported by [atsec information security corp](https://www.atsec.com).

## Interaction of ACVP Proxy and ACVP Parser - First Steps

The ACVP Proxy and the ACVP Parser collaborate in a full ACVP test cycle as
follows.

. Find the right cipher definition or create a new definition with the
  ACVP Proxy:
  `acvp-proxy -l`

. Download the ACVP test vectors for the chosen cryptographic module using
  the ACVP Proxy:
  `acvp-proxy -m <cryptomodule> --request`

. Send the entire testvectors data store to the IUT that is connected with the
  ACVP Parser. You may use the following command:
  `tar -cvzf testvectors-<vendor>-<date>.tar.gz testvectors/`

  You may also create a symbolic link to the testvectors directory from the
  ACVP Parser.

.Invoke the JSON files in the testvectors directory with the ACVP Parser:
  `acvp-parser testvector-request.json testvector-response.json`

  You may use the helper script helper/exec_*.sh in the ACVP Parser code
  to automate the processing of all JSON files.

. Load the generated testvector-response.json files into the testvectors/
  data store of the ACVP Proxy. For example:

.. Create the archive with the responses from ACVP Parser:
   `tar -cvzf testvectors-<vendor>-response-<date>.tar.gz testvectors/`

.. Unpack the testvector-responses.json files into the ACVP Proxy
   testvectors/ directory:
   `tar -xvzf testvectors-<vendor>-response-<date>.tar.gz *testvector-response.json`

   You may skip this copying business if the ACVP Proxy and ACVP Parser both
   access the very same testvectors directory.

. Use the ACVP Proxy to submit the test responses and fetch the verdicts.
  `acvp-proxy -m <cryptomodule>`

. Check the verdict.json file in the testvectors directory for the ACVP
  server verdict.

## Test Publication Phase

When communicating the cipher options, the test vectors and responses and the
test verdicts in the process above, no information about the particular
module like module name or version or vendor details is communicated.

After obtaining the hopefully passing verdict the test session is eligible
for obtaining a certificate (with the production server). For such publication
the ACVP server now must receive all meta data characterizing the product.

Below this README describes how the user shall maintain several JSON files
defining the vendor, the module, and the operational environment. During the
publication phase, the ACVP server is informed about this meta data and it
is associated with the test session. That means that even yet unannounced
products can be tested using the public servers since nobody will be able to
associate the test session with a particular product before the publication
phase.

The publication phase is triggered with the `--publish` option. During the
publication phase, the ACVP proxy will ensure the ACVP server receives the
meta data and then combines it with the test session.

### Vendor ID / Module ID / Operational Environment ID

To support the publication phase, the ACVP Proxy will add and maintain
various ID entries in the JSON files explained below.

These IDs point to the respective vendor, module or operational environment
information maintained by the ACVP server for the given data set like vendor
or contact person details. These IDs must be used by the ACVP Proxy for the
final publication phase, i.e. when the option `--publish` is used. Before
using them, the ACVP proxy will verify that the IDs still point to the
entry with the same information as given by the JSON files.

If the IDs do not exist in the JSON files, the ACVP Proxy tries to resolve them
by looking up the ACVP server database which may take some time.

In case the specified resource like vendor definition or module definition
is not found, the definition is registered with the ACVP server if the
`--register-definition` option is provided with the `acvp-proxy` tool.

As mentioned, even existing IDs are checked with the ACVP server to verify
the ACVP server still contains the proper information. If it does not
and `--register-definition` is provided, it will request an update of the
ACVP server entry.

If the ACVP proxy identifies it needs to perform a register operation but
the `--register-definition` option is not provided, it will issue a warning
about the needed register operation and terminate. In this case, no add or
update request is sent to the ACVP server.

Please note: if the addition or update request is sent to the ACVP server,
NIST will need to perform a manual step to verify the data. Thus, you MUST
interact with NIST to have the request(s) approved. After the approval process
the publication operation can be invoked again and the ACVP proxy will
now fetch the registered ID information from the ACVP server. If the
request was not yet approved at the time the ACVP proxy shall publish
a particular module, it will identify this scenario and terminate with an
error message.

Note, if you maintain multiple modules which have the same, say, vendor
definition or operational environment definition, and you have the respective
IDs for one module already, you may copy them to the JSON files of the
respective other modules. In this case, the ACVP proxy can skip the resolving
operation that may be lengthy and simply verify the given ID.

## Prerequisites

The code is written in clean and plain C-99 and has the following dependencies:

- POSIX API

- if threading is enabled, support for the POSIX threading API must be present

- SysV message queues if the TOTP server is compiled

Compile-time options can be specified in the `lib/config.h` file.

The ACVP Proxy requires the presence of the following libraries and the
associated header files:

- libcurl

With these limited prerequisites, the code can be compiled and executed at
least on the following operating systems:

- Linux

- BSDs

- Apple macOS

- MS Windows

- Solaris

## Compilation

The ACVP Proxy is used by compiling the command line configuration
application `acvp-proxy`. This application is compiled by calling make.

To compile the ACVP Proxy standalone shared library, call `make` in the
`lib/` directory.

Compiling with `make debug` allows debugging of the code.

### Building on Windows 10

To compile the tool on Windows 10, perform the following steps as administrator:

- Enable Windows Subsystem for Linux (WSL)

	* In the Windows Settings search box, type: Turn Windows features on or off

	* Select: Windows Subsystem for Linux

- Reboot

In addition, perform the following steps with your normal (developer) account:

- Install "Ubuntu 18.04 LTS" from the Windows Store

- Launch "Ubuntu 18.04 LTS" app to bring up a bash shell. In bash, type:

```
	sudo apt-get update
	sudo apt install gcc
	sudo apt install make
	sudo apt install libcurl4-gnutls-dev
	sudo apt-get update

	mkdir src
	cd src
	git clone https://github.com/smuellerDD/acvpproxy
	cd acvpproxy
	make
```

### Building and Execution on Cygwin

The following steps are required to build the ACVP Proxy on Cygwin:

- Verify that the required cygwin packages are installed:

	* gcc

	* make

	* libcurl

	* cygserver

- Build acvp-proxy.exe using make.

For running the acvp proxy, IPC support is enabled by starting the windows
service cygserver by running the following command as administrator under
Cygwin:

`./cygserver-config`

Reply "yes" when asked whether to install cygserver as service. The service
is started with the command:

`net start cygserver`

## Configuration File

The ACVP proxy needs a configuration file as documented in apps/README.md.

## Module Instantiation Configuration

The ACVP Proxy contains the module cipher operation as part of its C code.

However, the instantiation of a particular module cipher definition into a
module is driven by configuration files. With the term instantiation, the
combination of the cipher option with a module information, the operational
environment and the vendor information is referred to.

To instantiate a particular module cipher definition, the ACVP Proxy uses
information provided in a directory `module_definitions` in the current
working directory. A different directory can be provided with a command
line switch.

That directory must have the following structure:

```
<directory>
    |
    \---- oe
    |
    \---- vendor
    |
    \---- module_info
    |
    \---- implementations
```

The `implementations` directory allows the specification of
supported cipher definitions. The C code commonly defines as much cipher
implementations as possible. However, some versions of the module (e.g. older
ones) may not support one or the other cipher definitions. Thus, one or more
JSON files in `implementations` can be defined that only define the supported
cipher definitions.

Each of these directories must have JSON files providing the
module definition. One JSON file holds one definition. It is permissible to
have multiple JSON files in one directory. The ACVP Proxy will instantiate the
module with all permutations of oe/vendor/module_info definitions that it
finds.

The JSON files in the `oe` directory must contain the following JSON keywords:

* `oeEnvName`: Name of the operation environment (such as operating system and
  its version)

* `cpe`: CPE tag (may be non-existant if `swid` exists)

* `swid`: SWID tag (may be non-existant if `cpe` exists)

* `oe_description`: Description of the operational environment

* `manufacturer`: Manufacturer of the underlying CPU executing the module.

* `procFamily`: Processor family executing the module. Note, this string must
  match one `processor` of the module cipher definitions registered with the
  ACVP Proxy. See `acvp-proxy -u` for details. If the list contains an
  empty string for the processor, the JSON are not restricted in the processor
  information.

* `procName`: Specific processor name executing the module.

* `procSeries`: Processor series executing the module.

* `features`: Feature defined by an OR of the `OE_PROC_*` definitions.

* `envType`: Type of the execution environment which is a selection from
  `enum def_mod_type`.

The JSON files in the `vendor` directory must contain the following JSON
keywords:

* `vendorName`: Company name of the vendor.

* `vendorUrl`: URL of the company.

* `contactName`: Name of the contact person at the vendor.

* `contactEmail`: Email address of the vendor's contact person.

The JSON files in the `module_info` directory must contain the following JSON
keywords:

* `moduleName`: Free-form name of the module. Note, this name must be identical
  to the cryptographic definitions name found in the variable `algo_name` that
  can be obtained with `acvp-proxy -u`.
  Note, when instantiating the module, that string is appended by the module
  crypto definition detailed reference specified with the variable `impl_name`
  found in the C code defining the cryptographic implementation. That
  implementation name can be shown with `acvp-proxy -u`.

* `moduleVersion`: Version number of the module.

* `moduleDescription`: Free-from description of the module.

* `moduleType`: Type of the module which is a selection from
  `enum def_mod_type`.

The JSON files in the `implementations` directory must contain the following
JSON keywords:

* `implementations`: This keyword must point to an array or one or more
  supported names which match the `impl_name` variable in the algorithm
  definitions of the C code.

## Usage

The ACVP Proxy has the following use cases:

- The retrieval of test vectors from ACVP is the first use case. This use case
  is invoked by using the command line option of `--request`. The test vectors
  are stored in local directory documented below.

- The submission of test results to ACVP and the fetching of the ACVP verdict
  for the test results is invoked by using the command line option of either
  `--vsid <VSID>` or `--testid <TESTID>`.

- Register the meta information about the module with the ACVP server, including
  vendor information, module information and operational environment
  information.

- Request the publication of the completed ACVP testing cycle to obtain an
  ACVP certificate.

NOTE: If neiter `--request`, nor `--vsid <VSID>` or `--testid <TESTID>` is
specified, the application will try to send out all existing test results
which have not yet been submitted and obtain a verdict for them.

## Signals

The ACVP Proxy supports the following signals:

* SIGHUP, SIGINT, SIGTERM: Stopping any outstanding requests and deleting the
  outstanding requests at the ACVP server (invoke the DELETE operation
  on the test session URL). The ACVP server is intended to cleanup all
  reference to the test session preventing any later retrieval.

* SIGSTOP (commonly CTRL+\ ): Stopping any outstanding requests without
  deleting them on the ACVP server. This allows terminating the network
  connection and allow the retrieval of the test vector at a later time.
  Usually this is helpful if the ACVP server takes a long time where you
  do not want the ACVP Proxy to constantly retry downloading the vector. To
  resume the download of the test vectors, use `--request --testid <NUM>`
  where `<NUM>` is the test session ID printed out by the ACVP Proxy when
  receiving SIGSTOP.

The stopping and restarting of downloads applies to the download of
testvectors (i.e. when --request is used) as well as to the download of
verdicts after the upload of the test responses.

## Directory Structure To Maintain Test Vectors and Test Results

The test vectors and results are stored in the directories with the following
structure: `testresults/<Module_Name>/<Module_Version>/<testsessionID>/<vsID>`.

The `<Module_Name>/<Module_Version>` directory refers to the name and version
of the cryptographic module under test.

The `<testsessionID>/<vsID>` is a unique number provided by the ACVP server
for the respective test. This test session ID and vsID are the unique
references for subsequent data exchange with the ACVP server regarding the
particular obtained test vector.

In each `<testsessionID>/<vsID>` directory, the following files can be found,
depending on the state of the communication with the ACVP server.

- `testvector-request.json`: Test vector to be processed by the module
  implementation.

- `testvector-response.json`: The test results returned by the module
  implementation must be provided in this file.

- `verdict.json`: After submission of the test results to the ACVP server, the
  test verdict is provided in this file.

- `processed.txt`: This file contains the time stamp when the test results
  were sent to the ACVP server and the verdict was received. Once this file
  is present, the ACVP Proxy will not submit the test results again.

In addition to the `testresults` directory, a shadow `secure-datastore` is
maintained with the same directory structure. That shadow directory contains
internal or sensitive data as follows:

- `jwt_authtoken.txt`: JWT authorization token required for authentication
  with the ACVP server when exchanging subsequent messages for the vsID.

- The `*.debug` files contain the respective server responses allowing to
  debug network communication problems. These files are not processed by
  the ACVP Proxy.

- The `request-<DATE>.json` file contains the IUT register data sent to
  the ACVP server for requesting test vectors.

- The `testid_metadata.json` file contain the initial response from the
  ACVP server indicating the number of vsIDs as well as the testID
  with their URLs.

## FIPS 140-2 Compliance

The ACVP Proxy uses the following cryptographic support:

- TLS: This is provided by the TLS implementation used by CURL. Commonly this
  is provided by either OpenSSL or NSS. Those libraries are FIPS 140-2 validated
  and thus should be used according to the security policy.

- TOTP: HMAC SHA-256 is used as the cipher for the TOTP operation.
  This implies that the ACVP Proxy implements a FIPS 140-2 cipher which must
  be subject to the power on self test. The known answer test for the
  HMAC SHA-256 is always executed. The integrity test is executed when
  the environment variable ACVPPROXY_FORCE_FIPS is set (or on Linux, when
  /proc/sys/crypto/fips_enabled contains a 1). In this case the integrity
  check with HMAC SHA-256 is enforced where the HMAC control file is
  searched in the same directory as the ACVP Proxy executable. When the
  HMAC control file does not exist, it is created.

# Architecture of ACVP Proxy

The ACVP Proxy contains of the following components:

- A small wrapper application allowing the user to provide command line
  options. This wrapper application does not contain any of the ACVP Proxy
  logic and can be replaced using the API of the library mentioned below.

- The libacvpproxy.so library implements the ACVP Proxy logic. It
  exports an API in `acvpproxy.h` that can be used by a wrapping
  application.

## ACVP Proxy Library

The ACVP Proxy library provided in the `lib/` directory contains the heavy
lifting. This library also contains an internal structure. The core ACVP
protocol handling uses the following extensions which can be replaced with
a respective different implementation:

- A network access backend implements the communication provider. The ACVP
  server requires HTTPS support which implies that the communication provider
  must implement HTTPS. The network access backend is provided by
  implementing the callbacks specified in `struct acvp_netaccess_be`. An
  example implementation based on libcurl is provided in
  `network_backend_curl.c`.

- A datastore backend implements the storage of the data retrieved from the
  ACVP server or must provide the data to be sent to the ACVP server. The
  datastore backend implements the callbacks defined by
  `struct acvp_datastore_be`. The example implementation storing the data
  in directories as outlined above is provided in `datastore_file.c`.

- The JSON request generators for the different cipher types are implemented
  in the files `request_sym.c` and similar. To add a new generator for a new
  cipher type, create a data structure to hold all options in `definition.h`
  (like `struct def_algo_sym`) add add it to `struct def_algo` along with a new
  identifier. Finally, the entry function to the new generator must be
  invoked in the function `acvp_req_set_algo`. The JSON generator shall
  implement the generation of one entry for a particular cipher request only.

- A definition for a particular module holds all supported cipher options.
  Such a definition instantiates the data structures defined by the different
  JSON generators. One example for such a definition is provided in
  `definition_impl_openssl.c`. Each definition must have a unique module name
  and version combination to allow a user to point to it.

## Threading Support

Using the PTHREAD library support, threading is implemented. Concurrent
threads are supported for the following operations:

- Downloading of the test vectors: The test vectors of each individual vsID are
  downloaded in a separate thread.

- Uploading of test responses and downloading the associated verdicts for
  each testsession ID and the assoicated vsID.

## Debugging

Compile with `make debug` to compile debug symbols for debugging.

# Addition of New Cipher Implementation Definitions

The library is written such that new cipher implementations can be added to
generate the required register information with the CAVS servers.

## Registering

A new cipher implementation is registered with the library using the API call
of `acvp_register_algo_map`. This call should be made during the initialization
of the library.

To allow isolating the module definition code into one C file, it is
permissible to invoke the `acvp_register_algo_map` function from a constructor.
For example, the following function is invoked autonomously from the loader
of the library and does not require any changes to the library code itself
in order to register the module specification:

```
ACVP_DEFINE_CONSTRUCTOR(libgcrypt_register)
static void libgcrypt_register(void)
{
	acvp_register_algo_map(libgcrypt_algo_map,
			       ARRAY_SIZE(libgcrypt_algo_map));
}
```

## Defining of Cipher Implementation Capabilities

The function `acvp_register_algo_map` requires a completely defined data
structure tree which contains all details of the cipher implementation. Thus,
for a new cipher definition, all data structures starting with struct
definition must be filled in corresponding to the implementation details.

The header files defining the data structures contain the documentation how
the data needs to be filled in.

# Author

Stephan Mueller <smueller@chronox.de>
Copyright (C) 2018 - 2019


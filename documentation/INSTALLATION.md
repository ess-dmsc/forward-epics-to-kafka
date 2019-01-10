## Installation

### Requirements

These libraries are expected in the ESS dev default locations or set via
environment variables (see `src/CMakeLists.txt`):

- EPICSv4
- `streaming-data-types`, easiest if cloned parallel to this repository.
  <https://github.com/ess-dmsc/streaming-data-types>

Tooling
- Conan
- Cmake (minimum tested is 2.8.11)
- C++ compiler with c++11 support
- Doxygen - CMake variable `RUN_DOXYGEN` needs to be set to `TRUE` and then use `make docs`


### Conan repositories

The following remote repositories are required to be configured:

- https://api.bintray.com/conan/ess-dmsc/conan
- https://api.bintray.com/conan/conan-community/conan

You can add them by running

```
conan remote add <local-name> <remote-url>
```

where `<local-name>` must be substituted by a locally unique name. Configured
remotes can be listed with `conan remote list`.


### Build

Assuming you have `make`:

```
cmake <path-to-source> [-DCONAN_DISABLE=TRUE]
make
make docs  # optional
```

To skip building the tests target pass cmake `-DBUILD_TESTS=FALSE`

#### Running on OSX

When using Conan on OSX, due to the way paths to dependencies are handled,
the `activate_run.sh` file must be sourced before running the application. The
`deactivate_run.sh` can be sourced to undo the changes afterwards. This has not
been tested yet, and it is possible that EPICS libraries cannot be found.
Please report any issues you encounter when running this setup.


#### Dependencies in custom locations

The `forward-epics-to-kafka` follows standard `CMake` conventions.
You can use the standard `CMAKE_INCLUDE_PATH`, `CMAKE_LIBRARY_PATH` and
`CMAKE_PROGRAM_PATH` to point `CMake` into the right direction.
It will prefer dependencies found there over those in the system directories.

We of course also support the ESS EPICS installation scheme.
To that end, we use as specified in the ESS wiki:
- `EPICS_V4_BASE_VERSION`
- `EPICS_BASES_PATH`
- `EPICS_HOST_ARCH`
- `EPICS_MODULES_PATH`


#### Fully non-standard dependencies

If you like full control over the dependencies:

Here follows an example where all dependencies are in non-standard locations.
Also EPICS is a custom build from source.
No additional environment variables are needed.
Only a few basic dependencies (like PCRE) are in standard locations.

```
export D1=$HOME/software/;
export EPICS_MODULES_PATH=$D1/epics/EPICS-CPP-4.6.0;
export EPICS_HOST_ARCH=darwin-x86;
cmake \
-DREQUIRE_GTEST=1 \
-DCMAKE_INCLUDE_PATH="$D1/fmt;$D1/rapidjson/include;$D1/flatbuffers/include;$D1/librdkafka/include;$D1/googletest;$D1/epics/base-3.16.0.1/include" \
-DCMAKE_LIBRARY_PATH="$D1/librdkafka/lib;$D1/epics/base-3.16.0.1/lib/$EPICS_HOST_ARCH" \
-DCMAKE_PROGRAM_PATH="$D1/flatbuffers/bin" \
<path-to-forward-epics-to-kafka-repository>
```

Note that in this example, there is no need for `EPICS_V4_BASE_VERSION`
or `EPICS_BASES_PATH` because we give them explicitly in `CMAKE_*_PATH`.

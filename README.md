[![MIT License](https://img.shields.io/badge/License-MIT-brightgreen.svg?)](https://github.com/kala13x/smake/blob/main/LICENSE)
[![C/C++ CI](https://github.com/kala13x/smake/actions/workflows/make.yml/badge.svg)](https://github.com/kala13x/smake/actions/workflows/make.yml)
[![CodeQL](https://github.com/kala13x/smake/actions/workflows/codeql.yml/badge.svg)](https://github.com/kala13x/smake/actions/workflows/codeql.yml)

## Simple-Make
`SMake` is small and simple tool which helps developers to automatically generate `Makefile` for C/C++ projects by only typing `smake` in the project directory. See the file `src/info.h` to check out what release version you have.

### Installation
Use included scripts to build and clean the projct.

```bash
git clone https://github.com/kala13x/smake.git --recursive
cd smake && ./build.sh --install --cleanup
```

### Usage
To use the `Makefile` generator you need to go into your project directory and type `smake`, it will automatically try to scan the project and generate the `Makefile` for you.

`SMake` also has some options to make your steps easier:
```bash
  -f <'flags'>        # Compiler flags
  -l <'libs'>         # Linked libraries
  -L <'libs'>         # Custom libraries (LD_LIBS)
  -b <path>           # Install destination for binary
  -c <path>           # Path to config file
  -e <paths>          # Exclude files or directories
  -i <path>           # Install destination for includes
  -o <path>           # Object output destination
  -p <name>           # Program or library name
  -s <path>           # Path to source files
  -j                  # Generate config file
  -I                  # Initialize project
  -d                  # Virtual directory
  -v                  # Verbosity level
  -x                  # Use CPP compiler
  -h                  # Print version and usage
```
For example, if your project requires `lrt` and `lpthread` linking and you need to compile it with `-Wall` flag, the command will be following:
```bash
smake -f '-Wall' -l '-lrt -lpthread'
```

With option `-p`, you can specify program name for your project, if you run smake without this argument, smake will scan your files to search main function and your program name will be that filename where `main()` is located.

Also if you specify program name with `.a` or `.so` extensions (`smake -p example.so`), smake will generate `Makefile` to compile your project as the static or shared library.

This is an example to generate `Makefile` for static library and specify install location for the library and headers:
```bash
smake -p mylib.a -l '-lpthread' -b /usr/lib -i /usr/include
```

The `Makefile` of this project is generated with command:
```bash
smake -j \
    -o ./obj \
    -b /usr/bin \
    -e './xutils' \
    -l '-lpthread' \
    -L './xutils/build/libxutils.a' \
    -f '-g -O2 -Wall -I./src -I./xutils/build' \
```

With argument `-j` it also generates the 'json' config file, which can be used in the future to avoid using command line arguments every time.

Config file generated and used by this project.
```json
{
    "build": {
        "name": "smake",
        "flags": "-g -O2 -Wall -I./src -I./xutils/build",
        "ldLibs": "./xutils/build/libxutils.a",
        "libs": "-lpthread",
        "outputDir": "./obj",
        "overwrite": true,
        "cxx": false,
        "verbose": 0,

        "excludes": [
            "./xutils",
        ]
    },

    "install": {
        "binaryDir": "/usr/bin"
    }
}
```

Anything that can be passed as argument can also be parsed from the config file. `SMake` will search config file at current working directory with name `smake.json` or you can specify path for the file with argument `-c`.

Example:
```json
{
    "build": {
        "name": "myproj.a",
        "outputDir": "./obj",
        "flags": "-g -O2 -Wall",
        "libs": "-lpthread",
        "compiler": "gcc",
        "overwrite": true,
        "verbose": 2,
        "cxx": false,

        "excludes": [
            "./examples"
            "./cmake"
        ],

        "find":{
            "libssl.so:libcrypto.so": {
                "path": "/usr/local/ssl/lib:/usr/local/ssl/lib64",
                "flags": "-D_PROJ_USE_SSL",
                "libs": "-lssl -lcrypto"
            },

            "libz.so": {
                "flags": "-D_PROJ_USE_LIBZ",
                "libs": "-lz"
            },

            "any_file.txt": {
                "path": "/opt/examples/smake",
                "flags": "-D_OPTIONAL_FLAGS",
                "libs": "-loptional -lexample",
                "thisPathOnly": true,
                "insensitive": false,
                "recursive": false
            }
        }
    },

    "install": {
        "binaryDir": "/usr/lib",
        "headerDir": "/usr/include/myproj"
    }
}
```

### Feel free to fork
You can fork, modify and change the code unther the The MIT license. The project contains LICENSE file to see full license description.

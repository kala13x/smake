[![MIT License](https://img.shields.io/badge/License-MIT-brightgreen.svg?)](https://github.com/kala13x/smake/blob/main/LICENSE)
[![C/C++ CI](https://github.com/kala13x/smake/actions/workflows/make.yml/badge.svg)](https://github.com/kala13x/smake/actions/workflows/make.yml)
[![CodeQL](https://github.com/kala13x/smake/actions/workflows/codeql.yml/badge.svg)](https://github.com/kala13x/smake/actions/workflows/codeql.yml)

## Simple-Make
`SMake` is a simple tool that helps developers automatically generate `Makefile` for C/C++ projects by only typing `smake` in the project directory. See the file `src/info.h` to check out what release version you have.

### Installation
Use included scripts to build and clean the project.

```bash
git clone https://github.com/kala13x/smake.git --recursive
cd smake && ./build.sh --install --cleanup
```

### Usage
To use the `Makefile` generator you need to go into your project directory and type `smake`, it will automatically try to scan the project and generate the `Makefile` for you.

Here is a brief overview of all available command line arguments that `smake` supports:

* `-f <'flags'>` - Specify compiler flags.
* `-l <'libs'>` - Specify libraries to be linked with your program.
* `-L <'libs'>` - Specify custom libraries (LD_LIBS).
* `-c <path>` - Set the path to the config file.
* `-b <path>` - Set the install destination for the binary.
* `-i <path>` - Set the install destination for the includes.
* `-e <path>` - Exclude specific files or directories.
* `-o <path>` - Set the object output destination.
* `-p <name>` - Set the program or library name.
* `-s <path>` - Set the path to the source files.
* `-j` - Generate a config file.
* `-I` - Initialize a new project.
* `-d` - Enable the use of a virtual directory.
* `-v` - Adjust the verbosity level of the output.
* `-x` - Use the CPP compiler.
* `-h` - Print version and usage information.

Each argument is optional and can be used in combination with others to suit your project's specific needs.\
Please ensure you replace the placeholders (<'flags'>, <path>, etc.) with actual values relevant to your project.

For example, if your project requires `lrt` and `lpthread` linking and you need to compile it with `-Wall` flag, the command will be the following:
```bash
smake -f '-Wall' -l '-lrt -lpthread'
```

With option `-p`, you can specify program name for your project, if you run `smake` without this argument, smake will scan your files to search `main` function and your program name will be that filename where `main()` is located.

Also if you specify the program name with `.a` or `.so` extensions (`smake -p example.so`), smake will generate `Makefile` to compile your project as the static or shared library.

This is an example of generating `Makefile` for a static library and specifying the install location for the library and headers:
```bash
smake -p mylib.a -l '-lpthread' -b /usr/lib -i /usr/include
```

The `Makefile` of this project is generated with the command:
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

The config file was generated and used by this project.
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

Anything that can be passed as an argument can also be parsed from the config file. `SMake` will search the config file at a current working directory with the name `smake.json` or you can specify the path for the file with the argument `-c`.

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

As you can see in the example above, `find` JSON object can be used to find files and libraries in the system. Each entry in the find section is a key-value pair where the key is the file (or a colon-separated list of files) you want to locate and the value is another object describing how to handle the dependency.

The keys in the nested objects describe how `smake` should handle each dependency:

- `path` (optional): A colon-separated list of directories where `smake` should look for the files. If a file is found in these directories, the corresponding flags and libs will be applied.
- `flags` (optional): Flags that should be added to the compiler command line if the file is found.
- `libs` (optional): Libraries that should be linked to the executable if the file is found.
- `thisPathOnly` (optional): If set to true, smake will only look for the file in the specified path and not in the default locations.
- `insensitive` (optional): If set to true, the file search will be case-insensitive.
- `recursive` (optional): If set to true, smake will search recursively in the directories specified by path.

In the example above, `smake` will try to find `libssl.so` and `libcrypto.so` in either `/usr/local/ssl/lib` or `/usr/local/ssl/lib64`, if both of them are found, it will add `-D_PROJ_USE_SSL` to the compiler flags and `-lssl -lcrypto` to the linked libraries. The options for `libz.so` and `any_file.txt` are handled in a similar manner, with the additional `thisPathOnly`, `insensitive`, and `recursive` options.

Without `thisPathOnly` option, `smake` will first look for files in the provided `path`. If not found there, it will search in the following default locations:

- `/lib`
- `/lib64`
- `/usr/lib`
- `/usr/lib64`
- `/usr/local/lib`
- `/usr/local/lib64`

### Initialize the project
```bash
smake -I
```
When running the above command, `smake` will generate a "Hello, World!" project in the current working directory. The name of the project will be the same as the name of the current working directory. If you wish to provide a custom name for the executable, you can do so by passing the name as an argument using `-p`.

Any remaining arguments can also be used to initialize the project, for example:
```bash
smake -I -f '-Wall' -p test
```
The following command will create a compilable `test.c` file in the current working directory with "Hello, World!" content inside and a `Makefile` that compiles the project with `-Wall` flag.

### Feel free to fork
You can fork, modify and change the code under the MIT license. The project contains a LICENSE file to see the full license description.

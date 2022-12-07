## Simple-Make
`SMake` is small and simple tool which helps developers to automatically generate `Makefile` for C/C++ projects by only typing `smake` in the project directory. See the file `src/info.h` to check out what release version you have.

### Installation
`SMake` doesn't use any additional dependencies, so you can easily install it on any `Linux` / `Unix` distribution using an included `Makefile`.
```bash
git clone https://github.com/kala13x/smake.git
cd smake
make
sudo make install
```

### Usage
To use the `Makefile` generator you need to go into your project directory and type `smake`, it will automatically try to scan the project and generate the `Makefile` for you. 

`SMake` also has some options to make your steps easier:
```bash
  -f <'flags'>        # Compiler flags
  -l <'libs'>         # Linked libraries
  -b <path>           # Install destination for binary
  -c <path>           # Path to config file
  -e <paths>          # Exclude files or directories
  -i <path>           # Install destination for includes
  -o <path>           # Object output destination
  -p <name>           # Program or library name
  -s <path>           # Path to source files
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
smake -b /usr/bin -o obj -f '-g -O2 -Wall'
```
<p align="center">
    <img src="https://github.com/kala13x/smake/blob/master/smake.png" alt="alternate text">
</p>

### Config file
Anything that can be passed as arguments can also be parsed from the config file. `SMake` will search config file at current working directory with name `smake.json` or you can specify path for the file with argument `-c`.

This example includes all supported parameters and all of them are optional:
```json
{
    "build": {
        "name": "libxutils.a",
        "outputDir": "./obj",
        "flags": "-g -O2 -Wall -D_XSOCK_USE_SSL",
        "libs": "-lpthread -lssl -lcrypto",
        "compiler": "gcc",
        "overwrite": true,
        "verbose": 2,
        "cxx": false,
        "excludes": [
            "./examples"
        ]
    },
    "install": {
        "binaryDir": "/usr/lib",
        "headerDir": "/usr/include/xutils"
    }
}
```

### Feel free to fork
You can fork, modify and change the code unther the The MIT license. The project contains LICENSE file to see full license description.

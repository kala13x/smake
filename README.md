## Simple-Make - Version 1.0 build 26
SMake is small and simple tool which helps developers to automatically generate Makefiles for C/C++ projects.

### Installation
Installation is easy because the smake does not use any additional dependencies. It can be installed on any linux / unix platform with a Makefile:
```bash
git clone https://github.com/kala13x/smake.git
cd smake
make
sudo make install
```

### Usage
If you want to use smake, just cd to the source directory of your project and write smake. It will automatically scan files at working directory and generate `Makefile`. SMake also has some options to make your steps easier. Options are:
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
  -d                  # Virtual directory
  -v                  # Verbosity level
  -x                  # Use CPP compiler
  -h                  # Print version and usage
```
For example, if you have project which needs to link `pthread` and `lrt` library and you need to compile it with `-Wall` flag, you must write:
```bash
smake -f '-Wall' -l '-lrt -lpthread'
```

With option `-p`, you can specify program name for your project, if you will run smake without this argument, smake will scan your files to search main function and your program name will be that filename where `main()` is located.

Also if you will specify program name with `.a` or `.so` extensions (`smake -p example.so`), smake will generate `Makefile` to compile your project as the static or shared library.

This is an example create `Makefile` for static library and specify install location for the library and headers:
```bash
smake -p mylib.a -l '-lpthread' -b /usr/lib -i /usr/include
```

The `Makefile` of this project is generated with command:
```bash
smake -b /usr/bin -o obj -f '-g -Wall -O2' -l '-lpthread'
```

![alt tag](https://github.com/kala13x/smake/blob/master/smake.png)

### Config file
If you are so lazy and you wont to run smake anytime with arguments, you can write small config file and arguments will be parsed from it. SMake will search config file at current working directory with name `smake.json` or you can specify full path for config file with argument `-c`.
Config file example:
```json
{
    "build": {
        "name": "libxutils.a",
        "outputDir": "./obj",
        "flags": "-g -O2 -Wall -D_XSOCK_USE_SSL",
        "libs": "-lpthread -lssl -lcrypto",
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

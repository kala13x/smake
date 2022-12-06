## Simple-Make - Version 1.0 build 29
`SMake` is small and simple tool which helps developers to automatically generate `Makefile` for C/C++ projects by only typing `smake` in the project directory.

### Installation
`SMake` doesn't use any additional dependencies, so you can easily install it on any `Linux` / `Unix` distribution using an included `Makefile`.
```bash
git clone https://github.com/kala13x/smake.git
cd smake
make
sudo make install
```

### Usage
To use the `Makefile` generator you need to go into your project directory and type `smake`, it will automatically try to scan the project and generate the `Makefile` for you. `SMake` also has some options to make your steps easier. Options are:
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
For example, If your project requires 'lrt' and 'lpthread' linking and you need to compile it with `-Wall` flag, the command will be something like this:
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

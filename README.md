# Simple HTTP parser

Small HTTP parser for made from scratch. Currently only able to parse Requests, and can leak and segfault.

## TODO:

> * Add complete URI support to HTTP target
> * Prevent memory leaks due to malformed http request/headers
> * Prevent segfaults due to incomplete request body
> * HTTP response parsing

## Build

This project was developed in Windows with the MSYS2 environment (gcc from Mingw-w64). In order to build the project, just run:

```
git submodule init
build.bat
```

The project should work seamlessly on UNIX systems, just copy the contents of `build.bat` into a `.sh` file and you're good to go.

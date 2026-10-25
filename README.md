# Project Description

On NixOS you cannot just put a binary in `/usr/local/bin` to make it available everywhere. That is why I made this tool.

Another option is to always build the binary from source code. You can do this with a Makefile, with Rust/Cargo, or from small C files that make a full program.

This automation tool does these steps:

- **Compile**: It automatically builds local system programs.
- **Package**: The tool writes the right build instructions for the package manager.
- **Deploy**: It creates a new system state.
- After the build goes into the system state, it deletes the temporary compiled files.

It does not use root to compile. Compiling happens in a forked process as a normal dev user.

# Includes

- main.c <- compile_projects.h <- config_paths.h <- filesystem_operations.h
- main.c <- recreate_dir.h
- main.c <- deploy_pkgs.h <- config_paths.h <- filesystem_operations.h

# Build Makefile

```
make
make clean
make test
```

# Tests

There are many tests for creating config files and paths. These tests make sure that configs and folder structures are made correctly and can be repeated the same way every time. The program was also tested in real situations to check how it works in normal deployment cases. Manual ownership rules were followed.

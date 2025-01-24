# File System Simulator

This project is a **File System Simulator** written in C for GNU/Linux. The program runs in the command line, simulating a simple file system with basic operations such as file creation, deletion, listing, and copying. Also, this is the third assignment for the Operating Systems course.

## Features

- **Mounting and Unmounting**: Load or create a simulated file system.
- **File and Directory Operations**: Create, delete, and list directories and files.
- **Copying Files**: Copy text files from the real system into the simulated file system.
- **Metadata Management**: Tracks creation, modification, and access times.
- **File System Statistics**: Displays information about the number of files, directories, free space, and wasted space.
- **Database and Search**: Maintains an in-memory database of the file system structure and allows searches by file or directory name.

## Compilation

To compile the project, simply run:

```sh
make
```

This will generate the executable `ep3`.

## Execution

Run the program with:

```sh
./ep3
```

This will open an interactive shell with the following prompt:

```
{ep3}:
```

## Available Commands

- `monta <file>` – Mount the simulated file system from `<file>`.
- `desmonta` – Unmount the file system.
- `criadir <dir>` – Create a directory `<dir>`.
- `apagadir <dir>` – Delete a directory `<dir>` and its contents.
- `copia <origin> <destination>` – Copy a text file from the real system into the simulated file system.
- `mostra <file>` – Display the contents of a file.
- `toca <file>` – Update access time of `<file>` or create it if it does not exist.
- `apaga <file>` – Delete the specified file.
- `lista <dir>` – List the contents of `<dir>`.
- `atualizadb` – Update the in-memory database of the file system.
- `busca <string>` – Search for files and directories containing `<string>` in their names.
- `status` – Display file system statistics.
- `sai` – Exit the simulator.

## Notes

- The simulator assumes valid input. Special characters in file paths (e.g., spaces, `../`) are not supported.
- The file system is structured hierarchically, starting at `/`.
- The system supports up to **100MB** using **FAT** for file management and a **bitmap** for free space tracking.
- The simulator persists the file system state in the provided mount file.
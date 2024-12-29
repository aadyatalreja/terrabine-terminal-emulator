# TerraBine Terminal Emulator

TerraBine is a custom terminal emulator designed to provide users with a seamless command-line interface experience. The project features custom implementations of shell commands, with each command's logic encapsulated in dedicated shell files to ensure modularity and maintainability.

## Features

- **Custom Shell Commands**: TerraBine supports a range of commonly used shell commands, including `cd`, `ls`, `pwd`, and others.
- **Modular Design**: Each command is implemented in a separate shell file, making the codebase extensible and easy to understand.
- **Lightweight and Efficient**: Built for speed and simplicity, TerraBine runs efficiently on most systems.
- **Educational Tool**: Ideal for understanding the inner workings of terminal emulation and shell command implementation.

## Usage

Once the TerraBine terminal emulator is running, you can use it just like a standard shell. It supports the following commands:

- Change directories using `cd`
- List directory contents with `ls`
- Display the current working directory using `pwd`

### Example Commands

```bash
$ pwd
/home/user/terrabine

$ ls
file1.txt file2.py folder1

$ cd folder1

$ pwd
/home/user/terrabine/folder1

Macbot's Nothing But Net Robot
==============================

**Status:** *Early development. Nothing guaranteed to work nicely.*

## Usage

 1. [Install](http://purdueros.sourceforge.net/)
    *the Purdue Robotics OS for drivers.*

 2. [Download](https://github.com/ErnWong/NothingButNet-Mark-III/)
    *this project, either by*

     a) using git version control,

    ```bash
    git clone https://github.com/ErnWong/NothingButNet-Mark-III.git
    ```

     b) [downloading](https://github.com/ErnWong/NothingButNet-Mark-III/archive/dev-flat.zip)
        a zip.

 3. **Build** *the project (enter these in your favourite shell)*

    ```bash
    make
    ```

 4. **Upload** *the project to your vex robot (make sure that's plugged in)*

    ```bash
    make upload
    ```

 5. **Test** *the project to make sure all subsystems are functional*

    ```bash
    make test
    ```

 6. **Clean** *the binary files that were generated. (Note, if you don't clean
    then `make` compiles lazily, which is a good thing: files are only
    compiled when they've changed, and this speeds up the compile step)*

    ```bash
    make clean
    ```

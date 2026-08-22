# How To Compile Noctis IV Plus
No DeLorean needed for this trip through time. You will however need a computer capable of running DOSBox-X.

## Install DOSBox-X
### Windows
Before moving onto the following steps, [download DOSBox-X from here](https://dosbox-x.com/) and install it to it's default location `C:\DOSBox-X\dosbox-x.exe`. All the default settings should be good.

Alternatively, you can download the portable version and extract it's `bin` folder to the main `Noctis-IV-Plus` folder.
### \*nix
Installing DOSBox-X on most \*nix systems is as easy as running `sudo snap install dosbox-x`.

## Install Borland C++ 3.1 for DOS
1. Download Borland C++ 3.1 for DOS here: [BCPP31.ZIP](https://archive.org/download/bcpp31/BCPP31.ZIP).
2. Make a new folder in the main `Noctis-IV-Plus` folder named `bc.31`.
3. Extract the contents of the `BCPP31.ZIP` file to the `bc.31` folder to finish the install.

## Compiling
1. Double-click `source\Build.bat` (Windows) or `source\Build.bash` (\*nix) to run the build script.
    - It will prompt you if anything is missing.
    - If everything is downloaded correctly, it will open DOSBox-X for compilation.
        - If the build fails, it will let you know and prompt you to press a key to exit.
        - If the build succeeds, it will prompt you to press a key to run the newly compiled Noctis IV Plus.

The compiled binary is automatically copied over to the `modules` folder inside the main `Noctis-IV-Plus` folder.

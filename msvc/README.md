# How to Compile Libhat with MSVC/Visual Studio Compiler

You can either just get the lib from [here](https://github.com/notchyves/libhat/raw/master/msvc/libhat.lib), or you can use the build.bat to generate the .obj files. From there, you want to turn the .obj files into a .lib file by doing this:

```lib.exe /out:libhat.lib Process.obj Scanner.obj System.obj......``` and so on, including all of the .obj files.

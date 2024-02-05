cl.exe /std:c++latest /c /EHsc src/System.cpp src/Scanner.cpp src/Process.cpp

cl.exe /c /std:c++latest src/os/win32/MemoryProtector.cpp /Fowin32/MemoryProtector.obj
cl.exe /c /std:c++latest src/os/win32/Process.cpp /Fowin32/Process.obj
cl.exe /c /std:c++latest src/arch/x86/SSE.cpp /Fox86/SSE.obj
cl.exe /c /std:c++latest src/arch/x86/AVX2.cpp /Fox86/AVX2.obj
cl.exe /c /std:c++latest src/arch/x86/AVX512.cpp /Fox86/AVX512.obj
cl.exe /c /std:c++latest src/arch/x86/System.cpp /Fox86/System.obj
cl.exe /c /std:c++latest src/arch/arm/System.cpp /Foarm/System.obj
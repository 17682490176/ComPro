GCC_BIN := C:/Users/admin/AppData/Local/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin
SRC := main.c
RC := compro.rc
OUT := ComPro.exe

all: $(OUT)

$(OUT): $(SRC) $(RC)
	"$(GCC_BIN)/windres.exe" -i $(RC) -o compro_res.o
	"$(GCC_BIN)/gcc.exe" -o $(OUT) $(SRC) compro_res.o \
		-lcomdlg32 -lcomctl32 -lgdi32 -lwinmm -lws2_32 -liphlpapi -ldwmapi \
		-municode -O2 -Wall -flto -s -mwindows

run: all
	./$(OUT)

clean:
	rm -f $(OUT) compro_res.o

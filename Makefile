x86: CC=gcc
x86: PLATFORM=x86
x86: x86mbt

arm64: CC=aarch64-linux-gnu-gcc
arm64: PLATFORM=arm64
arm64: aarch64mbt

# Android (Bionic) arm64, built with the NDK clang. Set ANDROID_NDK_ROOT to
# the NDK directory, for example ~/Android/Sdk/ndk/26.1.10909125.
ANDROID_API ?= 30
NDK_CC = $(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android$(ANDROID_API)-clang

android: CC=$(NDK_CC)
android: PLATFORM=android
android: ndkcheck androidmbt

CFLAGS= -Wall -g

MKDIR= mkdir -p
OUTDIR=./Release

OBJS:= mbt_common.o \
		mbt.o

%.o: %.c
	$(CC) -c -o $@ $^

x86mbt : $(OBJS)
	$(MKDIR) $(OUTDIR)/$(PLATFORM)
	$(CC) -o $(OUTDIR)/$(PLATFORM)/mbt $^

aarch64mbt : $(OBJS)
	$(MKDIR) $(OUTDIR)/$(PLATFORM)
	$(CC) -o $(OUTDIR)/$(PLATFORM)/mbt $^
	@cp $(OUTDIR)/$(PLATFORM)/mbt ./cyw5557x/scripts_for_xavier/
	@cp $(OUTDIR)/$(PLATFORM)/mbt ./cyw5557x/scripts_for_nano/
	@cp $(OUTDIR)/$(PLATFORM)/mbt ./cyw5557x/scripts_for_rpicm4/
	@cp $(OUTDIR)/$(PLATFORM)/mbt ./cyw5557x/scripts_for_imx8/

ndkcheck :
	@test -x "$(NDK_CC)" || { echo "NDK clang not found: $(NDK_CC) (set ANDROID_NDK_ROOT)"; exit 1; }

androidmbt : $(OBJS)
	$(MKDIR) $(OUTDIR)/$(PLATFORM)
	$(CC) -o $(OUTDIR)/$(PLATFORM)/mbt $^

# UART flow control helpers in tools/, for Android (see tools/README.md)
TOOLS = cts_watch rts_ctl uart_watch gpio_line

tools: ndkcheck
	$(MKDIR) $(OUTDIR)/android
	$(foreach t,$(TOOLS),$(NDK_CC) -O2 -Wall -o $(OUTDIR)/android/$(t) tools/$(t).c &&) true

.PHONY: clean ndkcheck tools

clean :  
	@echo "Clean All"
	@rm *.o 2> /dev/null
	@rm Release -rf 2> /dev/null

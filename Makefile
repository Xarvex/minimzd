MODE := dev
ARGS :=

TARGET := minimzd

SDIR   := src
IDIR   := include
LDIR   := lib
ODIR   := obj
TDIR   := bin
GDIR   := generated
TPDIR  := third-party
PKGDIR := pkgconfig
SCRDIR := tools

CC     := clang
CPPC   := $(CC)++
PKG    := pkgconf
PKGOBJ := $(PKG) --cflags
PKGT   := $(PKG) --libs

POSTDEV  :=
POSTPROD := llvm-strip $(TDIR)/$(TARGET)

CFLAGS        :=
CPPFLAGS      := -std=c++11
CCPPFLAGS     := -Wall
CFLAGSDEV     :=
CPPFLAGSDEV   :=
CCPPFLAGSDEV  := -O0 -ggdb3 -g
CFLAGSPROD    := -fno-asynchronous-unwind-tables
CPPFLAGSPROD  := -fno-exceptions
CCPPFLAGSPROD := -O3 -fmerge-all-constants -ffast-math -ffunction-sections -fdata-sections -fno-stack-protector -fno-ident

VALGRIND := valgrind -s --leak-check=full --show-leak-kinds=definite,possible --track-origins=yes --log-file=valgrind-out.txt

cflags   := $(CCPPFLAGS) $(CFLAGS)
cppflags := $(CCPPFLAGS) $(CPPFLAGS)
ifeq ($(MODE),prod)
	cflags   += $(CCPPFLAGSPROD) $(CFLAGSPROD)
	cppflags += $(CCPPFLAGSPROD) $(CPPFLAGSPROD)
	post     := $(POSTPROD)
else ifeq ($(MODE),dev)
	cflags   += $(CCPPFLAGSDEV) $(CFLAGSDEV)
	cppflags += $(CCPPFLAGSDEV) $(CPPFLAGSDEV)
	post     := $(POSTDEV)
endif

LIBS        := dbus-1 gio-2.0
GDKDIR      := $(TPDIR)/gdk
GDKSDIR     := $(GDKDIR)/src
GDKGDIR     := $(GDKDIR)/generated
YYJSONDIR   := $(TPDIR)/yyjson
YYJSONSDIR  := $(YYJSONDIR)/src
YYJSONBDIR  := $(YYJSONDIR)/build
YYJSONFLAGS := -DYYJSON_DISABLE_WRITER=ON -DYYJSON_DISABLE_UTILS=ON -DYYJSON_DISABLE_FAST_FP_CONV=ON -DYYJSON_DISABLE_NON_STANDARD=ON -DYYJSON_DISABLE_UTF8_VALIDATION=ON -DYYJSON_DISABLE_UNALIGNED_MEMORY_ACCESS=ON

DJB2 := djb2
KMAP := keymap

RPATH := $(SCRDIR)/relpath.py
CKMAP := $(SCRDIR)/gen-keymap.pl

###############################
#    CAREFUL EDITING BELOW    #
###############################

target := $(TDIR)/$(TARGET)

odir := $(ODIR)/$(MODE)

libs = $(LIBS) $(wildcard $(shell readlink -f $(PKGDIR))/*.pc)

cobjects   := $(patsubst $(SDIR)/%.c, $(odir)/%.o, $(wildcard $(SDIR)/*.c))
cppobjects := $(patsubst $(SDIR)/%.cpp, $(odir)/%.opp, $(wildcard $(SDIR)/*.cpp))
objects    := $(cobjects) $(cppobjects)
sheaders   := $(wildcard $(SDIR)/*.h) $(wildcard $(SDIR)/*.hpp)
iheaders    = $(wildcard $(IDIR)/*.h) $(wildcard $(IDIR)/*.h)
headers     = $(sheaders) $(iheaders)

djb2 := $(GDIR)/$(DJB2)

define test-local-dependency
[ -d $(1) ]
endef

define update-local-dependency-git
if $(call test-local-dependency,$(1)); then\
	git -C $(1) pull -q;\
	else\
	git clone -q $(2) $(1);\
	fi
endef

define execute
$(target) $(ARGS)
endef

default: $(TARGET)
all: default valgrind

$(SDIR):
	mkdir -p $@

$(IDIR):
	mkdir -p $@

$(odir):
	mkdir -p $@

$(TDIR):
	mkdir -p $@

$(TPDIR):
	mkdir -p $@

$(PKGDIR):
	mkdir -p $@

$(GDKSDIR):
	mkdir -p $@

$(GDKGDIR):
	mkdir -p $@

$(GDKSDIR)/gen-keyname-table.pl: | $(GDKSDIR)
	curl -sSL https://gitlab.gnome.org/GNOME/gtk/-/raw/main/gdk/gen-keyname-table.pl > $@
	chmod +x $@

$(GDKSDIR)/keynames.txt: | $(GDKSDIR)
	curl -sSL https://gitlab.gnome.org/GNOME/gtk/-/raw/main/gdk/keynames.txt > $@

$(GDKSDIR)/keynames-translate.txt: | $(GDKSDIR)
	curl -sSL https://gitlab.gnome.org/GNOME/gtk/-/raw/main/gdk/keynames-translate.txt > $@

$(GDKGDIR)/keynamesprivate.h: $(GDKSDIR)/gen-keyname-table.pl $(GDKSDIR)/keynames.txt $(GDKSDIR)/keynames-translate.txt | $(GDKGDIR)
	$^ > $@

$(GDIR)/$(KMAP).h: $(CKMAP) | $(GDKGDIR)/keynamesprivate.h $(GDIR)
	$(CKMAP) /usr/include/linux/input-event-codes.h $(GDKGDIR)/keynamesprivate.h > $@

$(IDIR)/keymap.h: $(GDIR)/$(KMAP).h | $(IDIR)
	-rm -f $@
	ln -s $$($(RPATH) $^ $(IDIR)) $@

$(YYJSONDIR): | $(TPDIR)
	$(call update-local-dependency-git,$(YYJSONDIR),https://github.com/ibireme/yyjson.git)

$(YYJSONSDIR): | $(YYJSONDIR)

$(YYJSONBDIR): | $(YYJSONDIR) $(YYJSONSDIR)
	mkdir -p $@

$(YYJSONBDIR)/libyyjson.a: | $(YYJSONBDIR)
	cmake $(YYJSONDIR) -S $(YYJSONDIR) -B $(YYJSONBDIR)\
		-DCMAKE_MESSAGE_LOG_LEVEL=WARNING\
		$(YYJSONFLAGS) && cmake --build $(YYJSONBDIR) -- -s

$(PKGDIR)/yyjson.pc: | $(YYJSONSDIR) $(YYJSONBDIR)/libyyjson.a $(PKGDIR)
	printf "Name: yyjson\nDescription: %s\nVersion: %s\nCflags: -I%s\nLibs: -L%s -l:libyyjson.a"\
		"$$(perl -0777 -ne '/# Introduction\s*(\[.*\s*)*([^#]*)#/ && print $2;' < $(YYJSONDIR)/README.md | head -n 1)"\
		"$$(sed -n 's/\#*\S*\(.*\)\s*(.*/\1/p' < $(YYJSONDIR)/CHANGELOG.md | head -n 1)"\
		$(YYJSONSDIR)\
		$(YYJSONBDIR)\
		> $(PKGDIR)/yyjson.pc

compile_flags.txt: $(PKGDIR)/yyjson.pc | $(IDIR)
	$(PKGOBJ) $(libs) | tr ' ' '\n' > $@
	echo "-I$(IDIR)" >> $@

setup: $(YYJSONDIR) $(IDIR)/$(KMAP).h $(PKGDIR)/yyjson.pc compile_flags.txt

$(odir)/%.o: $(SDIR)/%.c $(headers) setup | $(SDIR) $(odir)
	$(CC) $(cflags) $$($(PKGOBJ) $(libs)) -I$(IDIR) -c $< -o $@

$(odir)/%.opp: $(SDIR)/%.cpp $(headers) setup | $(SDIR) $(odir)
	$(CPPC) $(cppflags) $$($(PKGOBJ) $(libs)) -I$(IDIR) -c $< -o $@

$(ODIR)/.mode_$(MODE): $(ODIR)
	-rm -f $(ODIR)/.mode_*
	touch $@

$(TARGET): $(target)
$(target): $(objects) setup $(ODIR)/.mode_$(MODE) | $(TDIR)
	$(CPPC) $(objects) $$($(PKGT) $(libs)) -o $@
	$(post)

$(GDIR):
	mkdir -p $@

$(DJB2): $(djb2)
$(djb2): $(SDIR)/util.c $(SDIR)/util.h | $(GDIR)
	printf '#include"$(SDIR)/util.h"\n#include<stdio.h>\nint main(const int c,const char**v){printf("%%luul\\n",mzd_str_djb2(c==1?"":v[1]));}'\
		| $(CC) -Wall -O3 -x c - $(SDIR)/util.c -o $@

execute: $(TARGET)
	$(call execute)

ldd: $(target)
	ldd $^

valgrind: $(TARGET)
	$(VALGRIND) $(call execute)

djb2: $(djb2)
	$(djb2) $(ARGS)

clean:
	-rm -rf $(GDIR)
	-rm -f valgrind*.txt* vgcore.*

clean-deep: clean
	-rm -rf $(TDIR)
	-rm -rf $(ODIR)

reset: clean-deep
	-rm -rf $(IDIR)
	-rm -rf $(LDIR)
	-rm -rf $(PKGDIR)
	-rm -rf $(TPDIR)
	-rm -rf compile_flags.txt

.PRECIOUS: $(TARGET) $(objects)
.PHONY: default all setup ldd valgrind djb2 clean clean-deep reset

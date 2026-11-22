# =====================================================================
#  Makefile  -  FILE 4 of 4
# =====================================================================
#
#  WHAT THIS FILE IS
#  -----------------
#  A recipe for the "make" tool. Instead of typing a long compiler
#  command every time, you type one word on the pi-Stomp:
#
#       make            compile the C code into lv2_simple_volume.so
#       make install    copy the finished bundle to where MOD looks
#       make clean      delete the compiled file to start fresh
#       make uninstall  remove the pedal from MOD's folder
#
#  WHY WE COMPILE ON THE PI-STOMP ITSELF
#  -------------------------------------
#  The Raspberry Pi has an ARM processor; your Windows PC has an Intel
#  or AMD one. They speak different machine languages, so code compiled
#  on the PC won't run on the Pi. Compiling ON the Pi avoids that.
#
#  HOW TO READ IT
#  --------------
#    NAME = value     sets a variable. Use it later as $(NAME).
#    target: needs    "to make 'target', you first need 'needs'"
#    <TAB>command     the command(s) that build the target.
#                     These lines MUST start with a TAB, not spaces.
#                     This is the #1 Makefile mistake.
# =====================================================================


# ---- Variables ------------------------------------------------------

# The bundle folder (the plugin "box").
BUNDLE  = lv2_simple_volume.lv2

# Where to install. $(HOME) is your home folder: /home/pistomp
# So the pedal ends up in /home/pistomp/.lv2/lv2_simple_volume.lv2/
# ("?=" means "use this unless something else was already set")
# The dot in ".lv2" makes it a hidden folder; see it with:  ls -a
LV2DIR ?= $(HOME)/.lv2

# Compiler settings (CFLAGS = "C flags"):
#   -O2          optimise: make the code run fast
#   -ffast-math  allow faster, very slightly less strict maths
#   -Wall        show all warnings - helpful when learning
#   -fPIC        "position independent code" - required for a .so
#                that gets loaded into another program (MOD)
#   -std=c99     the version of the C language we're using
CFLAGS ?= -O2 -ffast-math
CFLAGS += -Wall -fPIC -std=c99


# ---- Targets (the recipes) -------------------------------------------

# "all" is the default: what plain "make" builds.
all: $(BUNDLE)/lv2_simple_volume.so

# How to build the .so: it depends on the .c file, so if you edit the
# .c, make knows to rebuild it.
#   $(CC)    the C compiler (gcc on the Pi)
#   -shared  build a shared library (.so), not a normal program
#   $<       shorthand for "the first thing this needs" = the .c file
#   -o $@    output to "the target" = lv2_simple_volume.lv2/lv2_simple_volume.so
#   -lm      link the maths library (for powf and expf)
$(BUNDLE)/lv2_simple_volume.so: lv2_simple_volume.c
	$(CC) $(CFLAGS) -shared $< -o $@ -lm

# Copy the whole bundle folder into MOD's plugin folder.
#   mkdir -p   create the folder if it isn't there (no error if it is)
#   cp -r      copy the folder and everything in it
install: all
	mkdir -p $(LV2DIR)
	cp -r $(BUNDLE) $(LV2DIR)/

# Remove the installed pedal.
uninstall:
	rm -rf $(LV2DIR)/$(BUNDLE)

# Delete the compiled file (the source files are kept).
clean:
	rm -f $(BUNDLE)/lv2_simple_volume.so

# These names are commands, not files.
.PHONY: all install uninstall clean

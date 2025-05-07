#----------------------------------------------------------------------------
# lunch.mak
# copyright (c) innovaphone 2015
#----------------------------------------------------------------------------

OUT = lunch

include common/build/build.mak

include sdk/platform/sdk-defs.mak

include web1/appwebsocket/appwebsocket.mak
include web1/config/config.mak
include web1/fonts/fonts.mak
include web1/lib1/lib1.mak
include web1/ui1.lib/ui1.lib.mak
include web1/ui1.svg/ui1.svg.mak
include web1/ui1.scrolling/ui1.scrolling.mak

include lunch/submodules.mak

APP_OBJS += $(OUTDIR)/obj/lunch-main.o
$(OUTDIR)/obj/lunch-main.o: lunch-main.cpp

include sdk/platform/sdk.mak



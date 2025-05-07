
APPWEBPATH = lunch/apps

STARTHTMLS += \
	$(APPWEBPATH)/innovaphone-lunchAdmin.htm \
	$(APPWEBPATH)/innovaphone-lunch.htm

APPWEBSRC_NOZIP += \
	$(APPWEBPATH)/innovaphone-lunchAdmin.png \
	$(APPWEBPATH)/innovaphone-lunch.png \
	$(APPWEBPATH)/fruehstueck.pdf \
	$(APPWEBPATH)/allergene.pdf

APPWEBSRC_ZIP += \
	$(APPWEBPATH)/innovaphone-lunch.js \
	$(APPWEBPATH)/innovaphone-lunchAdmin.js \
	$(APPWEBPATH)/innovaphone-lunch.css \
	$(APPWEBPATH)/innovaphone-lunchTexts.js

MANAGERSRC = \
	$(APPWEBPATH)/innovaphone.lunchManager.js \
	$(APPWEBPATH)/innovaphone.lunchManager.css \
	$(APPWEBPATH)/innovaphone.lunchManagerTexts.js \

APPWEBSRC = $(APPWEBSRC_ZIP) $(MANAGERSRC) $(APPWEBSRC_NOZIP)

$(OUTDIR)/obj/apps_start.cpp: $(STARTHTMLS) $(OUTDIR)/obj/innovaphone.manifest
		$(IP_SRC)/exe/httpfiles -k -d $(OUTDIR)/obj -t $(OUTDIR) -o $(OUTDIR)/obj/apps_start.cpp \
	-s 0 $(subst $(APPWEBPATH)/,,$(STARTHTMLS))

$(OUTDIR)/obj/apps.cpp: $(APPWEBSRC)
		$(IP_SRC)/exe/httpfiles -k -d $(APPWEBPATH) -t $(OUTDIR) -o $(OUTDIR)/obj/apps.cpp \
	-s 0,HTTP_GZIP $(subst $(APPWEBPATH)/,,$(APPWEBSRC_ZIP) $(MANAGERSRC)) \
	-s 0 $(subst $(APPWEBPATH)/,,$(APPWEBSRC_NOZIP))

APP_OBJS += $(OUTDIR)/obj/apps_start.o
$(OUTDIR)/obj/apps_start.o: $(OUTDIR)/obj/apps_start.cpp
APP_OBJS += $(OUTDIR)/obj/apps.o
$(OUTDIR)/obj/apps.o: $(OUTDIR)/obj/apps.cpp

include lunch/apps/apps.mak

APP_OBJS += $(OUTDIR)/obj/lunch.o
$(OUTDIR)/obj/lunch.o: lunch/lunch.cpp $(OUTDIR)/lunch/lunch.png force

APP_OBJS += $(OUTDIR)/obj/lunch_tasks.o
$(OUTDIR)/obj/lunch_tasks.o: lunch/lunch_tasks.cpp

$(OUTDIR)/lunch/lunch.png: lunch/lunch.png
	copy lunch\lunch.png $(OUTDIR)\lunch\lunch.png

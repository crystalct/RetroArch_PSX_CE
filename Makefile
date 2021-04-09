all:
	@cd RetroArch_Orig; $(MAKE) -f Makefile.ps3

init:
	@echo Init RetroArch Submodule...
	@cd RetroArch_Orig; git reset --hard; git pull; cp -rf ../Makefile.ps3 .;
	@echo Patching RetroArch....
	@./RetroArchPatch.sh

clean:
	@cd RetroArch_Orig; $(MAKE) -f Makefile.ps3 clean
	
test:
	@cd RetroArch_Orig;
	@./RetroArchPatch.sh
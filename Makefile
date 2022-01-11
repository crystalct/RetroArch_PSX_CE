all:
	@cd RetroArch_Orig; $(MAKE) -f Makefile.ps3

init:
	@echo Init RetroArch Submodule...
	@git submodule init; git submodule update; cd RetroArch_Orig; git reset --hard; git pull origin master; cp -rf ../Makefile.ps3 .;
	@echo Patching RetroArch....
	@./RetroArchPatch.sh

clean:
	@cd RetroArch_Orig; $(MAKE) -f Makefile.ps3 clean
	
test:
	@cd RetroArch_Orig;
	@./RetroArchPatch.sh
	
	
	
	git submodule init; git submodule update; cp -rf Makefile.ps3 ./RetroArch_Orig;
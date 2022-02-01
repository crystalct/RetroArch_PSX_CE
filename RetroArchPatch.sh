#!/bin/sh -e

count=`sed -n ':a;N;$!ba;s/.*\(#elif defined(WIIU)\).*/\1/p' RetroArch_Orig/config.def.h | wc -l`; if [[ $count -eq 0 ]]; then echo "elif defined(WIIU)"; fi
sed ':a;N;$!ba;s/#elif defined(WIIU)/#elif defined(__PS3__)\n#define DEFAULT_BUILDBOT_SERVER_URL "http:\/\/cristaldi.github.unict.it\/"\n#elif defined(WIIU)/' RetroArch_Orig/config.def.h > config.def.h.mod
cat config.def.h.mod > RetroArch_Orig/config.def.h
rm config.def.h.mod
count=`grep "config_set_defaults(global_get_ptr())" RetroArch_Orig/retroarch.c | wc -l`; if [[ $count -eq 0 ]]; then echo "Errore global_get_ptr()"; fi
sed ':a;N;$!ba;s/config_set_defaults(global_get_ptr());/config_set_defaults(global_get_ptr());\n         command_event(CMD_EVENT_RESTART_RETROARCH, NULL);/' RetroArch_Orig/retroarch.c > retroarch.c.mod
count=`grep "case CMD_EVENT_UNLOAD_CORE:" RetroArch_Orig/retroarch.c | wc -l`; if [[ $count -eq 0 ]]; then echo "Errore CMD_EVENT_UNLOAD_CORE"; fi
sed ':a;N;$!ba;s/case CMD_EVENT_UNLOAD_CORE:/case CMD_EVENT_UNLOAD_CORE:\n         command_event(CMD_EVENT_RESTART_RETROARCH, NULL);\n         break;/' retroarch.c.mod > RetroArch_Orig/retroarch.c
#cat retroarch.c.mod > RetroArch_Orig/retroarch.c
rm retroarch.c.mod
count=`grep ".index-extended" RetroArch_Orig/tasks/task_core_updater.c | wc -l`; if [[ $count -eq 0 ]]; then echo ".index-extended"; fi
sed ':a;N;$!ba;s/.index-extended/index-extended.txt/' RetroArch_Orig/tasks/task_core_updater.c > task_core_updater.c.mod
cat task_core_updater.c.mod > RetroArch_Orig/tasks/task_core_updater.c
rm task_core_updater.c.mod
cd RetroArch_Orig;
git apply ../diff.patch
cp ../load_config_file_PS3.c .
cat ../copy_file_PS3.c >> frontend/drivers/platform_ps3.c
echo "Patching done."

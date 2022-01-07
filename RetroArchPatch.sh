#!/bin/sh -e

sed ':a;N;$!ba;s/#elif defined(WIIU)/#elif defined(__PS3__)\n#define DEFAULT_BUILDBOT_SERVER_URL "http:\/\/cristaldi.github.unict.it\/"\n#elif defined(WIIU)/' RetroArch_Orig/config.def.h > config.def.h.mod
cat config.def.h.mod > RetroArch_Orig/config.def.h
rm config.def.h.mod
sed ':a;N;$!ba;s/config_set_defaults(\&p_rarch->g_extern);/config_set_defaults(\&p_rarch->g_extern);\n         command_event(CMD_EVENT_RESTART_RETROARCH, NULL);/' RetroArch_Orig/retroarch.c > retroarch.c.mod
sed ':a;N;$!ba;s/case CMD_EVENT_UNLOAD_CORE:/case CMD_EVENT_UNLOAD_CORE:\n         command_event(CMD_EVENT_RESTART_RETROARCH, NULL);\n         break;/' retroarch.c.mod > RetroArch_Orig/retroarch.c
#cat retroarch.c.mod > RetroArch_Orig/retroarch.c
rm retroarch.c.mod
sed ':a;N;$!ba;s/.index-extended/index-extended.txt/' RetroArch_Orig/tasks/task_core_updater.c > task_core_updater.c.mod
cat task_core_updater.c.mod > RetroArch_Orig/tasks/task_core_updater.c
rm task_core_updater.c.mod
cd RetroArch_Orig;
git apply ../diff.patch
echo "Patching done."

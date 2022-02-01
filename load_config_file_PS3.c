char dest_txt[256];
int i;
fill_pathname_join(dest_txt, g_defaults.dirs[DEFAULT_DIR_PORT], FILE_PATH_MAIN_CONFIG, 256);
settings_t *settings = config_get_ptr();
settings->bools.config_save_on_exit = false;
i = copy_file_PS3(args->config_path, dest_txt);
if (i == 0) {
	command_event(CMD_EVENT_RESTART_RETROARCH, NULL);
	return;
}
if (i ==1)
	snprintf(dest_txt,  sizeof(dest_txt), "%s - %s", args->config_path, MENU_ENUM_LABEL_VALUE_QT_FILE_READ_OPEN_FAILED);
else 
	snprintf(dest_txt,  sizeof(dest_txt), "%s - %s", dest_txt, MENU_ENUM_LABEL_VALUE_QT_FILE_WRITE_OPEN_FAILED);

runloop_msg_queue_push(dest_txt, 1, 220, true, NULL, MESSAGE_QUEUE_ICON_DEFAULT, MESSAGE_QUEUE_CATEGORY_ERROR);
return;

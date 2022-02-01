int copy_file_PS3(const char *source, const char *dest)
{
	char msg[256];	
    FILE *fptr1, *fptr2; 
    char c; 
  
    
    fptr1 = fopen(source, "r"); 
    if (fptr1 == NULL) 
    { 
        printf("%s - %s\n", source, MENU_ENUM_LABEL_VALUE_QT_FILE_READ_OPEN_FAILED); 
        return 1; 
    } 
  
    fptr2 = fopen(dest, "w"); 
    if (fptr2 == NULL) 
    { 
        printf("%s - %s\n", dest, MENU_ENUM_LABEL_VALUE_QT_FILE_WRITE_OPEN_FAILED); 
        return 2; 
    } 
  
    // Read contents from file 
    c = fgetc(fptr1); 
    while (c != EOF) 
    { 
        fputc(c, fptr2); 
        c = fgetc(fptr1); 
    } 
  
    fclose(fptr1); 
    fclose(fptr2); 
	
	return 0;
}
 
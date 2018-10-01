#include <stdio.h>
void DisplayValue(unsigned int value){
    char str[snprintf(NULL, 0,"%d",value)+1];
    sprintf(str, "%d", value);
    MessageBox(NULL, str, TEXT("Value"), MB_OK | MB_ICONERROR);
}

int ErrorCode(TCHAR *title, int value){
    char str[snprintf(NULL, 0,"%d",value)+1];
    sprintf(str, "%d", value);
    MessageBox(NULL, str, title, MB_OK | MB_ICONERROR);
}

void DisplayPointer(void *value){
    char str[snprintf(NULL, 0,"%p",value)+1];
    sprintf(str, "%p", value);
    MessageBox(NULL, str, TEXT("Value"), MB_OK | MB_ICONERROR);

}
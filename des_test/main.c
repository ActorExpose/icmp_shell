/*@file wjcdestest.cpp 
WjcDes test 
complied ok with vc++6.0,mingGW 
*/ 
#include <windows.h>
#include <stdio.h> 
#include "des.h"

int main() 
{ 
    char *key = "sincoder";
    char word[64]="mynamexxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";/*����*/
    char buff[65]={0};
    printf("����=%s\n",word); 
    /*1.���ü���key*/ 
    DesEnter(word,buff,64,key,FALSE);
    buff[64] = 0;
    printf("����֮��,����=%s\n",buff); 
    /*3.DES����*/ 
    DesEnter(word,buff,64,key,TRUE); 
    printf("����֮��,����=%s\n",word); 
    return 0; 
} 
/* 
���н��: 
des demo... 
����=myname 
����֮��,����=.S ��$��. 
����֮��,����=myname 
*/ 



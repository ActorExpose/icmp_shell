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
    char word[64]="mynamexxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxt";/*����*/
    char buff[65]={0};
    int len ;
    printf("����=%s\n",word); 
    /*1.���ü���key*/ 
    len = DesEnter(word,buff,strlen(word),key,FALSE);
    buff[64] = 0;
    printf("����֮��,����=%s  len : %d \n",buff,len); 
    /*3.DES����*/ 
    DesEnter(buff,word,len,key,TRUE);   //���ĳ��ȱ���Ϊ 8 �ı��� 
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



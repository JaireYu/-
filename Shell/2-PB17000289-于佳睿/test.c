#include<stdio.h>
#include<unistd.h>
#include<sys/syscall.h>
int main(){
	char str[100];
	int num;
	int len;
	printf("Give me a string\n");
	scanf("%s", str);
	for(len = 0; str[len] !='\0'; len ++){
	}
	syscall(328, str, len, &num);
	syscall(327, num);
}

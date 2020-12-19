//
// Created by wonder on 2020/12/19.
//

#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int token;  //当前token
char * src,*old_src;
int pool_size;
int line;

/**
 * 用于词法分析,获取下一个标记
 */
void next(){
    token == *src++;
}
/**
 * 解析表达式
 */
void expression(int level){

}
/**
 * 语法分析入口,分析整个C程序
 */
void program(){
    next();
    while(token > 0){
        printf("token is %c\n",token);
        next();
    }
}
/**
 * 虚拟机入口,用于解释目标代码
 * @return
 */
int eval(){
    return 0;
}

int main(int argc,char *argv[]){

    int i,fd;
    argc--;argv++;
    pool_size = 256 * 1024;
    line = 1;

    if((fd = open(*argv,0)) < 0){
        printf("could not open (%s)\n",*argv);
        return -1;
    }
    if(!(src = old_src =(char *)malloc(pool_size * sizeof(char)))){
        printf("could not malloc(%d) for source area\n",pool_size);
        return -1;
    }

    if((i = read(fd,src,pool_size-1)) <= 0){
        printf("read() returned %d\n",i);
        return -1;
    }
    src[i] = '\0';
    close(fd);

    program();
    return eval();
}


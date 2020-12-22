//
// Created by wonder on 2020/12/19.
//

#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define int long long   // work with 64bit system

int token;  //当前token
char * src,*old_src;
int pool_size;
int line;

// virtual machine area
int * text; //text segment
int * old_text; //dump text segment
int * stack; // stack
char * data; // data segment

// virtual machine registers
int *pc,*bp,*sp,ax,cycle;

// for lexical analysis
int token_val;      // value of current token
int * current_id;    // current parsed ID
int * symbols;      // symbol table
int * idmain;
// fields of identifier
enum {
    Token,Hash,Name,Type,Class,Value,BType,BClass,BValue,IdSize
};
// instructions
enum {
    LEA,IMM,JMP,CALL,JZ,JNZ,ENT,ADJ,LEV,LI,LC,SI,SC,PUSH,
    OR,XOR,AND,EQ,NE,LT,GT,LE,GE,SHL,SHR,ADD,SUB,MUL,DIV,MOD,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT
};
// tokens and classes
enum {
    Num = 128,Fun,Sys,Glo,Loc,Id,
    Char,Else,Enum,If,Int,Return,Sizeof,While,
    Assign,Cond,Lor,Lan,Or,Xor,And,Eq,Ne,Lt,Gt,Le,Ge,Shl,Shr,Add,Sub,Mul,Div,Mod,Inc,Dec,Brak
};
// types of variable/function
enum {
    CHAR,INT,PTR
};
/**
 * 用于词法分析,获取下一个标记
 */
void next(){
    char * last_pos;
    int hash;

    while(token = *src){
        ++src;

        if(token == '\n') ++line;
        else if(token == '#'){  //not support it,skip macro
            while(*src != 0 && *src != '\n'){
                src++;
            }
        }else if((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')){
            // parse identifier
            last_pos = src - 1;
            hash = token;

            while((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')){
                hash = hash * 147 + *src;
                src++;
            }

            //look for existing identifier,linear search
            current_id = symbols;
            while(current_id[Token]){
                if(current_id[Hash] == hash && !memcmp((char *)current_id[Name],last_pos,src - last_pos)){
                    //found one,return
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }

            // store new ID
            current_id[Name] = (int )last_pos;
            current_id[Name] = hash;
            token = current_id[Token] = Id;

            return;
        }else if(token >= '0' && token <= '9'){
            // parse number,three kinds: dec(123) hex(0x123) oct(017)
            token_val = token - '0';
            if(token_val > 0){
                // dec,starts with [1-9]
                while(*src >= '0' && *src <= '9'){
                    token_val = token_val * 10 + *src++ - '0';
                }
            }else{
                // starts with number 0
                if(*src == 'x' || *src == 'X'){
                    // hex
                    token = *++src;
                    while((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')){
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                }else{
                    // oct
                    while(*src >= '0' && *src <= '7'){
                        token_val = token_val * 8 + *src++ - '0';
                    }
                }
            }
            token = Num;
            return;
        }else if(token == '"' || token == '\''){
            // parse string literal,currently,the only supported escape
            // character is '\n', store the string literal into data
            last_pos = data;
            while(*src != 0 && *src != token){
                token_val = *src++;
                if(token_val == '\\'){
                    // escape character
                    token_val = *src++;
                    if(token_val == 'n'){
                        token_val = '\n';
                    }
                }
                if(token == '"'){
                    *data++ = token_val;
                }
            }
            src++;
            // if it is a single character,return Num token
            if(token == '"'){
                token_val = (int)last_pos;
            }else{
                token = Num;
            }
            return;
        }else if(token == '/'){
            if(*src == '/') {
                // skip comments
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            }else{
                // divide operator
                token = Div;
                return;
            }
        }else if(token == '='){
            // parse '==' and '='
            if(*src == '='){
                src++;
                token = Eq;
            }else{
                token = Assign;
            }
            return;
        }else if(token == '+'){
            // parse '+' and '++'
            if(*src == '+'){
                src++;
                token = Inc;
            }else{
                token = Add;
            }
            return;
        }else if(token == '-'){
            // parse '-' and '--'
            if(*src == '-'){
                src++;
                token = Dec;
            }else{
                token = Sub;
            }
            return;
        }else if(token == '!'){
            // parse '!='
            if(*src == '='){
                src++;
                token = Ne;
            }
            return;
        }else if(token == '<'){
            // parse '<=','<<' or '<'
            if(*src == '='){
                src++;
                token = Le;
            }else if(*src == '<'){
                src++;
                token = Shl;
            }else{
                token = Lt;
            }
            return;
        }else if(token == '>'){
            // parse '>=', '>>' or '>'
            if(*src == '='){
                src++;
                token = Ge;
            }else if(*src == '>'){
                src++;
                token = Shr;
            }else{
                token = Gt;
            }
            return;
        }else if(token == '|'){
            // parse '|' or '||'
            if(*src == '|'){
                src++;
                token = Lor;
            }else{
                token = Or;
            }
            return;
        }else if(token == '&'){
            // parse '&' and '&&'
            if(*src == '&'){
                src++;
                token = Lan;
            }else{
                token = And;
            }
            return;
        }else if(token == '^'){
            token = Xor;
            return;
        }else if(token == '%'){
            token = Mod;
            return;
        }else if(token == '*'){
            token = Mul;
            return;
        }else if(token == '['){
            token = Brak;
            return;
        }else if(token == '?'){
            token = Cond;
            return;
        }else if(token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')'
                || token == ']' || token == ',' || token == ':'){
            // directly return the character as token
            return;
        }
    }

    return;
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
    int op,*tmp;
    while(1){
        op = *pc++;

        if(op == IMM) ax = *pc++;                           // load immediate value to ax
        else if(op == LC) ax = *(char *)ax;                 // load character to ax,address in ax
        else if(op == LI) ax = *(int *)ax;                  // load integer to ax,address in ax
        else if(op == SC) ax = *(char *)*sp++ = ax;         // save character to address,value in ax,address on stack
        else if(op == SI) *(int *)*sp++ = ax;               // save integer to address,value in ax,address on stack
        else if(op == PUSH) *--sp = ax;                     // push the value of ax onto the stack
        else if(op == JMP) pc = (int *)*pc;                 // jump to the address
        else if(op == JZ) pc = ax ? pc + 1 : (int *)*pc;    // jump if ax is zero
        else if(op == JNZ) pc = ax ? (int *)*pc : pc + 1;   // jump if ax is not zero
        else if(op == CALL){
            *--sp = (int)(pc + 1);
            pc = (int *)*pc;
        }else if(op == ENT){     // make new stack frame
            *--sp = (int)bp;
            bp = sp;
            sp = sp - *pc++;
        }else if(op == ADJ) sp = sp + *pc++;                // add esp,<size>
        else if(op == LEV){     // restore call frame and pc
            sp = bp;
            bp = (int *)*sp++;
            pc = (int *)*sp++;
        }else if(op == LEA) ax = *(bp + *pc++);         // load address for arguments
        

        else if(op == OR) ax = *sp++ | ax;
        else if(op == XOR) ax = *sp++ ^ ax;
        else if(op == AND) ax = *sp++ & ax;
        else if(op == EQ) ax = *sp++ == ax;
        else if(op == NE) ax = *sp++ != ax;
        else if(op == LT) ax = *sp++ < ax;
        else if(op == LE) ax = *sp++ <= ax;
        else if(op == GT) ax = *sp++ > ax;
        else if(op == GE) ax = *sp++ >= ax;
        else if(op == SHL) ax = *sp++ << ax;
        else if(op == SHR) ax = *sp++ >> ax;
        else if(op == ADD) ax = *sp++ + ax;
        else if(op == SUB) ax = *sp++ - ax;
        else if(op == MUL) ax = *sp++ * ax;
        else if(op == DIV) ax = *sp++ / ax;
        else if(op == MOD) ax = *sp++ % ax;


        else if(op == EXIT){
            printf("exit(%d)\n",*sp);
            return *sp;
        }else if(op == OPEN){
            ax = open((char *)sp[1],sp[0]);
        }else if(op == CLOS){
            ax = close(*sp);
        }else if(op == READ){
            ax = read(sp[2],(char *)sp[1],*sp);
        }else if(op == PRTF){
            tmp = sp + pc[1];
            ax = printf((char *)tmp[-1],tmp[-2],tmp[-3],tmp[-4],tmp[-5],tmp[-6]);
        }else if(op == MALC){
            ax = (int)malloc(*sp);
        }else if(op == MSET){
            ax = (int)memset((char *)sp[2],sp[1],*sp);
        }else if(op == MCMP){
            ax = memcmp((char *)sp[2],(char *)sp[1],*sp);
        }else{
            printf("unknow instruction:%d\n",op);
            return -1;
        }
    }
    return 0;
}

#undef int // For Mac

int main(int argc,char *argv[]){
    
    #define int long long // work with 64bit system

    int i,fd;
    argc--;argv++;
    pool_size = 256 * 1024;
    line = 1;

    if((fd = open(*argv,0)) < 0){
        printf("could not open (%s)\n",*argv);
        return -1;
    }
    if(!(src = old_src = (char *)malloc(pool_size))){
        printf("could not malloc(%d) for source area\n",pool_size);
        return -1;
    }

    if((i = read(fd,src,pool_size-1)) <= 0){
        printf("read() returned %d\n",i);
        return -1;
    }
    src[i] = 0;
    close(fd);

    //allocate memory for virtual machine
    if(!(text = old_text =(int *)malloc(pool_size))){
        printf("could not malloc(%d) for text area\n",pool_size);
        return -1;
    }
    if(!(data =(char *)malloc(pool_size))){
        printf("could not malloc(%d) for data area\n",pool_size);
        return -1;
     }
    if(!(stack = (int *)malloc(pool_size))){
        printf("could not malloc(%d) for stack area\n",pool_size);
        return -1;
    }

    memset(text,0,pool_size);
    memset(data,0,pool_size);
    memset(stack,0,pool_size);
    
    // init register
    bp = sp = (int *)((int)stack + pool_size);
    ax = 0;

    /*
    //Test
    i = 0;
    text[i++] = IMM;
    text[i++] = 2;
    text[i++] = PUSH;
    text[i++] = IMM;
    text[i++] = 3;
    text[i++] = ADD;
    text[i++] = PUSH;
    text[i++] = EXIT;
    pc = text;
    */
    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";
    // add keywords to symbol stable
    i = Char;
    while (i <= While){
        next();
        current_id[Token] = i++;
    }
    //add library to symbol table
    i = OPEN;
    while (i <= EXIT){
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next();current_id[Token] = Char;    // handle void type
    next();idmain = current_id;         // keep track of main

    program();

    return eval();
}


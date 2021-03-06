/*
 ** $Id: print.c,v 1.55a 2006/05/31 13:30:05 lhf Exp $
 ** print bytecodes
 ** See Copyright Notice in lua.h
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define luac_c
#define LUA_CORE

#include "ldebug.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lundump.h"

// code from http://isthe.com/chongo/src/fnv/hash_32.c
typedef lu_int32 Fnv32_t;
/*
 * fnv_32_buf - perform a 32 bit Fowler/Noll/Vo hash on a buffer
 *
 * input:
 *	buf	- start of buffer to hash
 *	len	- length of buffer in octets
 *	hval	- previous hash value or 0 if first call
 *
 * returns:
 *	32 bit hash as a static hash type
 *
 * NOTE: To use the 32 bit FNV-0 historic hash, use FNV0_32_INIT as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 *
 * NOTE: To use the recommended 32 bit FNV-1 hash, use FNV1_32_INIT as the hval
 *	 argument on the first call to either fnv_32_buf() or fnv_32_str().
 */
Fnv32_t fnv_32_buf(const void *buf, size_t len, Fnv32_t hval)
{
    unsigned char *bp = (unsigned char *)buf;	/* start of buffer */
    unsigned char *be = bp + len;		/* beyond end of buffer */
    /*
     * FNV-1 hash each octet in the buffer
     */
    while (bp < be) {
        hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
        /* xor the bottom with the current octet */
        hval ^= (Fnv32_t)*bp++;
    }
    /* return our new hash value */
    return hval;
}


#define PrintFunction	luaU_print

#define Sizeof(x)	((int)sizeof(x))
#define VOID(p)		((const void*)(p))

static void PrintString(const TString* ts)
{
    const char* s=getstr(ts);
    size_t i,n=ts->tsv.len;
    putchar('"');
    for (i=0; i<n; i++)
    {
        int c=s[i];
        switch (c)
        {
            case '"': printf("\\\""); break;
            case '\\': printf("\\\\"); break;
            case '\a': printf("\\a"); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            case '\v': printf("\\v"); break;
            default:	if (isprint((unsigned char)c))
                            putchar(c);
                        else
                            printf("\\%03u",(unsigned char)c);
        }
    }
    putchar('"');
}

static Fnv32_t PrintConstant(const Proto* f, int i)
{
    Fnv32_t hash = 0;
    const TValue* o=&f->k[i];
    switch (ttype(o))
    {
        case LUA_TNIL:
            printf("nil");
            break;
        case LUA_TBOOLEAN:
            {
                int v = bvalue(o) ? 1 : 0;
                printf(bvalue(o) ? "true" : "false");
                hash = fnv_32_buf(&v, sizeof(v), hash);
                break;
            }
        case LUA_TNUMBER:
            {
                double v = nvalue(o);
                printf(LUA_NUMBER_FMT, v);
                hash =  fnv_32_buf(&v, sizeof(v), hash);
                break;
            }
        case LUA_TSTRING:
            {
                const TString* s = rawtsvalue(o);
                PrintString(s);
                hash = fnv_32_buf(getstr(s), s->tsv.len, hash);
                break;
            }
        default:				/* cannot happen */
            printf("? type=%d",ttype(o));
            break;
    }
    return hash;
}

static void PrintCode(const Proto* f)
{
    Fnv32_t hash = 0;
    printf("opcode = {\n");
    const Instruction* code=f->code;
    int pc,n=f->sizecode;
    for (pc=0; pc<n; pc++)
    {
        Instruction i=code[pc];
        hash = fnv_32_buf(&i, 4, hash);
        OpCode o=GET_OPCODE(i);
        int a=GETARG_A(i);
        int b=GETARG_B(i);
        int c=GETARG_C(i);
        int bx=GETARG_Bx(i);
        int sbx=GETARG_sBx(i);
        int line=getline(f,pc);
        /*printf("\t%d\t",pc+1);*/
        printf("{\n");
        /*if (line>0) printf("[%d]\t",line); else printf("[-]\t");*/
        printf("line=%d,\n", line);
        /*printf("%-9s\t",luaP_opnames[o]);*/
        printf("opname=\"%s\",\n", luaP_opnames[o]);
        printf("opargs=\"");
        switch (getOpMode(o))
        {
            case iABC:
                printf("%d",a);
                if (getBMode(o)!=OpArgN) printf(" %d",ISK(b) ? (-1-INDEXK(b)) : b);
                if (getCMode(o)!=OpArgN) printf(" %d",ISK(c) ? (-1-INDEXK(c)) : c);
                break;
            case iABx:
                if (getBMode(o)==OpArgK) printf("%d %d",a,-1-bx); else printf("%d %d",a,bx);
                break;
            case iAsBx:
                if (o==OP_JMP) printf("%d",sbx); else printf("%d %d",a,sbx);
                break;
        }
        printf("\",\n"); // end of opargs
        printf("constant = [=====[");
        switch (o)
        {
            case OP_LOADK:
                {
                    int v = PrintConstant(f,bx);
                    hash = fnv_32_buf(&v, sizeof(v), hash);
                    break;
                }
            case OP_GETUPVAL:
            case OP_SETUPVAL:
                {
                    //printf("\t; %s", (f->sizeupvalues>0) ? getstr(f->upvalues[b]) : "-");
                    const char* s = (f->sizeupvalues>0) ? getstr(f->upvalues[b]) : "-";
                    printf("%s", s);
                    hash = fnv_32_buf(s, strlen(s), hash);
                    break;
                }
            case OP_GETGLOBAL:
            case OP_SETGLOBAL:
                {
                    //printf("\t; %s",svalue(&f->k[bx]));
                    const char* s = svalue(&f->k[bx]);
                    printf("%s", s);
                    hash = fnv_32_buf(s, strlen(s), hash);
                    break;
                }
            case OP_GETTABLE:
            case OP_SELF:
                if (ISK(c)) 
                { 
                    // printf("\t; "); 
                    const int v = PrintConstant(f, INDEXK(c)); 
                    hash = fnv_32_buf(&v, sizeof(v), hash);
                }
                break;
            case OP_SETTABLE:
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_POW:
            case OP_EQ:
            case OP_LT:
            case OP_LE:
                if (ISK(b) || ISK(c))
                {
                    // printf("\t; ");
                    int v = 0;
                    if (ISK(b)) 
                    {
                        v += PrintConstant(f,INDEXK(b)); 
                    } 
                    else 
                    {
                        printf("-");
                    }
                    printf(" ");
                    if (ISK(c)) 
                    {
                        v += PrintConstant(f,INDEXK(c)); 
                    }
                    else 
                    {
                        printf("-");
                    }
                    hash = fnv_32_buf(&v, sizeof(v), hash);
                }
                break;
            case OP_JMP:
            case OP_FORLOOP:
            case OP_FORPREP:
                // printf("\t; to %d",sbx+pc+2);
                {
                    int v = sbx+pc+2;
                    printf("to %d",sbx+pc+2);
                    hash = fnv_32_buf(&v, sizeof(v), hash);
                    break;
                }
            case OP_CLOSURE:
                // printf("\t; %p",VOID(f->p[bx]));
                // TODO
                {
                    int v = (int)VOID(f->p[bx]);
                    printf("%p",VOID(f->p[bx]));
                    hash = fnv_32_buf(&v, sizeof(v), hash);
                    break;
                }
            case OP_SETLIST:
                {
                    int v = 0;
                    if (c==0) 
                    {
                        printf("%d",(int)code[++pc]);
                        v = (int)code[++pc];
                    }
                    else 
                    {
                        printf("%d",c);
                        v = c;
                    }
                    hash = fnv_32_buf(&v, sizeof(v), hash);
                    break;
                }
            default:
                break;
        }
        printf("]=====],\n"); // end of constant 
        printf("},\n"); // end of opcode
    }
    printf("hash = %d,\n", hash);
    printf("},\n");
}

#define SS(x)	(x==1)?"":"s"
#define S(x)	x,SS(x)

static void PrintHeader(const Proto* f)
{
    printf("header={\n");
    const char* s=getstr(f->source);
    if (*s=='@' || *s=='=')
        s++;
    else if (*s==LUA_SIGNATURE[0])
        s="(bstring)";
    else
        s="(string)";
    /* printf("\n%s <%s:%d,%d> (%d instruction%s, %d bytes at %p)\n",
       (f->linedefined==0)?"main":"function",s,
       f->linedefined,f->lastlinedefined,
       S(f->sizecode),f->sizecode*Sizeof(Instruction),VOID(f));
       printf("%d%s param%s, %d slot%s, %d upvalue%s, ",
       f->numparams,f->is_vararg?"+":"",SS(f->numparams),
       S(f->maxstacksize),S(f->nups));
       printf("%d local%s, %d constant%s, %d function%s\n",
       S(f->sizelocvars),S(f->sizek),S(f->sizep));
       */
    printf("source = [[%s]],\n", s);
    printf("linedefined = %d,\n", f->linedefined);
    printf("lastlinedefined = %d,\n", f->lastlinedefined);
    printf("sizecode = %d,\n", f->sizecode);
    printf("address = %p,\n", VOID(f));
    printf("numparams = %d,\n", f->numparams);
    printf("vararg = %s,\n", f->is_vararg?"true":"false");
    printf("maxstacksize = %d,\n", f->maxstacksize);
    printf("nups = %d,\n", f->nups);
    printf("sizelocvars = %d,\n", f->sizelocvars);
    printf("sizek = %d,\n", f->sizek);
    printf("sizep = %d,\n", f->sizep);
    printf("},-- end of header \n");
}

static void PrintConstants(const Proto* f)
{
    int i,n=f->sizek;
    printf("constants (%d) for %p:\n",n,VOID(f));
    for (i=0; i<n; i++)
    {
        printf("\t%d\t",i+1);
        PrintConstant(f,i);
        printf("\n");
    }
}

static void PrintLocals(const Proto* f)
{
    int i,n=f->sizelocvars;
    printf("locals (%d) for %p:\n",n,VOID(f));
    for (i=0; i<n; i++)
    {
        printf("\t%d\t%s\t%d\t%d\n",
                i,getstr(f->locvars[i].varname),f->locvars[i].startpc+1,f->locvars[i].endpc+1);
    }
}

static void PrintUpvalues(const Proto* f)
{
    int i,n=f->sizeupvalues;
    printf("upvalues (%d) for %p:\n",n,VOID(f));
    if (f->upvalues==NULL) return;
    for (i=0; i<n; i++)
    {
        printf("\t%d\t%s\n",i,getstr(f->upvalues[i]));
    }
}

void PrintFunction(const Proto* f, int full)
{
    int i,n=f->sizep;
    printf("{\n");
    PrintHeader(f);
    PrintCode(f);
    if (full)
    {
        PrintConstants(f);
        PrintLocals(f);
        PrintUpvalues(f);
    }
    for (i=0; i<n; i++) PrintFunction(f->p[i],full);
    printf("},-- end of function\n");
}

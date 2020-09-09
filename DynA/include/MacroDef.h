/***********************************************************
 * Author: Wen Li
 * Date  : 9/01/2020
 * Describe: MacroDef.h - Macro definition 
 * History:
   <1> 9/01/2020 , create
************************************************************/

#ifndef _MACRODEF_H_
#define _MACRODEF_H_ 
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include "TypeDef.h"

#define R_SUCCESS                 (0)
#define R_FAIL                    (1)

#define ALIGN_8(x)                (((x)%8)?(((x)&~7) + 8):(x))

#define INLINE                    inline


/////////////////////////////////////////////////////////////////////////////////
#define KNRM  "\x1B[1;0m"
#define KRED  "\x1B[1;31m"
#define KYEL  "\x1B[1;33m"
#define KBLU  "\x1B[1;34m"

#define ErrMsg(msg)    (std::string(KRED) + std::string(msg) + std::string(KNRM))
#define WarnMsg(msg)   (std::string(KYEL) + std::string(msg) + std::string(KNRM))
#define NormalMsg(msg) (std::string(KBLU) + std::string(msg) + std::string(KNRM))
/////////////////////////////////////////////////////////////////////////////////

#ifdef __DEBUG__
#define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...) 
#endif


#endif


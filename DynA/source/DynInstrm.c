/***********************************************************
 * Author: Wen Li
 * Date  : 7/24/2020
 * Describe: Queue.c - FIFO Queue
 * History:
   <1> 7/24/2020 , create
************************************************************/
#include "Queue.h"

void TRC_init ()
{
    InitQueue (4096);
}


void TRC_track (char* Format, ...)
{
	va_list ap;

	
	QNode *Node = InQueue ();
    if (Node == NULL)
    {
        printf ("Queue Full\r\n");
        exit (0);
    }
	
	va_start(ap, Format);
    (void)vsnprintf (Node->QBuf, sizeof(Node->QBuf), Format, ap);
    va_end(ap);

    printf ("%s \r\n", Node->QBuf);

    return;   
}




/**
 * name : Camill Folsom
 * proj : Register Allocation
 */
#include <stdio.h>
#include <strings.h>
#include "opt.h"
#include "misc.h"
#include "vars.h"
#include "opts.h"

#include "stdlib.h"
#include "io.h"
#include "flow.h"

/*
 * regalloc - perform register allocation
 */
void regalloc(int *changes)
{
   struct bblk *cblk;
   struct assemline *cline;
   extern struct bblk *top;
   char line[MAXLINE];
   char *regs[MAXREGS];
   char tempreg[MAXREGCHAR];
   varstate toalloc;
   extern struct varinfo vars[MAXVARS];
   int i, isalloc;
   int varcount = 0;

   varinit(toalloc);

   /* Loop through each block and get the registers being set or used */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      for(cline = cblk->lines; cline; cline = cline->next)
      {
         /* Copy the sets and uses to toalloc */
         if(!varempty(cline->sets))
            unionvar(toalloc, toalloc, cline->sets);

         if(!varempty(cline->uses))
            unionvar(toalloc, toalloc, cline->uses);
      }
   }

   /* Loop through the blocks and modify the assemlines */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      /* Loop through the assemlines in a block */
      for(cline = cblk->lines; cline; cline = cline->next)
      {
         if(cline->type == DEFINE_LINE)
         {
            /* Make sure a instruction is only used indirectly */
            if(!vars[varcount].indirect)
            {
               /* Allocate space for a register */
               regs[varcount] = allocstring(tempreg);
           
               /* Attempt to allocate a register */
               if(allocreg(vars[varcount].type, toalloc, regs[varcount]))
               {
                  /** 
                   * Insert a register into toalloc to make sure we select a
                   * different register on the next iteration of the loop 
                   */
                  insreg(regs[varcount], toalloc, 1);

                  /* make the new define line */
                  sprintf(line, "! %s = %s", cline->items[0], regs[varcount]);
                  free(cline->text);
                  cline->text = allocstring(line);
                  cline->type = COMMENT_LINE;

                  /* Keep track of whether a register was allocated */
                  isalloc = TRUE;
                  *changes = TRUE;
                  incropt(REGISTER_ALLOCATION);
               }
               else
                  isalloc = FALSE;
            }
            /* subtract the variables that are used indirectly from toalloc */
            else
               toalloc[2] = toalloc[2] & ~(1 << varcount);

            /** 
             * If the register allocation failed subtract the variable it 
             * failed for from toalloc 
             */
            if(!isalloc)
               toalloc[2] = toalloc[2] & ~(1 << varcount);

            /* Keeps track of the define line we are at */
            ++varcount;
         }

         /** 
          * If the line is a store and it's sets have a variable to be
          * allocated then we make a move instruction 
          */
         if(cline->type == STORE_INST && (toalloc[2] & cline->sets[2]))
         {
            /* Loop through the number of define lines */
            for(i = 0; i < varcount; i++)
            {
               /* Find the position, i, that the variable corresponds to */
               if(cline->sets[2] & (1 << i))
                  createmove(vars[i].type, cline->items[1], regs[i], cline);
            }
         }

         /**
          * If the line is a load and it's uses have a variable to be
          * allocated then we make a move instruction
          */ 
         if(cline->type == LOAD_INST && (toalloc[2] & cline->uses[2]))
         {
            for(i = 0; i < varcount; i++)
            {
               /* Find the position, i, that the variable corresponds to */
               if(cline->uses[2] & (1 << i))
                  createmove(vars[i].type, regs[i], cline->items[2], cline);
            }
         }
      }
   }

   check_cf();
}

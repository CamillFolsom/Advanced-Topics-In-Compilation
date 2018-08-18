/**
 * @name : Camill Folsom
 * @proj : Local Common Subexpression Elimination
 */
#include <stdio.h>
#include "opt.h"
#include "misc.h"
#include "vars.h"

#include <string.h>


/*
 * cseinblk - perform local common subexpression elimination in a block
 */
void cseinblk(struct bblk *cblk, int *changes)
{
   struct assemline *cline, *tline, *nline, *rline;
   extern struct instinfo insttypes[];
   char *reg, *cdst, *tdst;
   char tempreg[MAXREGCHAR];
   int replace = FALSE;
   int allocfail = FALSE;

   /* loop through each line in the block */
   for(cline = cblk->lines; cline; cline = cline->next)
   {
      /* Arithimatic instructions that are not using and setting a variable */
      if(cline->type == ARITH_INST && !varcommon(cline->sets, cline->uses))
      {
         /* Start looking for ar redundant math instruction */
         for(tline = cline->next; tline; tline = tline->next)
         {
            /**
             * Check to see if the instructions have the same name and first 
             * item 
             */
            if(tline->type == ARITH_INST &&
               strcmp(cline->items[0], tline->items[0]) == 0 &&
               strcmp(cline->items[1], tline->items[1]) == 0)
            {
               /* Check if we do not have an items[3] */
               if(strcmp(cline->items[0], "sethi") == 0 ||
                  strcmp(cline->items[0], "fnegs") == 0)
               {
                  /* Use cdst and tdst to track the destination register */
                  cdst = allocstring(cline->items[2]);
                  tdst = allocstring(tline->items[2]);
               }

               /* If we have items[3] check if the 2nd args are equal */
               else if(strcmp(cline->items[2], tline->items[2]) == 0)
               {
                  cdst = allocstring(cline->items[3]);
                  tdst = allocstring(tline->items[3]);
               }

               /* If items[1] or items[2] are not equal keep searching */
               else
                  continue;

               /* Used to loop to the redundant instruction */
               nline = cline->next;

               /* Make sure are bools are reset at the start */
               replace = FALSE;
               allocfail = FALSE;

               /** 
                * Loop through the instructions in between cline and the 
                * redundant instruction 
                */
               while(nline != tline)
               {
                  /** 
                   * If the variable we are using for the mov changes in 
                   * between cline and the redundant instruction then the 
                   * instruction is not redundant
                   */
                  if(varcommon(cline->uses, nline->sets))
                  {
                     /* Make sure we move to the next cline */
                     allocfail = TRUE;
                     break;
                  }

                  /* Check to see if the mov value dies before we get to it */
                  else if(varcommon(cline->sets, nline->deads))
                  {
                     reg = allocstring(tempreg);

                     /* Allocate a reg if possible */
                     if(allocreg(insttypes[cline->instinfonum].datatype,
                                 cline->sets, reg))
                     {
                        /*Replace the use value that died with the new value */
                        replaceuse(nline, cdst, reg);

                        /** 
                         * Used to make sure we process lines between cline 
                         * and where the value died 
                         */
                        replace = TRUE;

                        break;
                     }
                     else
                     {
                        /*If we fail to allocate a register go to next cline */
                        allocfail = TRUE;
                        break;
                     }

                  }
                  nline = nline->next; 
               }

               /* If allocreg failed or if value changed go to next cline */
               if(allocfail)
                  break;

               if(replace)
               {
                  /* Used to loop through line */
                  rline = cline->next;

                  /* Deal with lines where new reg need to be inserted */
                  while(nline != rline)
                  {
                     /* Replace each use of the math dst with new reg */
                     if(varcommon(cline->sets, rline->uses))
                        replaceuse(rline, cdst, reg);

                     rline = rline->next;
                  }

                  /* Replace the math instructions dst with the new reg */
                  replacedst(cline, reg);

                  /* Update cdst for createmove below */
                  cdst = allocstring(reg);
               }

               /* Change the redundant instruction into a mov */
               createmove(insttypes[tline->instinfonum].datatype, cdst, tdst, 
                          tline); 

               *changes = TRUE;
               incropt(LOCAL_CSE);
            }
         }
      }
   }
}

/*
 * localcse - perform local common subexpression elimination
 */
void localcse(int *changes)
{
   struct bblk *cblk;
   extern struct bblk *top;

   for (cblk = top; cblk; cblk = cblk->down)
      cseinblk(cblk, changes);
}

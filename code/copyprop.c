/**
 * @name          : Camill Folsom
 * @proj 6 Part 1 : Local Copy Propagation 
 */

#include <stdio.h>
#include "opt.h"
#include "misc.h"
#include "vars.h"

#include <stdlib.h>
#include <string.h>
#include "flow.h"
/*
 * localcopyprop - perform copy propagation
 */
void localcopyprop(int *changes)
{
   struct bblk *cblk;
   extern struct bblk *top;
   struct assemline *cline, *cline2;
   char *inbrak1, *inbrak2, *reg1, *reg2, *brakreg2, *brakreg1;
   int regsize = 3 * MAXREGCHAR;
   char reg[regsize], tempreg[regsize]; 

   /* Loop through the blocks */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      /* Loop through each line in the block looking for a move inst */
      for(cline = cblk->lines; cline; cline = cline->next)
      {
         /* Keep track of of whether we propagated a register or not */
         *changes = FALSE;

         /** 
          * Once we find a move inst we make sure it is a register to register 
          * move and that it is not the last inst in the block
          */
         if(cline->type == MOV_INST && cline->items[1][0] == cline->items[2][0]
            && cline != cblk->lineend)
         {
            /* Loop through the line in the block after the reg to reg move */
            for(cline2 = cline->next; cline2; cline2 = cline2->next)
            {
               /* Make sure the register is being used by the inst */
               if(regexists(cline->items[2], cline2->uses))
               {
                  /* For loads and store we add brackets to cline2's items */
                  if(cline2->type == LOAD_INST || cline2->type == STORE_INST)
                  {
                     /* Add brackets to the second item of cline */
                     sprintf(reg, "[%s]", cline->items[2]);
                     inbrak2 = allocstring(reg);

                     /* Add brackets to the first item of cline */
                     sprintf(reg, "[%s]", cline->items[1]);
                     inbrak1 = allocstring(reg);
                  }

                  /** 
                   * Check the first item of cline2 is equal to the reg we want
                   * to replace
                   */
                  if(cline2->items[1] && 
                     strcmp(cline2->items[1], cline->items[2]) == 0)
                  {
                     /* Swap the reg we are moving with first item of cline2 */
                     replaceuse(cline2, cline2->items[1], cline->items[1]); 
                     *changes = TRUE;
                  }

                  /* Load instruction that does not add in brackets */
                  else if(cline2->type == LOAD_INST && cline2->items[1][0] == '[' &&
                          strlen(cline2->items[1]) == MAXREGCHAR &&
                          strcmp(cline2->items[1], inbrak2) == 0)
                  {
                     /* Swap the first item of cline2 with register we made */
                     replaceuse(cline2, cline2->items[1], inbrak1);
                     *changes = TRUE;
                  }

                  /* Store instruction that does not add in brackets */
                  else if(cline2->type == STORE_INST && cline2->items[2][0] == '[' &&
                          strlen(cline2->items[2]) == MAXREGCHAR && 
                          strcmp(cline2->items[2], inbrak2) == 0)
                  {
                     replaceuse(cline2, cline2->items[2], inbrak1);
                     *changes = TRUE;
                  }

                  /* Deal with loads that have a + in the brackets */
                  else if(cline2->type == LOAD_INST && cline2->items[1][0] == '[' &&
                          strlen(cline2->items[1]) > MAXREGCHAR)
                  {
                    /* Copy cline2->items[1] into tempreg so we can strtok it */
                     sprintf(tempreg, "%s", cline2->items[1]);

                     /* Break tempreg into two strings without the + */
                     reg1 = strtok(tempreg, " + ");
                     reg2 = strtok(NULL, " + ");

                     /* Enccloses the reg in brakets */
                     sprintf(reg, "[%s", reg2);
                     brakreg2 = allocstring(reg);

                     /* Encloses reg in brakets */
                     sprintf(reg, "%s]", reg1);
                     brakreg1 = allocstring(reg);

                     /** 
                      * Remake cline->items[1] because the spaces were being
                      * removed.
                      */
                     sprintf(reg, "%s + %s", reg1, reg2);
                     free(cline2->items[1]);
                     cline2->items[1] = allocstring(reg);

                     /* When the first reg in the brakets should be swapped */
                     if(strcmp(inbrak2, brakreg1) == 0)
                     {
                        /* Remake inbrak1 so it only has 1 braket */
                        sprintf(reg, "[%s", cline->items[1]);
                        inbrak1 = allocstring(reg);

                        /* Make the string we want to replace items[1] with */
                        sprintf(reg, "%s + %s", inbrak1, reg2);
                        inbrak1 = allocstring(reg);

                        /* Swap items[1] with the ne string made above */
                        replaceuse(cline2, cline2->items[1], inbrak1);
                        *changes = TRUE;
                     }

                     /* When the second reg in the brakets should be swapped */
                     if(strcmp(inbrak2, brakreg2) == 0)
                     {
                        /* Remake inbrak1 so it only has 1 braket */
                        sprintf(reg, "%s]", cline->items[1]);
                        inbrak1 = allocstring(reg);

                        /* Makes the string we want to replace items[1] with */
                        sprintf(reg, "%s + %s", reg1, inbrak1);
                        inbrak1 = allocstring(reg);

                        /* Swap items[1] with the ne string made above */
                        replaceuse(cline2, cline2->items[1], inbrak1);
                        *changes = TRUE;
                     }
                  }

                  /* Deal with stores that have a + in the brackets */
                  else if(cline2->type == STORE_INST && cline2->items[2][0] == '[' &&
                          strlen(cline2->items[2]) > MAXREGCHAR)
                  {
                     /* Copy cline2->items[2] into tempreg so we can strtok it */
                     sprintf(tempreg, "%s", cline2->items[2]);
                     reg1 = strtok(tempreg, " + ");
                     reg2 = strtok(NULL, " + ");

                     /* Enccloses reg in brakets */
                     sprintf(reg, "[%s", reg2);
                     brakreg2 = allocstring(reg);

                     /* Encclose the reg in brakets */
                     sprintf(reg, "%s]", reg1);
                     brakreg1 = allocstring(reg);

                     /* Remake cline2->items[2] */
                     sprintf(reg, "%s + %s", reg1, reg2);
                     free(cline2->items[2]);
                     cline2->items[2] = allocstring(reg);

                     /* When the first reg in the brakets should be swapped */
                     if(strcmp(inbrak2, brakreg1) == 0)
                     {
                        /* Remake inbrak1 so it only has 1 braket */
                        sprintf(reg, "[%s", cline->items[1]);
                        inbrak1 = allocstring(reg);

                        /* Make the string we want to replace items[2] with */ 
                        sprintf(reg, "%s + %s", inbrak1, reg2);
                        inbrak1 = allocstring(reg);

                        /* Swap items[2] with the ne string made above */
                        replaceuse(cline2, cline2->items[2], inbrak1);
                        *changes = TRUE;
                     }

                     /* When the second reg in the brakets should be swapped */
                     if(strcmp(inbrak2, brakreg2) == 0)
                     {
                        /* Remake inbrak1 so it only has 1 braket */
                        sprintf(reg, "%s]", cline->items[1]);
                        inbrak1 = allocstring(reg);

                        /* Make the string we want to replace items[2] with */
                        sprintf(reg, "%s + %s", reg1, inbrak1);
                        inbrak1 = allocstring(reg);

                        /* Swap items[1] with the ne string made above */
                        replaceuse(cline2, cline2->items[2], inbrak1);
                        *changes = TRUE;
                     }
                  }

                  /* Deals with the second item for ARITH and CMP insts */
                  else if((cline2->type == ARITH_INST || 
                           cline2->type == CMP_INST) && cline2->items[2] &&
                           strcmp(cline2->items[2], cline->items[2]) == 0)
                  {
                     replaceuse(cline2, cline2->items[2], cline->items[1]);
                     *changes = TRUE;
                  }
               }

               /* Break out of cline2 loop if either item in cline is set */
               if(regexists(cline->items[2], cline2->sets) || 
                  regexists(cline->items[1], cline2->sets))
                  break;

            }
         }
         if(*changes)
            incropt(COPY_PROPAGATION);
      }
   }
   check_cf();
}

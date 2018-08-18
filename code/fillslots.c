/**
 * @name : Camill Folsom
 * @proj : Filling Delay Slots
 */
#include <stdio.h>
#include "opt.h"
#include "misc.h"
#include "opts.h"

#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "vars.h"

/*
 * filldelayslots - fill the delay slots of the transfers of control in a
 *                  function
 */
void filldelayslots()
{
   extern struct bblk *top;
   struct bblk *cblk;
   struct assemline *cline, *prev;
   char buffer[MAXLINE];
   int cmp = FALSE;
   varstate used, set;

   /* Loop through each block */
   for(cblk = top; cblk; cblk = cblk->down)
   {
       /**
        * Checks whether the last instruction in the block is a branch, jump,
        * or call. It also checks that there is an instruction before lineend 
        */
      if(cblk->lineend && (cblk->lineend->type == BRANCH_INST || 
         cblk->lineend->type == CALL_INST || cblk->lineend->type == JUMP_INST) 
         && cblk->lineend->prev)
      {
         /* Resets values used below */
         cmp = FALSE;
         varinit(used);
         varinit(set);

         /* Loop through each line in the block starting at the last line */
         for(cline = cblk->lineend->prev; cline; cline = cline->prev)
         {
             /* Get the next prev line if save, comment, or define line */
            if(cline->type == COMMENT_LINE || cline->type == DEFINE_LINE ||
               cline->type == SAVE_INST)
               continue;

            /* Procces jumps, uncondition branches and calls */
            if(cline->type != CMP_INST && strcmp(cline->items[0],"andcc") != 0
               && !cmp && strcmp(cline->items[0],"subcc") != 0)
            {
               /* Chnage ba,a to ba */
               if(strcmp(cblk->lineend->items[0], "ba,a") == 0)
               {
                  /* Remake the ba,a as a ba instruction */
                  sprintf(buffer, "\tba\t%s", cblk->lineend->items[1]);

                  /* Update the lineend instruction to the new ba inst */
                  free(cblk->lineend->text);
                  cblk->lineend->text = allocstring(buffer);
                  setupinstinfo(cblk->lineend);
               }

               /* Unhook the line before lineend and place it after it */
               prev = cline;
               unhookline(prev);
               hookupline(cblk, cblk->lineend->next, prev);

               incropt(FILL_DELAY_SLOTS);
               break;
            }

            /* Deals with conditional branches */
            else
            {
               /* Check that an inst exists above a cmp or andcc inst */
               if(!cline->prev)
                  break;

               /** 
                * Sets cmp so that when another cline is processed the if part
                * of this if else statement is not executed 
                */ 
               cmp = TRUE;               

               /* If define, comment, or save inst grab next prev */
               if(cline->prev->type == COMMENT_LINE || 
                  cline->prev->type == DEFINE_LINE || 
                  cline->prev->type == SAVE_INST)
               {
                  /* Make sure that the next prev cline exists */
                  if(cline->prev->prev)
                     prev = cline->prev->prev;
                  else
                     break;
               }

               /* Grab the next prev if not comment, define, or save inst */
               else
                  prev = cline->prev;

               /* Track all the sets and uses of the insts as we mov up */
               unionvar(used, used, cline->uses);
               unionvar(set, set, cline->sets);

               /** 
                * Make sure the uses have not been set in following 
                * instructions and that the sets have not been used in 
                * following instructions 
                */
               if(!varcommon(used, prev->sets) && 
                  !varcommon(set, prev->uses)) 
               {
                  /* Unhook the line and hook it up after the lineend line */
                  unhookline(prev);
                  hookupline(cblk, cblk->lineend->next, prev);

                  incropt(FILL_DELAY_SLOTS);
                  break;
               }
            }
         } 
      }
   }
}

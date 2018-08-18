/**
 * @name          : Camill Folsom
 * @proj 6 part 2 : Local Constant Propagation
 */
#include <stdio.h>
#include "opt.h"
#include "io.h"
#include "misc.h"
#include "vars.h"

#include <string.h>
#include "flow.h"
/*
 * localconstprop - perform constant propagation
 */
void localconstprop(int *changes)
{
   struct bblk *cblk;
   extern struct bblk *top;
   struct assemline *cline, *cline2;
   extern struct instinfo insttypes[];

   /* Loop through the blocks */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      /* Loop through each line in the block looking for a move inst */
      for(cline = cblk->lines; cline; cline = cline->next)
      {
         /* Keep track of of whether we propagated a register or not */
         *changes = FALSE;

         /**
          * Once we find a move inst we make sure it is a const to register
          * move and that it is not the last inst in the block
          */
         if(cline->type == MOV_INST && cline != cblk->lineend && 
            isconst(cline->items[1]))
         {
            /* Loop through the line in the block after the reg to reg move */
            for(cline2 = cline->next; cline2; cline2 = cline2->next)
            {
               /**
                * Make sure the register is being used by the inst, cline2
                * check that the last src can be a constant, and make sure
                * that we do not make two src regs constants
                */ 
               if(regexists(cline->items[2], cline2->uses) && 
                  insttypes[cline2->instinfonum].lastsrccanbeconst &&
                  !isconst(cline2->items[1]) && !isconst(cline2->items[2]))
               {
                  /**
                   * Check the first item of cline2 is equal to the reg we want
                   * to replace and the inst is not a move
                   */ 
                  if(cline2->items[1] &&
                     strcmp(cline2->items[1], cline->items[2]) == 0) 
                  {
                     /* Swap the reg we are moving with first item of cline2 */
                     replaceuse(cline2, cline2->items[1], cline->items[1]);
                     *changes = TRUE;
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
 
               /* Break out of cline2 loop if cline's reg is being set */
               if(regexists(cline->items[2], cline2->sets))
                  break;
            }
         }
         if(*changes)
            incropt(CONSTANT_PROPAGATION);
      }
   }
   check_cf();
}

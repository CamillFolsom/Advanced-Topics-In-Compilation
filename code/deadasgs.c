/**
 * @name : Camill Folsom
 * @proj : Dead Assignment Elimination
 */
#include <stdio.h>
#include "opt.h"
#include "vars.h"
#include "analysis.h"
#include "opts.h"

#include "misc.h"
#include "flow.h"

/*
 * deadasgelim - perform dead assignment elimination
 */
void deadasgelim()
{
   struct bblk *cblk;
   extern struct bblk *top;
   struct assemline *cline, *cline2, *nline;
   varstate interuses, intersets, interouts;
   /* Register that are never useless: %g0, %sp, %fp */
   int nodel = 0x40004001;

   varinit(interuses);
   varinit(intersets);
   varinit(interouts);

   /* Loop through each block */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      /* Loop through each line looking for one to delete */
      for(cline = cblk->lines; cline; cline = nline)
      {
         /* Make a copy of cline->next so we can delete cline in the loop */
         nline = cline->next;

         /* The instruction we want to ignore deleting */
         if(cline->type != SAVE_INST && cline->type != RESTORE_INST && 
            cline->type != BRANCH_INST && cline->type != JUMP_INST && 
            cline->type != CALL_INST && !varempty(cline->sets) && 
            !(cline->sets[0] & nodel)) 
         {
            /* Loop throgh the line under the line we may want to delete */
            for(cline2 = cline->next; cline2; cline2 = cline2->next)
            {
               /** 
                * Check first to see if the instruction is used, if it is do 
                * not delete cline 
                */
               intervar(interuses, cline->sets, cline2->uses);
               if(!varempty(interuses))
                  break;

               /**
                * Since we know this instruction has not been used yet, check 
                * to see if it is being set. If it is cline is useless.
                */ 
               intervar(intersets, cline->sets, cline2->sets);
               if(!varempty(intersets))
               {
                  delline(cline);
                  incropt(DEAD_ASG_ELIM);
                  break;
               }

               /**
                * If cline2 makes it to lineend then we want to make sure
                * cline is not going out of the block, if it is not it is
                * useless and can be deleted
                */
               if(cline2 == cblk->lineend)
               {
                  intervar(interouts, cline->sets, cblk->outs);
                  if(varempty(interouts))
                  {
                    delline(cline);
                    incropt(DEAD_ASG_ELIM);
                  }
                  break;
               }
            }
          
            /* Check that lineend is not a useless inst */
            if(cline == cblk->lineend)
            {
               intervar(interouts, cline->sets, cblk->outs);
               if(varempty(interouts))
               {
                 delline(cline);
                 incropt(DEAD_ASG_ELIM);
               }
            }

           /* Delete mov instruction that mov a register to itself */
           if(cline->type == MOV_INST && varcmp(cline->sets, cline->uses))
           {
              delline(cline);
              incropt(DEAD_ASG_ELIM);
           } 
         }
      }
   }
   check_cf();
}

/**
 * @name   : Camill Folsom
 * @proj 1 : Branch Chaining
 */
#include <stdlib.h>
#include <stdio.h>
#include "opt.h"
#include "misc.h"
#include "opts.h"
#include "flow.h"

/*
 * remvbranchchains - remove branch chains
 */
void remvbranchchains()
{
   struct bblk *cblk, *tblk;
   extern struct bblk *top;
   struct blist *bptr, *bptr2;

   /* Loop through all the blocks */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      if(cblk->lines)
      {
         /* Check to see if the block is only a branch instruction */
         if(cblk->lines->type == JUMP_INST)
         {
            /* Loop through the predecessors */
            for(bptr = cblk->preds; bptr; bptr = bptr->next)
            {
               /* Predecessor blocks that are not the previous block */
               if(bptr->ptr->num != (cblk->num-1))
               {
                  assignlabel(cblk, "!");

                  /* Replaces the jump or branch instruction's target */
                  replaceuse(bptr->ptr->lineend, bptr->ptr->lineend->items[1], 
                             cblk->lines->items[1]);

                  /**
                   * Add cblk's successor to cblk's predecessors succsesors
                   * list.
                   */
                  addtoblist(&bptr->ptr->succs, cblk->succs->ptr);

                  /* Delete cblk from cblk's predecessors successors list */
                  delfromblist(&bptr->ptr->succs, cblk);

                  /* Add to cblk's succesors predecessors list */
                  addtoblist(&cblk->succs->ptr->preds, bptr->ptr);

                  /** 
                   * Deletes the blocks in cblk's predecessors list that
                   * are not its fallthrough block.
                   */
                  delfromblist(&cblk->preds, bptr->ptr);

                 /** 
                  * This loop makes sure the fallthrough block is the first 
                  * block in the successors list for cblk's predecessors
                  * successors list.
                  */
                 for(bptr2 = bptr->ptr->succs; bptr2; bptr2 = bptr2->next)
                 {
                    if(bptr2->ptr->num == bptr->ptr->num+1)
                    {
                       tblk = bptr2->ptr;
                       bptr2->ptr = bptr->ptr->succs->ptr;
                       bptr->ptr->succs->ptr = tblk;
                    }
                 }

                 incropt(BRANCH_CHAINING);
               }
            }
         }
      }
   }
   check_cf();
}

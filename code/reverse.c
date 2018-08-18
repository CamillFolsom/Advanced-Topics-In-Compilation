/**
 * @name  : Camill Folsom
 * @proj2 : Eliminating Unconditional Jumps by Reversing Branches
 */
#include <stdio.h>
#include "opt.h"
#include "misc.h"
#include "opts.h"

#include <stdlib.h>
#include <string.h>
#include "flow.h"
#include "io.h"

/*
 * reversebranches - avoid jumps by reversing branches
 * note            - I put a number next to the start and end of each bracket
 *                   that goes off screen to help identify it.
 */
void reversebranches()
{
   struct bblk *cblk, *tblk, *nblk;
   extern struct bblk *top;
   struct blist *bptr, *bptr2;
   char line[MAXLINE];

   /* Loop through all the blocks */
   for(cblk = top; cblk; cblk = nblk)
   { /* 1 */

      nblk = cblk->down;      

      /* Check to see if the block is only a JUMP instruction */
      if((cblk->lines) && (cblk->lines->type == JUMP_INST))
      {/* 2 */

         /* Loop through the predecessors */
         for(bptr = cblk->preds; bptr; bptr = bptr->next)
         { /* 3 */

            /** 
             * Makes sure this predecessor block is cblk's previous block and
             * that the last instruction in the previous block is a branch
             * instruction. In the following comments I refer to this block
             * as the branch block.
             */
            if((bptr->ptr->num == (cblk->num-1)) && 
               (bptr->ptr->lineend->type == BRANCH_INST))
            { /* 4 */

               /** 
                * Find the block that the brach jumps to and makes sure that 
                * the block it jumps to is after its fallthrough block.
                */
               if((tblk = findblk(bptr->ptr->lineend->items[1])) && 
                   (tblk->num == (cblk->num+1)))
               {/* 5 */ 

                  /** 
                   * Adds cblk's successor to the successors list of its
                   * immidiate predeseccor 
                   */
                  addtoblist(&bptr->ptr->succs, cblk->succs->ptr);

                  /** 
                   * Adds the bracnhing block to cblk's successors predecessors 
                   * list 
                   */
                  addtoblist(&cblk->succs->ptr->preds, bptr->ptr);

                  /**
                   * Makes cblk's successor the first successor in the branch
                   * blocks successors list.
                   */
                  for(bptr2 = bptr->ptr->succs; bptr2; bptr2 = bptr2->next)
                  {
                     if(bptr2->ptr->num == bptr->ptr->num+2)
                     {
                        tblk = bptr2->ptr;
                        bptr2->ptr = bptr->ptr->succs->ptr;
                        bptr->ptr->succs->ptr = tblk;
                     }
                  }

                  /** 
                   * The following if statements makes the text for the
                   * assemline instruction's text. It reverses the branch
                   * conditions for each branch statement.
                   */

                  /* be and bne */
                  if(strcmp(bptr->ptr->lineend->items[0], "be") == 0)
                    sprintf(line, "\t%s\t%s", "bne", cblk->lineend->items[1]); 
                  
                  if(strcmp(bptr->ptr->lineend->items[0], "bne") == 0)
                    sprintf(line, "\t%s\t%s", "be", cblk->lineend->items[1]); 

                  /* bg and ble */
                  if(strcmp(bptr->ptr->lineend->items[0], "bg") == 0)
                    sprintf(line, "\t%s\t%s", "ble", cblk->lineend->items[1]);
                  
                  if(strcmp(bptr->ptr->lineend->items[0], "ble") == 0)
                    sprintf(line, "\t%s\t%s", "bg", cblk->lineend->items[1]);

                  /* bge and bl */
                  if(strcmp(bptr->ptr->lineend->items[0], "bge") == 0)
                     sprintf(line, "\t%s\t%s", "bl", cblk->lineend->items[1]);

                  if(strcmp(bptr->ptr->lineend->items[0], "bl") == 0)
                    sprintf(line, "\t%s\t%s", "bge", cblk->lineend->items[1]);

                  /* Unsigned Branch Statements */

                  /* bgu and bleu */
                  if(strcmp(bptr->ptr->lineend->items[0], "bgu") == 0)
                    sprintf(line, "\t%s\t%s", "bleu", cblk->lineend->items[1]);

                  if(strcmp(bptr->ptr->lineend->items[0], "bleu") == 0)
                    sprintf(line, "\t%s\t%s", "bgu", cblk->lineend->items[1]);

                  /* bgeu and blu */
                  if(strcmp(bptr->ptr->lineend->items[0], "bgeu") == 0)
                    sprintf(line, "\t%s\t%s", "blu", cblk->lineend->items[1]);

                  if(strcmp(bptr->ptr->lineend->items[0], "blu") == 0)
                    sprintf(line, "\t%s\t%s", "bgeu", cblk->lineend->items[1]);

                  /* Floating Branch Statements */

                  /* fbe and fbne */
                  if(strcmp(bptr->ptr->lineend->items[0], "fbe") == 0)
                    sprintf(line, "\t%s\t%s", "fbne", cblk->lineend->items[1]);

                  if(strcmp(bptr->ptr->lineend->items[0], "fbne") == 0)
                    sprintf(line, "\t%s\t%s", "fbe", cblk->lineend->items[1]);

                  /* fbg and fble */
                  if(strcmp(bptr->ptr->lineend->items[0], "fbg") == 0)
                    sprintf(line, "\t%s\t%s", "fble", cblk->lineend->items[1]);

                  if(strcmp(bptr->ptr->lineend->items[0], "fble") == 0)
                    sprintf(line, "\t%s\t%s", "fbg", cblk->lineend->items[1]);

                  /* fbge and fbl */
                  if(strcmp(bptr->ptr->lineend->items[0], "fbge") == 0)
                     sprintf(line, "\t%s\t%s", "fbl", cblk->lineend->items[1]);

                  if(strcmp(bptr->ptr->lineend->items[0], "fbl") == 0)
                    sprintf(line, "\t%s\t%s", "fbge", cblk->lineend->items[1]);

                  free(bptr->ptr->lineend->text);
                  bptr->ptr->lineend->text = allocstring(line);
                  setupinstinfo(bptr->ptr->lineend);
                  
                  deleteblk(cblk);

                  incropt(REVERSE_BRANCHES);
               }/* 5 */
            } /* 4 */
         } /* 3 */
      } /* 2 */
   } /* 1 */

   check_cf();
}

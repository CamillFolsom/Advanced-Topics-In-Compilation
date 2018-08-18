/**
 * @name : Camill Folsom
 * @proj : Live and Dead Variable Analysis
 */
#include <stdio.h>
#include "opt.h"
#include "misc.h"
#include "vars.h"
#include "analysis.h"
#include "opts.h"

#include <string.h>
#include <ctype.h>

/*
 * makevar - unions v1 and v1 and deletes %g0, %sp, and %fp from v1
 */
void makevar(varstate v1, varstate v2)
{
   int g0pos = 0;
   int sppos = 14;
   int fppos = 30;

   unionvar(v1, v1, v2);

   /* Delete %g0 if its is in v1 */
   if(v1[0] & (1 << g0pos))
      delreg("%g0", v1, 1);

   /* Delete %sp */
   if(v1[0] & (1 << sppos))
      delreg("%sp", v1, 1);

   /* Delete %fp */
   if(v1[0] & (1 << fppos))
      delreg("%fp", v1, 1);
}

/*
 * calclivevars - calculate live variable information
 */
void calclivevars()
{
   struct bblk *cblk;
   struct blist *bptr;
   extern struct bblk *top;
   struct assemline *cline;
   varstate oldins, minus, toadd;

   /* Used as a bool where 0 = false and 1 = true */
   int change;

   /* Loop through each block starting from the top */
   for(cblk = top; cblk; cblk = cblk->down)
   {
      if(cblk->lines)
      {
         /* Loop through each line in the block starting at the first line */
         for(cline = cblk->lines; cline; cline = cline->next)
         {
            /* Ignore branch and jump instuctions */
            if(cline->type != BRANCH_INST && cline->type != JUMP_INST)
            {
               /** 
                * Add to defs if sets exits and it is not already in defs and 
                * if it is not in the block's uses 
                */
               if(!varempty(cline->sets) && !varcmp(cblk->defs, cline->sets)
                  && !varcmp(cblk->uses, cline->sets))
               {
                  /** 
                   * Makes sure minus is empty in case it is not set in the if
                   * statement below 
                   */
                  if(!varempty(minus))
                     varinit(minus);
           
                  /** 
                   * If sets and uses have something in common then we put the
                   * intersection in minus 
                   */
                  if(varcommon(cline->sets, cline->uses))
                     intervar(minus, cline->sets, cline->uses);

                  varcopy(toadd, cline->sets);

                  /** 
                   * Subtract what sets and uses have in common from what is to
                   * be added to defs, this deals with the case where the 
                   * register or variavle is is used and set on the same line 
                   */
                  minusvar(toadd, toadd, minus);

                  /* If toadd is not empty add it to defs */
                  if(!varempty(toadd))
                  {
                     /* Adds toadd to defs */
                     makevar(cblk->defs, toadd);
                     intervar(minus, cblk->defs, cblk->uses);

                     /* Subtract anything that is already in uses from defs */
                     minusvar(cblk->defs, cblk->defs, minus);
                  }
                     
               }

               /** 
                * If cline's uses is not empty and it is not in uses or defs
                * already
                */
               if(!varempty(cline->uses) && !varcmp(cblk->uses, cline->uses)
                  && !varcmp(cblk->defs, cline->uses))
               {
                  /* Add cline's uses to the block's uses */
                  makevar(cblk->uses, cline->uses);
                  intervar(minus, cblk->defs, cblk->uses);
                 
                  /* Subtract anything already in defs from uses */
                  minusvar(cblk->uses, cblk->uses, minus);
               }

            }
         }
      } 
   }

   do {
      /* Sets changed to false */
      change = 0;
      
      /* Loop through each block starting at the top */
      for(cblk = top; cblk; cblk = cblk->down)
      {
         /* Makes outs empty */
         varinit(cblk->outs); 

         /** 
          * Loop through each sucessor block and add the union of outs and the
          * sucessors ins to the block's outs 
          */
         for(bptr = cblk->succs; bptr; bptr = bptr->next)
            unionvar(cblk->outs, cblk->outs, bptr->ptr->ins);

         /* Copy the block's ins to olding */
         varcopy(oldins, cblk->ins);

         /* Subtract defs from outs */
         minusvar(minus, cblk->outs, cblk->defs);

         unionvar(cblk->ins, cblk->uses, minus);

         /* If ins changed the we set chanfed to true */
         if(!varcmp(cblk->ins,oldins))
            change = 1;
      }

   } while (change == 1);
}

/*
 * calcdeadvars - calculate dead variable information
 */
void calcdeadvars()
{
   struct bblk *cblk;
   extern struct bblk *top;
   struct assemline *cline;
   varstate toadd, minus, lastuse, lastset;

   /* Loop through all the blocks starting from the top */
   for(cblk = top ; cblk; cblk = cblk->down)
   {
      /* Make sure lastuse, lastset, and toadd are empty */
      varinit(lastuse);
      varinit(lastset);
      varinit(toadd);

      /* Loop through each block starting from the bottom */
      for(cline = cblk->lineend; cline; cline = cline->prev)
      {
         /* Ignore branch and jump instructions */
         if(cline->type != BRANCH_INST && cline->type != JUMP_INST)
         {
           /* Make sure uses if not empty */
           if(!varempty(cline->uses))
           {
               /* Copy cline's uses to toadd */
               varcopy(toadd, cline->uses);

               /* Makes sure we dont add something that is in outs */
               intervar(minus, cline->uses, cblk->outs);
               minusvar(toadd, toadd, minus);

               /** 
                * Makes sure we dont add something whos use died in a following
                * instruction 
                */
               minusvar(toadd, toadd, lastuse);

               makevar(cline->deads, toadd);

               /** 
                * Checks if something in uses was set in a following 
                * instruction 
                */
               if(varcommon(lastset, cline->uses))
               {
                  /* Find the set that matches the lines uses */
                  intervar(toadd, lastset, cline->uses);
                  makevar(cline->deads, toadd);

                  /** 
                   * Makes sure what we added above is not added again, on the
                   * next iteration of the loop, unless it is set again
                   */
                  minusvar(lastset, lastset, toadd);
               }

               /* Makes a list of the sets and uses for the lines */
               unionvar(lastset, lastset, cline->sets);
               unionvar(lastuse, lastuse, cline->uses);

              /* If sets and uses have something in common */
              if(varcommon(cline->sets, cline->uses))
              {
                 /* Get the value or values in sets and uses they share */
                 intervar(toadd, cline->sets, cline->uses);
                 makevar(cline->deads, toadd);
                 
                 /* Subtract anything we added from lastset */
                 minusvar(lastset, lastset, toadd);
              }
            }
            /* If uses is empty and sets isnt then add it to lastset */
            else if(!varempty(cline->sets))
               unionvar(lastset, lastset, cline->sets);
         }
      }   
   }
}

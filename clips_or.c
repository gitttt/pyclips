/* clips_or.c
 * Source of functions from CLIPS explicitly reimplemented for PyCLIPS:
 * depending on how CLIPS changes in time, some functions may appear or
 * disappear (or be conditionally compiled) from here. Functions defined
 * here have the same name as in the CLIPS API followed by _PY, and their
 * arguments may be different from the official CLIPS ones. However these
 * functions were actually copied from the original CLIPS sources, and
 * only slightly changed: therefore I list myself only as a contributing
 * programmer. Moreover, the license for this file is the one that covers
 * the CLIPS source (which is BSD-like).
 * $Id: clips_or.c 323 2007-04-02 22:24:35Z Franz $
 */

/* NOTE: This file used to be split in three parts, one per function */


/*************************************************************/
/* Principal Programmer(s): Gary D. Riley                    */
/*                          Brian L. Donnell                 */
/*                                                           */
/* Contributing Programmer(s): Francesco Garosi              */
/*                                                           */
/*************************************************************/


#define _RULECOM_SOURCE_
#define _CONSTRCT_SOURCE_
#define _INSCOM_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"


/* many of these might not be needed but they were in the original sources */

#include "argacces.h"
#include "commline.h"
#include "classcom.h"
#include "classfun.h"
#include "classinf.h"
#include "constant.h"
#include "constrct.h"
#include "crstrtgy.h"
#include "engine.h"
#include "envrnmnt.h"
#include "evaluatn.h"
#include "extnfunc.h"
#include "exprnpsr.h"
#include "insfile.h"
#include "insfun.h"
#include "insmngr.h"
#include "insmoddp.h"
#include "insmult.h"
#include "inspsr.h"
#include "lgcldpnd.h"
#include "memalloc.h"
#include "moduldef.h"
#include "msgcom.h"
#include "msgfun.h"
#include "multifld.h"
#include "pattern.h"
#include "prcdrfun.h"
#include "prcdrpsr.h"
#include "reteutil.h"
#include "router.h"
#include "ruledlt.h"
#include "scanner.h"
#include "sysdep.h"
#include "strngrtr.h"
#include "utility.h"
#include "watch.h"

#if BLOAD || BLOAD_AND_BSAVE || BLOAD_ONLY
#include "rulebin.h"
#endif

#if LOGICAL_DEPENDENCIES
#include "lgcldpnd.h"
#endif

#if INCREMENTAL_RESET
#include "incrrset.h"
#endif

#include "rulecom.h"
#include "constrct.h"
#include "inscom.h"


#include "clips_or.h"


/* EnvMatches_PY
 * provides a customized version of the EnvMatches API, that can redirect
 * its output to a specified output stream. This will be used instead of the
 * Matches() standard API.
 */

#if DEFRULE_CONSTRUCT

/********************************/
/* EnvMatches: C access routine */
/*   for the matches command.   */
/********************************/

globle BOOLEAN EnvMatches_PY(
  void *theEnv,
  char *logicalName,
  void *theRule)
  {
   DATA_OBJECT result;
   EnvMatches(theEnv, theRule, VERBOSE, &result);
   return(TRUE);
  }

#endif /* DEFRULE_CONSTRUCT */



/* EnvClear_PY
 * Overrides the EnvClear function that would normally conflict with PyCLIPS.
 */

/*****************************************************/
/* EnvClear: C access routine for the clear command. */
/*****************************************************/
globle BOOLEAN EnvClear_PY(
  void *theEnv)
  {
   struct callFunctionItem *theFunction;

   /*==========================================*/
   /* Activate the watch router which captures */
   /* trace output so that it is not displayed */
   /* during a clear.                          */
   /*==========================================*/

#if DEBUGGING_FUNCTIONS
   EnvActivateRouter(theEnv,WTRACE);
#endif

   /*===================================*/
   /* Determine if a clear is possible. */
   /*===================================*/

   ConstructData(theEnv)->ClearReadyInProgress = TRUE;
   if (ClearReady(theEnv) == FALSE)
     {
      PrintErrorID(theEnv,"CONSTRCT",1,FALSE);
      EnvPrintRouter(theEnv,WERROR,"Some constructs are still in use. Clear cannot continue.\n");
#if DEBUGGING_FUNCTIONS
      EnvDeactivateRouter(theEnv,WTRACE);
#endif
      ConstructData(theEnv)->ClearReadyInProgress = FALSE;
      return FALSE;
     }
   ConstructData(theEnv)->ClearReadyInProgress = FALSE;

   /*===========================*/
   /* Call all clear functions. */
   /*===========================*/

   ConstructData(theEnv)->ClearInProgress = TRUE;

   for (theFunction = ConstructData(theEnv)->ListOfClearFunctions;
        theFunction != NULL;
        theFunction = theFunction->next)
     {
      if (theFunction->environmentAware)
        { (*theFunction->func)(theEnv); }
      else
        { (* (void (*)(void)) theFunction->func)(); }
     }

   /*=============================*/
   /* Deactivate the watch router */
   /* for capturing output.       */
   /*=============================*/

#if DEBUGGING_FUNCTIONS
   EnvDeactivateRouter(theEnv,WTRACE);
#endif

   ConstructData(theEnv)->ClearInProgress = FALSE;
   return TRUE;
  }



/* EnvGetNextInstanceInClassAndSubclasses_PY
 * provides a customized version of the EnvGetNextInstanceInClassAndSubclasses
 * API, that does not "return" a pointer to the subclass. This will be used
 * instead of the EnvGetNextInstanceInClassAndSubclasses() standard API.
 */

#if OBJECT_SYSTEM

/***************************************************
  NAME         : EnvGetNextInstanceInClassAndSubclasses
  DESCRIPTION  : Finds next instance of class
                 (or first instance of class) and
                 all of its subclasses
  INPUTS       : 1) Class address (DIRECT POINTER!)
                 2) Instance address
                    (NULL to get first instance)
  RETURNS      : The next or first class instance
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
globle void *EnvGetNextInstanceInClassAndSubclasses_PY(
  void *theEnv,
  void *cptr,   /* this has changed */
  void *iptr,
  DATA_OBJECT *iterationInfo)
  {
   INSTANCE_TYPE *nextInstance;
   DEFCLASS *theClass;

   theClass = (DEFCLASS *)cptr;

   if (iptr == NULL)
     {
      ClassSubclassAddresses(theEnv,theClass,iterationInfo,TRUE);
      nextInstance = theClass->instanceList;
     }
   else if (((INSTANCE_TYPE *) iptr)->garbage == 1)
     { nextInstance = NULL; }
   else
     { nextInstance = ((INSTANCE_TYPE *) iptr)->nxtClass; }

   while ((nextInstance == NULL) &&
          (GetpDOBegin(iterationInfo) <= GetpDOEnd(iterationInfo)))
     {
      theClass = (struct defclass *) GetMFValue(DOPToPointer(iterationInfo),
                                                GetpDOBegin(iterationInfo));
      SetpDOBegin(iterationInfo,GetpDOBegin(iterationInfo) + 1);
      nextInstance = theClass->instanceList;
     }

   return(nextInstance);
  }

#endif /* OBJECT_SYSTEM */



/* end. */

//
// $Source$
// $Revision$
// $Date$
//
// DESCRIPTION:
// Interface to quantal-response correspondence tracing
//

#ifndef ALGQRE_H
#define ALGQRE_H

#include "game/efg.h"
#include "game/behavsol.h"
#include "game/nfg.h"
#include "game/mixedsol.h"

bool QreEfg(wxWindow *, const EFSupport &, gList<BehavSolution> &);
bool QreNfg(wxWindow *, const EFSupport &, gList<BehavSolution> &);
bool QreNfg(wxWindow *, const NFSupport &, gList<MixedSolution> &);

#endif  // ALGQRE_H

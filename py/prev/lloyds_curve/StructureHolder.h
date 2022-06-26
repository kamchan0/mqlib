/*****************************************************************************
    StructureHolder

	The StructureHolder class represents a base holder for all models
	that contain a structure curve

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureHolder_H__
#define __StructureHolder_H__


//	FlexYCF
#include "HolderClass.h"
#include "Structure.h"
#include "StructureCurve.h"


namespace FlexYCF
{
	DEFINE_HOLDER_CLASS( Structure		)
	DEFINE_HOLDER_CLASS( StructureCurve )
	DEFINE_HOLDER_CLASS( StructureSurface )
}


#endif	//	__StructureHolder_H__

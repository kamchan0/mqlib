/*****************************************************************************
    IStructureCurve

	StructureCurve is an interface for all structure curves

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __IStructureCurve_H__
#define __IStructureCurve_H__


namespace FlexYCF
{
	class IStructureCurve
	{
	public:
		virtual ~IStructureCurve() = 0 { }
	};
}
#endif	//	__IStructureCurve_H__
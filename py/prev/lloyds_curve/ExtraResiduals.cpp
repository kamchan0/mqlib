/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ExtraResiduals.h"
#include "ExtraResidual.h"
#include "BaseModel.h"
#include "FlexYCFCloneLookup.h"

using namespace LTQC;

namespace FlexYCF
{
    ExtraResiduals::ExtraResiduals(const BaseModelPtr baseModel):
        m_baseModel(baseModel)
    {
    }

    /**
        @brief Pseudo copy constructor that uses a lookup.

        Create a copy where the contained instances are clones using a lookup to preserved directed graph relationships.

        @param original The original instance to copy from.
        @param lookup   A lookup of previously cloned instances.
    */
    ExtraResiduals::ExtraResiduals(ExtraResiduals const& original, CloneLookup& lookup) :
        m_baseModel(lookup.get(original.m_baseModel))
    {
        CloneLookupUtils::assign(original.m_extraResiduals, m_extraResiduals, lookup);
    }

    size_t ExtraResiduals::getNumberOfExtraResiduals() const
    {
        return m_extraResiduals.size();
    }

    void  ExtraResiduals::addExtraResidual(const ExtraResidualPtr extraResidual)
    {
        m_extraResiduals.push_back(extraResidual);
    } 

    double ExtraResiduals::getResidualImpl(const size_t index) const
    {
        return m_extraResiduals[index]->getValue();
    }

    void ExtraResiduals::computeGradient(const size_t index, Gradient& gradient) const
    {
        m_extraResiduals[index]->computeGradient(gradient);   
    }

	void ExtraResiduals::computeGradient(const size_t index, 
										 Gradient& gradient, 
										 const CurveTypeConstPtr& /* curveType */) const
	{
		m_extraResiduals[index]->computeGradient(gradient);
		// Not supported at the moment:
		// m_extraResiduals[index]->computeGradient(gradient, curveType);
	}

	void ExtraResiduals::clear()
	{
		m_extraResiduals.clear();
	}

    void ExtraResiduals::checkIndex(const size_t index) const
    {
        if(index < 0 || index >= m_extraResiduals.size())
        {
            LT_THROW_ERROR( "Invalid index in ResidualCollection::getResidual." );
        }
    }

}
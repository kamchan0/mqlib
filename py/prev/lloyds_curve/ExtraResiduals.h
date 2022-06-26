/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRARESIDUALS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRARESIDUALS_H_INCLUDED
#pragma once
#include "ResidualCollection.h"
#include "Gradient.h"
#include "ExtraResidual.h"


namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( BaseModel )
    FWD_DECLARE_SMART_PTRS( ExtraResidual )

    class ExtraResiduals : public ResidualCollection
    {
    private:
        typedef std::vector<ExtraResidualPtr> ExtraResidualContainer;
    
    public:
        explicit ExtraResiduals(const BaseModelPtr baseModel);
        ExtraResiduals(ExtraResiduals const& original, CloneLookup& lookup);

        void addExtraResidual(const ExtraResidualPtr extraResidual);

        size_t getNumberOfExtraResiduals() const;

        size_t size() const
        {
            return m_extraResiduals.size();
        }

		/// Returns the index-th residual
		ExtraResidualPtr operator[](const size_t index) const
		{
			return m_extraResiduals[index];
		}

        virtual void computeGradient(const size_t index, Gradient& gradient) const;
		virtual void computeGradient(const size_t index, 
									 Gradient& gradient, 
									 const CurveTypeConstPtr& curveType) const;
		
		virtual void clear();

    protected:
        virtual double getResidualImpl(const size_t index) const;
    
    private:
        virtual void checkIndex(const size_t index) const;
        ExtraResiduals(ExtraResiduals const&); // deliberately disabled as won't clone properly

        BaseModelPtr m_baseModel;
        ExtraResidualContainer m_extraResiduals;

    };

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_EXTRARESIDUALS_H_INCLUDED

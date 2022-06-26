/*****************************************************************************
    StructureCurve

	The StructureCurve class represents the structure curve as product 
	of structure instrument curves.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureCurve_H__
#define __StructureCurve_H__


//	Standard
#include <functional>

//	LTQC
//	#include "Composite.h"
#include "lt/ptr.h"
#include "Macros.h"

//	FlexYCF
#include "Gradient.h"
#include "IStructureInstrumentCurve.h"
#include "StructureInstrument.h"
#include "InstrumentDelta.h"
#include "ReplicatingFlows.h"
#include "IndexRepFlow.h"
#include "FundingRepFlow.h"
#include "CurveType.h"

namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}
namespace FlexYCF
{

	class BaseModel;
	LTQC_FWD_DECLARE_SMART_PTRS( StructureCurve )


	// TODO:
	//	- get instruments?	
	class StructureCurve
	{
	public:
		//	Maybe StructureInstrumentCurveConstPtr in this case!
		typedef std::vector<IStructureInstrumentCurvePtr> StructureInstrumentCurves;


		StructureCurve() { }
		explicit StructureCurve(const LTQuant::GenericData& masterTable);
		explicit StructureCurve(const StructureInstrumentCurves& instrumentCurves);
		//	Returns the discount factor at the specified time
		inline double getDiscountFactor(const double time) const
		{
			return exp(-getLogFvf(time));
		}

		//	Returns the log Fvf at the specified time
		double getLogFvf(const double time) const;

		//	Accumulates the gradient of the structure curve at
		//	the specified time, relative to the structure instrument 
		//	rates
		void accumulateDiscountFactorGradient(const double time,
											  const double multiplier,
											  const GradientIterator begin,
											  const GradientIterator end) const;

		//	Returns a vector of the structure instruments
		//	StructureInstrumentList getInstrumentList() const;
        InstrumentDeltaVector calculateDelta(const BaseModel& model,
										   const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
										   const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows) const;

		//	Returns the instrument related to the index-th structure curve
		inline const StructureInstrument& getInstrument(const size_t index) const
		{
			return m_instrumentCurves[index]->getInstrument();
		}

		//	Returns the number of structure instruments in the curve
		inline size_t numberOfInstruments() const
		{
			return m_instrumentCurves.size();
		}
		
		//	Returns the spine curve details of the structure curve
        LTQuant::GenericDataPtr getCurveDetails() const;
        
        //	Adds the unfixed knot points of the spine curve of the structure curve to the list
        void getUnfixedKnotPoints(std::list<double>& points) const;
	
	private:
		//	Try to represent this with multiply and bind
		struct ScaledRateDerivativeFunctor: public std::unary_function<IStructureInstrumentCurvePtr, double>
		{
		public:
			explicit ScaledRateDerivativeFunctor(const double time,
												 const double multiplier):
				m_time(time),
				m_multiplier(multiplier)
			{
			}

			inline double operator()(const IStructureInstrumentCurvePtr& siCurve) const
			{
				return m_multiplier * siCurve->rateDerivative(m_time);
			}
		
		private:
			double m_time;
			double m_multiplier;
		};
		
		
		
		
		StructureInstrumentCurves m_instrumentCurves;
	};


	class StructureSurface
	{
	public:
		StructureSurface() { }
		explicit StructureSurface(const LTQuant::GenericData& masterTable);
		
		const std::vector<CurveTypeConstPtr>& curveTypes() const
		{
			return m_curveTypes;
		}

		//	Returns the discount factor at the specified time
		inline double getDiscountFactor(const double time, const CurveTypeConstPtr& curveType) const
		{
			return exp(-getLogFvf(time,curveType));
		}

		//	Returns the log Fvf at the specified time
		double getLogFvf(const double time, const CurveTypeConstPtr& curveType) const
		{
			return getStructureCurve(curveType)->getLogFvf(time);
		}
		
		//	Returns the discount factor at the specified time
		inline double getDiscountFactor(double time, double tenor) const
		{
			CurveTypeConstPtr curveType = CurveType::getFromYearFraction(tenor);
			return exp(-getLogFvf(time,curveType));
		}

		//	Returns the log Fvf at the specified time
		double getLogFvf(double time, double tenor) const
		{
			CurveTypeConstPtr curveType = CurveType::getFromYearFraction(tenor);
			return getStructureCurve(curveType)->getLogFvf(time);
		}

		//	Accumulates the gradient of the structure curve at
		//	the specified time, relative to the structure instrument 
		//	rates
		void accumulateDiscountFactorGradient(const double time,
											  const double multiplier,
											  const GradientIterator begin,
											  const GradientIterator end, const CurveTypeConstPtr& curveType) const
		{
			getStructureCurve(curveType)->accumulateDiscountFactorGradient(time,multiplier,begin,end);
		}

		//	Returns a vector of the structure instruments
		//	StructureInstrumentList getInstrumentList() const;
        InstrumentDeltaVector calculateDelta(const BaseModel& model,
										   const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
										   const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows, const CurveTypeConstPtr& curveType) const
		{
			return getStructureCurve(curveType)->calculateDelta(model,fundingRepFlows,indexRepFlows);
		}

		//	Returns the instrument related to the index-th structure curve
		inline const StructureInstrument& getInstrument(const size_t index, const CurveTypeConstPtr& curveType) const
		{
			return getStructureCurve(curveType)->getInstrument(index);
		}

		//	Returns the number of structure instruments in the curve
		inline size_t numberOfInstruments(const CurveTypeConstPtr& curveType) const
		{
			return getStructureCurve(curveType)->numberOfInstruments();
		}
		
		//	Returns the spine curve details of the structure curve
		LTQuant::GenericDataPtr getCurveDetails(const CurveTypeConstPtr& curveType) const
		{
			return getStructureCurve(curveType)->getCurveDetails();
		}

        //	Adds the unfixed knot points of the spine curve of the structure curve
        void getUnfixedKnotPoints(const CurveTypeConstPtr& curveType, std::list<double>& points) const
        {
            getStructureCurve(curveType)->getUnfixedKnotPoints(points);
        }
	
		///////////////////////////////////////////////////////////////////////////////////////////////////
		// singel curve case
		///////////////////////////////////////////////////////////////////////////////////////////////////

		//	Returns the discount factor at the specified time
		inline double getDiscountFactor(const double time) const
		{
			return exp(-getLogFvf(time));
		}

		//	Returns the log Fvf at the specified time
		double getLogFvf(const double time) const
		{
			return getStructureCurve()->getLogFvf(time);
		}

		//	Accumulates the gradient of the structure curve at
		//	the specified time, relative to the structure instrument 
		//	rates
		void accumulateDiscountFactorGradient(const double time,
											  const double multiplier,
											  const GradientIterator begin,
											  const GradientIterator end) const
		{
			getStructureCurve()->accumulateDiscountFactorGradient(time,multiplier,begin,end);
		}

		//	Returns a vector of the structure instruments
		//	StructureInstrumentList getInstrumentList() const;
        InstrumentDeltaVector calculateDelta(const BaseModel& model,
										   const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
										   const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows) const
		{
			return getStructureCurve()->calculateDelta(model,fundingRepFlows,indexRepFlows);
		}

		//	Returns the instrument related to the index-th structure curve
		inline const StructureInstrument& getInstrument(const size_t index) const
		{
			return getStructureCurve()->getInstrument(index);
		}

		//	Returns the number of structure instruments in the curve
		inline size_t numberOfInstruments() const
		{
			return getStructureCurve()->numberOfInstruments();
		}
		
		//	Returns the spine curve details of the structure curve
		LTQuant::GenericDataPtr getCurveDetails() const
		{
			return getStructureCurve()->getCurveDetails();
		}

        //	Adds the unfixed knot points of the spine curve of the structure curve
        void getUnfixedKnotPoints(std::list<double>& points) const
        {
            getStructureCurve()->getUnfixedKnotPoints(points);
        }
	private:
		

		StructureCurvePtr getStructureCurve(const CurveTypeConstPtr& curveType) const
		{
			for(size_t i=0; i<m_curveTypes.size();++i)
			{
				if(CurveType::Equals(*curveType,*m_curveTypes[i]))
				{
					return m_curves[i];
				}
			}

			if(CurveType::Equals(*curveType, *CurveType::_2D()) || CurveType::Equals(*curveType, *CurveType::_1W()) || CurveType::Equals(*curveType, *CurveType::_2W()))
			{
				for(size_t i=0; i<m_curveTypes.size();++i)
				{
					if(CurveType::Equals(*m_curveTypes[i], *CurveType::ON()))
					{
						return m_curves[i];
					}
				}
			}
			return m_defaultCurve;
		}

		StructureCurvePtr getStructureCurve() const
		{
			if(m_curves.size() > 0)
			{
				return m_curves[0];
			}
			return m_defaultCurve;
		}
		StructureCurvePtr			   m_defaultCurve;

		std::vector<CurveTypeConstPtr> m_curveTypes;
		std::vector<StructureCurvePtr> m_curves;
		
	};
}

#endif	//	__StructureCurve_H__
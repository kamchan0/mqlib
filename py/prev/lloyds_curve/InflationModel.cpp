/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "InflationModel.h"
#include "TransformCurve.h"
#include "UkpCurve.h"
#include "InterpolationMethodFactory.h"
#include "TransformFunctionFactory.h"
#include "SolverVariable.h"
#include "Maths\Problem.h"
#include "Data\GenericData.h"
#include "InterpolationCurve.h"
#include "ExtrapolationMethodFactoryDefs.h"
#include "ExtrapolationMethods.h"
#include "FlexYCFCloneLookup.h"
#include "SpineDataCache.h"

//	IDeA
#include "DataExtraction.h"
#include "DictMarketData.h"


using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;

    string InflationModel::getName()
    {
        return "Inflation";
    }

    BaseModelPtr InflationModel::createInstance(const LTQuant::GenericDataPtr& data, const FlexYCFZeroCurvePtr parent)
    {
        const BaseModelPtr inflationModel(new InflationModel(data, parent));
        return inflationModel;
    }

	InflationModel::InflationModel(const GenericDataPtr& masterTable, const FlexYCFZeroCurvePtr parent):
		SingleCurveModel(*masterTable, parent)
    {
        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(INFLATIONCURVE, CURVEDETAILS)));
		setValueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

        m_seasonality	= createSeasonality(masterTable);
        m_baseCurve		= createBaseCurve(masterTable);
    }

    double InflationModel::getDiscountFactor(const double flowTime) const
    {
        const double structure(StructureSurfaceHolder::holdee().getDiscountFactor(flowTime));
        const double seasonality(getSeasonality(flowTime));
        //double unadjustedRate(m_baseCurve->evaluate(flowTime));
        const double indexValue(getTenorDiscountFactor(flowTime, 0.) * seasonality * structure);
        return indexValue;
    }

    double InflationModel::getTenorDiscountFactor(const double flowTime, const double) const
    {
		return m_baseCurve->evaluate(flowTime);
    }

	double InflationModel::getAdjustmentFactor(const double flowTime) const
	{
		return StructureSurfaceHolder::holdee().getDiscountFactor(flowTime) * m_seasonality->evaluate(flowTime);
	}


    /// after calib is finished destory all of the calibration
    /// instruments so they do not take up memory via the CachedInstrument component
    // Inflation model does hold instruments
    void InflationModel::finishCalibration()
    {
        SingleCurveModel::finishCalibration();
    }
    double InflationModel::getSeasonality(const double flowTime) const
    {
        if(m_seasonality)
        {
            double seasonalityAdjustment(1.0);
            double theoreticalYears(getTheoreticalYears(flowTime)); 
            if(theoreticalYears < 0.0)
            {
                double wholeYears(floor(theoreticalYears));
                double oneYearAdjust(m_seasonality->evaluate(1.0));
                seasonalityAdjustment *= pow(oneYearAdjust, wholeYears);
                theoreticalYears -= wholeYears;
            }
            else
            {
                double wholeYears(floor(theoreticalYears));
                if(wholeYears > 0.0)
                {
                    double oneYearAdjust(m_seasonality->evaluate(1.0));
                    seasonalityAdjustment *= pow(oneYearAdjust, wholeYears);
                    theoreticalYears -= wholeYears;
                }
            }
            seasonalityAdjustment *= m_seasonality->evaluate(theoreticalYears);
            return seasonalityAdjustment;
        }
        else
        {
            return 0;
        }
    }

    // Computes the DiscountFactor sensitivities with respect to the knot-points' variables
    //  at a certain flow-time
    void InflationModel::accumulateDiscountFactorGradient(const double flowTime, 
                                                          double multiplier,
                                                          GradientIterator gradientBegin,
                                                          GradientIterator gradientEnd) const
    {
        multiplier *= StructureSurfaceHolder::holdee().getDiscountFactor(flowTime) * getSeasonality(flowTime);
        m_baseCurve->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

    void InflationModel::accumulateTenorDiscountFactorGradient(const double flowTime, 
                                                               const double /* tenor */, 
                                                               double multiplier, 
                                                               GradientIterator gradientBegin,
                                                               GradientIterator gradientEnd) const
    {
		m_baseCurve->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

    ICurvePtr InflationModel::createSeasonality(const LTQuant::GenericDataPtr& data)
    {
        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(INFLATIONCURVE, CURVEPARAMETERS)));
        const GenericDataPtr instrumentListTable(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(INFLATIONCURVE, INSTRUMENTLIST)));
        if(!instrumentListTable->doesTagExist("Seasonal"))
        {
            ICurvePtr empty;
            return empty;
        }
        GenericDataPtr seasonalTable(instrumentListTable->get<GenericDataPtr>("Seasonal", 0));

        // if we just have titles or nothing exit
        if(seasonalTable->numItems() < 2)
        {
            ICurvePtr empty;
            return empty;
        }

        // anything other than 12 entries is an error
        if(seasonalTable->numItems() != 13)
        {
            LT_THROW_ERROR("Wrong number of entries in Seasonal Table")
        }

        string seasonalInterpolator;
        parametersTable->permissive_get("Seasonal.Interpolator", 0, seasonalInterpolator, StraightLineInterpolation::getName());
       
        KnotPointsPtr knotPoints(new KnotPoints);
        InterpolationCurvePtr interpolationCurve(new UkpCurve(knotPoints, 
                                                              InterpolationMethodFactory::create((seasonalInterpolator == "StraightLineExtend"?
                                                                                                  StraightLineInterpolation::getName() :
                                                                                                  seasonalInterpolator))));
        
        // Flat or straight line right extrapolation, depending on the value of "seasonal interpolator"
        //  to ensure backward compatibility
        RightExtrapolationPtr rightExtrapolation;
        if(seasonalInterpolator == StraightLineInterpolation::getName())
        {
            rightExtrapolation = RightExtrapolationMethodFactory::createInstance(RightFlatExtrapolationMethod::getName(), 
                   [interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
			            {interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);});

        } else
        {
            // "StraightLineExtend" by default
            rightExtrapolation = RightExtrapolationMethodFactory::createInstance(RightStraightLineExtrapolationMethod::getName(), 
                   [interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
			            {interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);});
        }
        
        ICurvePtr seasonalityCurve  (
            new BaseCurve(knotPoints, 
                             interpolationCurve,
                             LeftExtrapolationMethodFactory::createInstance(LeftFlatExtrapolationMethod::getName(),
                                    [interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
										 {interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}),
                             rightExtrapolation
                            )       );

		//**************************************************************************************************
		// Seasonality model with 12th implied variable used to impose consistency condition: theta(12) = 1.0
		// The choice of the implied parameter given by SeasonalBaseMonth:
        string seasonalQuoteType;
        parametersTable->permissive_get("Seasonal Quote Type", 0, seasonalQuoteType, string("BaseMonthImplied"));

        // TODO SD - Hardcoded for testing
		if (compareNoCase(seasonalQuoteType, "BaseMonthImplied")) 
        {
			// compute seasonality base date
			string seasonalityBaseMonthStr;
            parametersTable->permissive_get("Seasonal Base Month", 0, seasonalityBaseMonthStr, string("Jan"));
            LT::date::month_type seasonalityBaseMonth(ModuleDate::returnMonth(seasonalityBaseMonthStr));

			// compute implied seasonality factor
			double theta(1.0);
			for(size_t i(0); i < 12; ++i)
			{
				double value(seasonalTable->get<double>("Value", i));
				string monthString(seasonalTable->get<string>("Month", i));
				LT::date::month_type monthNum(ModuleDate::returnMonth(monthString));
				if (monthNum != seasonalityBaseMonth)
					theta *= (1.0+value);
			}
			double impliedSeasonality = 1.0/theta - 1.0;
		
			// compute theta curve
			seasonalityCurve->addKnotPoint(KnotPoint(0.0, 1.0, true));
			theta = 1.0;
			double janSeasonality(0.0);
			for(size_t i(0); i < 12; ++i)
			{
				double value(seasonalTable->get<double>("Value", i));
				string monthString(seasonalTable->get<string>("Month", i));
				LT::date::month_type monthNum(ModuleDate::returnMonth(monthString));
				double x(double(monthNum) / 12.0);
				if (monthNum == seasonalityBaseMonth) {
					value = impliedSeasonality;
				} 
				if (i == 0) {
					janSeasonality = value;
				}
				theta *= (1.0+value);
				seasonalityCurve->addKnotPoint(KnotPoint(x, theta, true));
			}
			// add last point for december interpolation
			double x = 13.0/12.0;
			theta *= (1.0 + janSeasonality);
			seasonalityCurve->addKnotPoint(KnotPoint(x, theta, true));
		} else if (compareNoCase(seasonalQuoteType, "Normalised")) {

			//**************************************************************************************************
			// Seasonality model with 13th implied variable used to impose consistency condition: theta(12) = 1.0

			// compute the required adjustment so that total seasonality always compounds to 1.0
			std::vector<double> values_hat(12);
			double runningTotal_hat = 0.0;
			int monthIndex = 0;
			for(size_t i(0); i < 12; ++i)
			{
				double value(seasonalTable->get<double>("Value", i));

				string monthString(seasonalTable->get<string>("Month", i));
				LT::date::month_type monthNum(ModuleDate::returnMonth(monthString));
				if (int(monthNum) != monthIndex+1)
					LT_THROW_ERROR("Seasonal Table entries must be ordered")
				monthIndex = int(monthNum);
				values_hat[i] = ::log(1 + value);
				runningTotal_hat += values_hat[i];
			}

			double adj = runningTotal_hat / 12.0;
			seasonalityCurve->addKnotPoint(KnotPoint(0.0, 1.0, true));
			double theta(1.0);
			for(size_t i(0); i < 12; ++i)
			{
//				double value(seasonalTable->get<double>("Value", i));
				string monthString(seasonalTable->get<string>("Month", i));
				LT::date::month_type monthNum(ModuleDate::returnMonth(monthString));
				double x(double(monthNum) / 12.0);
				theta *= ::exp(values_hat[i] - adj);
				seasonalityCurve->addKnotPoint(KnotPoint(x, theta, true));
			}
			// add last point for december interpolation
			double x = 13.0/12.0;
			theta *= ::exp(values_hat[0] - adj);
			seasonalityCurve->addKnotPoint(KnotPoint(x, theta, true));
		} else 
            LT_THROW_ERROR("Invalid Seasonal quote type")


        seasonalityCurve->finalize();
        return seasonalityCurve;
    }

    ICurvePtr InflationModel::createBaseCurve(const LTQuant::GenericDataPtr& data)
    {
        const GenericDataPtr details(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(INFLATIONCURVE, CURVEDETAILS)));
			//	data->get<GenericDataPtr>("Curve Details", 0);
		const string ccyStr(IDeA::extract<string>(*details, IDeA_KEY(CURVEDETAILS, ASSET)));	//	details->get<string>("Asset", 0);
		const string market(IDeA::extract<string>(*details, IDeA_KEY(CURVEDETAILS, MARKET)));	//	details->get<string>("Market", 0);
		const string assetName(ccyStr + "." + market);

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(INFLATIONCURVE, CURVEPARAMETERS)));
		const GenericDataPtr eventTable(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(INFLATIONCURVE, EVENTSCHEDULE)));
			//	data->doesTagExist("Events") ? data->get<GenericDataPtr>("Events", 0) : data->get<GenericDataPtr>("Event Schedule", 0));

        string interpolator;
		IDeA::permissive_extract<string>(parametersTable, IDeA_KEY(CURVEPARAMETERS, INTERPOLATOR), interpolator, StraightLineInterpolation::getName());
		//	parametersTable->permissive_get("Interpolator", 0, interpolator, string(StraightLineInterpolation::getName()));
        
        const KnotPointsPtr knotPoints(new KnotPoints);
        const InterpolationCurvePtr baseInterpCurve(new UkpCurve(knotPoints, 
                                                            InterpolationMethodFactory::create((interpolator == "StraightLineExtend"?
                                                                                                StraightLineInterpolation::getName():
                                                                                                interpolator))));

        string transformFuncName;
        parametersTable->permissive_get("Transform", 0, transformFuncName, string("Null"));
        
		m_transformFunction = TransformFunctionFactory::create(transformFuncName);
 
        // Flat left extrapolation
        const LeftExtrapolationPtr leftExtrapolation(LeftExtrapolationMethodFactory::createInstance(LeftFlatExtrapolationMethod::getName(),
                   [baseInterpCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
			            {baseInterpCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}));
                                  
        // Flat or straight line right extrapolation, depending on the value of "seasonal interpolator"
        //  to ensure backward compatibility
        RightExtrapolationPtr rightExtrapolation;
        if(interpolator == StraightLineInterpolation::getName())
        {
            rightExtrapolation = RightExtrapolationMethodFactory::createInstance(RightFlatExtrapolationMethod::getName(), 
                   [baseInterpCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
			            {baseInterpCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);});
        }
        else 
        {
            // "StraightLineExtend" by default
            rightExtrapolation = RightExtrapolationMethodFactory::createInstance(RightStraightLineExtrapolationMethod::getName(), 
                   [baseInterpCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
			            {baseInterpCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);});
        }

        const TransformCurvePtr curve(new TransformCurve(knotPoints,
														 baseInterpCurve,
														 leftExtrapolation,
														 rightExtrapolation,
														 m_transformFunction));

		const size_t nbEvents(IDeA::numberOfRecords(*eventTable));

        for(size_t i(0); i < nbEvents; ++i)
        {
			string eventAssetName = eventTable->get<string>("Asset",0);
			if (compareNoCase(eventAssetName, assetName)) 
            {
				LT::date pubDate(eventTable->get<LT::date>("Fixing Date", i));
				string state(eventTable->get<string>("State", i));

                // only include points that could have been known on build date with value been given
                if(pubDate <= getValueDate())
				{
					double value = 0.0;
					bool isValueGiven = eventTable->permissive_get("Value",i, value, 0.0); 
					if (isValueGiven) {
						string stateCertain("Certain");
						string state(stateCertain);
						bool isStateGiven = eventTable->permissive_get("State", i, state, stateCertain);
						// If state is given, then only read values with certain. 
						// This is to ensure P&L attribution in CalcServices continue to work
						if ( !isStateGiven || (isStateGiven && compareNoCase(state, stateCertain))) {
							LT::date refDate = (eventTable->doesTagExist("Reference Date") ? eventTable->get<LT::date>("Reference Date", i) : eventTable->get<LT::date>("ReferenceDate", i));
							double flowTime(ModuleDate::getYearsBetweenAllowNegative(getValueDate(), refDate));
							double structure(StructureSurfaceHolder::holdee().getDiscountFactor(flowTime));
							double seasonality(getSeasonality(flowTime));
							double unadjustedValue(value / structure / seasonality);

							curve->addKnotPoint(KnotPoint(flowTime, unadjustedValue, true));
						}
					}
				}
			}
        }
        curve->finalize();
        return curve;
    }

    double InflationModel::getTheoreticalYears(const double time) const
    {
        const double timeInDays(time * 365.0);
		const double factor = timeInDays > 0.0 ? 0.5 : -0.5;
        const long numDays(static_cast<long>(timeInDays + factor));
        LT::date interpDate;
        interpDate = getValueDate() + numDays;

		const LT::date firstDayInMonth(interpDate.year(), interpDate.month(), 1);
		const LT::date firstDayInNextMonth(DateFunctional::addMonths(firstDayInMonth.getAsLong(), 1, true));
		const LT::DateDuration daysInMonth = firstDayInNextMonth - firstDayInMonth;
        double result(static_cast<double>(interpDate.year() - getValueDate().year()));
        result += static_cast<double>(interpDate.month()) / 12.0; // the seasonality for the reference index on the first day of MONTH is the value of the seasonality quoted for MONTH
        const double dayFraction(static_cast<double>(interpDate.day() - 1) / static_cast<double>(daysInMonth.days()));
        result += dayFraction / 12.0;
        return result;
    }

    // The following functions might be factored into SingleCurveModel as they are 
    //  the same for StripperModel too.
    void InflationModel::addKnotPoint(const KnotPoint& knotPoint) const
    {
        m_baseCurve->addKnotPoint(knotPoint);
    }

    /*void InflationModel::addInitialSpotRate(const double flowTime,
                                            const double spotRate) const
    {
        m_baseCurve->addFixedKnotPoint(flowTime, spotRate);
    }*/

    void InflationModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem)
    {
        m_baseCurve->addUnknownsToProblem(problem);
    }

	void InflationModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
											   IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		m_baseCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
	}
    
    void InflationModel::update()
    {
        m_baseCurve->update();
    }
   
    void InflationModel::finalize()
    {
        m_baseCurve->finalize();
    }

	double InflationModel::inflationIndexToVariable(const double x) const 
	{ 
		if(m_transformFunction)
		{
			return m_transformFunction->doInverseTransform(x); 
		}
		else
		{
			LT_THROW_ERROR( "The transform function of the inflation model is not set" );
		}
	}

	LTQuant::GenericDataPtr InflationModel::getSpineCurvesDetails() const
	{
		LTQuant::GenericDataPtr spineCurvesDetailsData(new LTQuant::GenericData("Inflation Spine Curves", 0));
		
		//	Set the base curve
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Base Curve", 0, m_baseCurve->getCurveDetails());
		
		//	Set the structure curve
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Structure", 0, StructureSurfaceHolder::holdee().getCurveDetails());

		//	Set the	seasonality curve
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Seasonality", 0, m_seasonality->getCurveDetails());
		
		return spineCurvesDetailsData;
	}

	void InflationModel::getSpineInternalData(SpineDataCachePtr& sdp) const { 
		m_baseCurve->getCurveInternalData(sdp->xy_);
		m_seasonality->getCurveInternalData(sdp->xy_);
	}

	void InflationModel::assignSpineInternalData(SpineDataCachePtr& sdp) {
		knot_points_container::const_iterator p = sdp->xy_.begin();
		m_baseCurve->assignCurveInternalData(p++);
		m_seasonality->assignCurveInternalData(p++);
	}

    void InflationModel::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        //	Set the base curve
        m_baseCurve->getUnfixedKnotPoints(points);

        //	Set the structure curve
        StructureSurfaceHolder::holdee().getUnfixedKnotPoints(points);

        //	Set the	seasonality curve
        m_seasonality->getUnfixedKnotPoints(points);
    }
    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure the clone has the same directed graph relationship as the original.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created instances.
    */
    InflationModel::InflationModel(InflationModel const& original, CloneLookup& lookup) :
        SingleCurveModel(original, lookup),
        m_seasonality(lookup.get(original.m_seasonality)),
        m_baseCurve(lookup.get(original.m_baseCurve)),
        m_transformFunction(original.m_transformFunction->clone())
    {
    }

    /**
        @brief Create a clone of this instance.

        Use a lookup of previously created clones to ensure that directed graph relationships are preserved.

        @param lookup The lookup of previuosly created clones.
    */
    ICloneLookupPtr InflationModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new InflationModel(*this, lookup));
    }
}

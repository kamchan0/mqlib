#include "stdafx.h"
#include "Structure.h"
#include "InterpolationMethod.h"
#include "Data\GenericData.h"
#include "BaseCurve.h"
#include "UkpCurve.h"
#include "InterpolationMethodFactory.h"
#include "StraightLineInterpolation.h"
#include "ExtrapolationMethodFactoryDefs.h"
#include "ExtrapolationMethods.h"

#include "DataExtraction.h"
#include "DictYieldCurve.h"


using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;

    /// Creates a structure object from the table based market data
    /// definition, the various structure elements are stored as instruments.
    /// All of the adjusment types are implementable in terms of steps
    /// and so we build up a master list of the step components required
    /// and the times at which they occur.
    /// This list is then sorted and a knot points added to 
    /// the curve of logFVF values.
    /// If there is still a step active at the end we add an additional 
    /// point so that the straight line extend method used
    /// will extrapolate correctly
    Structure::Structure(const GenericDataPtr data)
    {
        // same as in InflationModel (->refactoring?)
        KnotPointsPtr knotPoints(new KnotPoints);
        InterpolationCurvePtr interpolationCurve(new UkpCurve(knotPoints, InterpolationMethodFactory::create(StraightLineInterpolation::getName())));
        m_curve = BaseCurvePtr(new BaseCurve(knotPoints, 
                                   interpolationCurve,
                                   LeftExtrapolationMethodFactory::createInstance(LeftFlatExtrapolationMethod::getName(),
										[interpolationCurve] (double x, double multiplier, const GradientIterator& gradientBegin, const GradientIterator& gradientEnd) 
											{interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}),
                                   RightExtrapolationMethodFactory::createInstance(RightStraightLineExtrapolationMethod::getName(), 
                                        [interpolationCurve] (double x, double multiplier, const GradientIterator& gradientBegin, const GradientIterator& gradientEnd) 
											{interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);})                                
                                  ));

        // get the basic details out of the curve
        const GenericDataPtr details(IDeA::extract<LTQuant::GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date buildDate(IDeA::extract<LT::date>(*details, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        const GenericDataPtr instrumentsTable(IDeA::extract<LTQuant::GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST)));

        // this is the internal list of steps that we will map each of the
        // adjustments types onto
        //	vector< pair<double, double> > steps;

        // a turn is modeled as a step up on the start date and a step 
        // down a day later
        // note that turns are quoted as change
        // in futures quote rather than as an increased overnight rate
        // and so we need to multiply by 91 to get it in the correct form
        if(instrumentsTable->doesTagExist("Turns"))
        {
            GenericDataPtr subTable(instrumentsTable->get<GenericDataPtr>("Turns", 0));
            for(size_t i(0); i < subTable->numItems() - 1; ++i)
            {
                double spread(subTable->get<double>("Spread", i) * 91.0);
                LT::date startDate(subTable->get<LT::date>("Date", i));
                if(startDate > buildDate)
                {
                    LT::date endDate(startDate + 1);
                    double startTime(ModuleDate::getYearsBetween(buildDate, startDate));
                    double endTime(ModuleDate::getYearsBetween(buildDate, endDate));
                    //	steps.push_back(make_pair(startTime, spread));
                    //	steps.push_back(make_pair(endTime, -spread));
					m_instruments.add(Instrument(startTime, spread, std::string("Turn"), startDate));
					m_instruments.add(Instrument(endTime, -spread, "Turn", endDate));
				}
            }
        }

        // steps are modeled directly
        if(instrumentsTable->doesTagExist("Steps"))
        {
            GenericDataPtr subTable(instrumentsTable->get<GenericDataPtr>("Steps", 0));
            for(size_t i(0); i < subTable->numItems() - 1; ++i)
            {
                double spread(subTable->get<double>("Spread", i));
                LT::date startDate(subTable->get<LT::date>("Date", i));
                if(startDate > buildDate)
                {
                    double startTime(ModuleDate::getYearsBetween(buildDate, startDate));
                    //	steps.push_back(make_pair(startTime, spread));
					m_instruments.add(Instrument(startTime, spread, "Step", startDate));
				}
            }
        }

        // Bumps are a step at the start date and a step
        // at the end date
        if(instrumentsTable->doesTagExist("Bumps"))
        {
            GenericDataPtr subTable(instrumentsTable->get<GenericDataPtr>("Bumps", 0));
            for(size_t i(0); i < subTable->numItems() - 1; ++i)
            {
                double spread(subTable->get<double>("Spread", i));
                LT::date startDate(subTable->get<LT::date>("Start Date", i));
                if(startDate > buildDate)
                {
                    LT::date endDate(subTable->get<LT::date>("End Date", i));
                    double startTime(ModuleDate::getYearsBetween(buildDate, startDate));
                    double endTime(ModuleDate::getYearsBetween(buildDate, endDate));
                    //	steps.push_back(make_pair(startTime, spread));
                    //	steps.push_back(make_pair(endTime, -spread));
					m_instruments.add(Instrument(startTime, spread, "Bump", startDate));
					m_instruments.add(Instrument(endTime, -spread, "Bump", endDate));
				}
            }
        }

        // sort the steps so that we can build the logFvf curve correctly
        //	sort(steps.begin(), steps.end());
		//	sort(m_instruments.begin(), m_instruments.end(), InstrumentCompare());

        // we always add a point at x=0, y=0
        // so that even if no instruments are defined we always get a result
        // and that the first period is defined
        m_curve->addKnotPoint(KnotPoint(0.0, 0.0, true));

        // use these values to keep track of where we are
        double currentSpread(0.0);
        double lastTime(0.0);
        double previousValue(0.0);

		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// NOT IDEAL with m_instruments... we would need to adjust some instrument name
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		// as a list of instrument name and their rates too.
        // add each of the steps keeping a running total of the current rate
        // and the logFvf so far
        for(size_t i(0); i < /*steps.size()*/ m_instruments.size(); ++i)
        {
            //	if(steps[i].first > lastTime)
			if(m_instruments[i].time() > lastTime)
			{
                /* double nextValue(previousValue + currentSpread * (steps[i].first - lastTime));
                m_curve->addKnotPoint(KnotPoint(steps[i].first, nextValue, true));
                lastTime = steps[i].first;
                */
				const double nextValue(previousValue + currentSpread * (m_instruments[i].time() - lastTime));
				m_curve->addKnotPoint(KnotPoint(m_instruments[i].time(), nextValue, true));
				lastTime = m_instruments[i].time();
				previousValue = nextValue;
            }

            //	currentSpread += steps[i].second;
			currentSpread += m_instruments[i].spread();
		}

        // we need to ensure extrapolation works correctly
        // so if there is no active spread add an extra point the 
        // same as the last one
        // otherwise continue the slope out for 1 year
        if(doubleEquals(currentSpread, 0.0))
        {
            m_curve->addKnotPoint(KnotPoint(lastTime + 1.0, previousValue, true));
        }
        else
        {
            // we are adding exactly 1.0 year so don't need to
            // multiply currentSpread by time
            const double nextValue(previousValue + currentSpread);
            m_curve->addKnotPoint(KnotPoint(lastTime + 1.0, nextValue, true));
        }

        // we're done so finish off the curve
        m_curve->finalize();
    }

    /// returns the log(FVF) associated with this Structure
    /// since this is the form in which the data is stored
    /// it is implemented using interpolation
    const double Structure::getLogFvf(const double x) const
    {
        return m_curve->evaluate(x);
    }

    /// returns the dcf associated with this Structure
    /// since data is stored in logFVF form
    /// we need to return exp(-logFvf)
    const double Structure::getDcf(const double x) const
    {
        const double logFvf(getLogFvf(x));
        return exp(-logFvf);
    }

	LTQuant::GenericDataPtr Structure::getCurveDetails() const
	{
		const LTQuant::GenericDataPtr structureSpineCurve(m_curve->getCurveDetails());

		// add the discount factors
		for(size_t i(0); i < structureSpineCurve->numItems() - 1; ++i)
		{
			//	first point is (0,0) due to LogFvf formulation and last one does not
			//	come from a structure instrument, hence the following two conditions:
			if(i > 0 && i <= m_instruments.size())
			{
				IDeA::inject<std::string>(*structureSpineCurve, IDeA_KEY(SPINECURVETABLE, INSTRUMENT), i, m_instruments[i-1].type());
				IDeA::inject<std::string>(*structureSpineCurve, IDeA_KEY(SPINECURVETABLE, MATURITY), i, m_instruments[i-1].maturity());
			}
			//	structureSpineCurve->set<double>("df", i, getDcf(structureSpineCurve->get<double>("x", i)));
			IDeA::inject<double>(*structureSpineCurve, IDeA_KEY(SPINECURVETABLE, DF), i, 
								 getDcf(IDeA::extract<double>(*structureSpineCurve, IDeA_KEY(SPINECURVETABLE, X), i)));
		}

		return structureSpineCurve;
	}


}   // FlexYCF

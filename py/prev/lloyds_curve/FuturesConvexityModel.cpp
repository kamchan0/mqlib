#include "stdafx.h"
#include "FuturesConvexityModel.h"
#include "ImpliedVolQuotes.h"
#include "InstantaneousVolCalibrator.h"
#include "InstantaneousVolFactory.h"
#include "Models/LiborMarketModel.h"
#include "Models/InstantaneousVol.h"
#include "Models/RebonatoVol.h"
#include "Models/RebonatoCorrelation.h"
#include "Models/CorrelationStructure.h"
#include "Models/LMMSimple.h"
#include "Pricers/Black76.h"
#include "Pricers/PriceSupplier.h"
#include "Pricers/TwoQSwaptionCube.h"
#include "GlobalComponentCache.h"
#include "ImpliedVolQuote.h"
#include "LiborMarketModelFactory.h"

// ModuleStaticData
#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"

using namespace std;
using namespace LTQC;
using namespace LTQuant;

namespace
{
    // This functor cross-compares the time to expiry of a forward rate and a double
    struct ForwardRateExpiryCompare
    {
    public:
        bool operator()(const FlexYCF::ForwardRatePtr& lhs, const FlexYCF::ForwardRatePtr& rhs) const 
        { 
            return operator()(lhs->getTimeToExpiry(), rhs->getTimeToExpiry()); 
        }
        
        bool operator()(const FlexYCF::ForwardRatePtr& lhs, const double rhs) const 
        { 
            return operator()(lhs->getTimeToExpiry(), rhs); 
        }

        bool operator()(const double lhs, const FlexYCF::ForwardRatePtr& rhs) const 
        { 
            return operator()(lhs, rhs->getTimeToExpiry()); 
        }

    private:
        bool operator()(const double lhs, const double rhs) const 
        { 
            return lhs < rhs; 
        }
    };

}


namespace FlexYCF
{

    FuturesConvexityModel::FuturesConvexityModel(const LT::date valueDate,      
                                                 const ModuleStaticData::IRIndexPropertiesPtr& irIndexProperties,
                                                 const vector<LT::date>& futuresStartDates,
                                                 const vector<LT::date>& futuresEndDates,
                                                 const ImpliedVolQuotes& impliedVolQuotes,
                                                 const LTQuant::PriceSupplierPtr& priceSupplier,
                                                 const LTQuant::GenericDataPtr& lmmTable,
                                                 const LTQuant::InstantaneousVolPtr& instantaneousVolatility,
                                                 const LTQuant::CorrelationStructurePtr& correlationStructure,
                                                 GlobalComponentCache& globalComponentCache):
        m_timeEpsilon(1.e-4),                   // time epsilon to speed up algorithms, < 1 day, could be a parameter
        m_valueDate(valueDate),
        m_currencyName(irIndexProperties->getCurrencyName()),
        m_indexName(irIndexProperties->getIndexName()),
        m_futuresStartDates(futuresStartDates),
        m_futuresEndDates(futuresEndDates),
        m_impliedVolQuotes(impliedVolQuotes),
        m_priceSupplier(priceSupplier),
        m_lmmTable(lmmTable),
        m_instantaneousVolatility(instantaneousVolatility),
        m_correlationStructure(correlationStructure),
        m_instantaneousVolCalibrator(new InstantaneousVolCalibrator)        
    {
        if(m_futuresStartDates.size() != m_futuresEndDates.size())
        {
            LT_THROW_ERROR("futuresStartDates and futuresEndDates must have the same size.");
        }
        
        LT_LOG << "Building forward rates from futures:" << endl;
        for(size_t k(0); k < m_futuresStartDates.size(); ++k)
        {

			LT::date fixingDate = LTQuant::calcFixingDate(m_futuresStartDates[k], irIndexProperties); // fixing date

            LT_LOG << m_futuresStartDates[k] << "\t" << m_futuresEndDates[k] << endl;
            m_forwardRates.push_back( globalComponentCache.get( ForwardRate::Arguments( m_valueDate,
																						fixingDate,
                                                                                        m_futuresStartDates[k],
                                                                                        m_futuresEndDates[k],
                                                                                        "3M",
                                                                                        irIndexProperties->getBasis(),
                                                                                        globalComponentCache
                                                                                      )));
        }

        if(!impliedVolQuotes.empty())
        {
			LT::date fixingDate = LTQuant::calcFixingDate(m_futuresStartDates[m_futuresStartDates.size()-1], irIndexProperties); // fixing date
            ForwardRatePtr lastForwardRate(globalComponentCache.get(ForwardRate::Arguments(m_valueDate,
																						   fixingDate,
                                                                                           m_futuresStartDates[m_futuresStartDates.size()-1],
                                                                                           m_futuresEndDates[m_futuresEndDates.size()-1],
                                                                                           "3M",
                                                                                           irIndexProperties->getBasis(),
                                                                                           globalComponentCache)));

            const ImpliedVolQuote lastQuote(impliedVolQuotes[impliedVolQuotes.size()-1]);            
            LT_LOG << "Building forward rates after futures:" << endl;

            while( lastForwardRate->getTimeToExpiry() < lastQuote.getTimeToExpiry()
                || lastForwardRate->getTimeToMaturity() < lastQuote.getTimeToMaturity() )
            {
				LT::date startDate = addMonths(lastForwardRate->getStartDate(), 3, irIndexProperties->getCalendar());
				LT::date fixingDate = LTQuant::calcFixingDate(startDate, irIndexProperties); // fixing date
                lastForwardRate = globalComponentCache.get(ForwardRate::Arguments(m_valueDate,
																				  fixingDate,
                                                                                  startDate,
                                                                                  addMonths(lastForwardRate->getEndDate(), 3, irIndexProperties->getCalendar()),
                                                                                  "3M",
                                                                                  irIndexProperties->getBasis(),
                                                                                  globalComponentCache));
                m_forwardRates.push_back(lastForwardRate);
            }
            
        }
        // calibrateToSwaptionCube();
    }

    // should we pass the last Futures expiry date, its end date, the tenor structure of the futures?
    FuturesConvexityModelPtr FuturesConvexityModel::create(const LTQuant::GenericDataPtr& convexityModelTable,
                                                           const LTQuant::GenericDataPtr& futuresTable,
                                                           const LT::date valueDate,
                                                           const ModuleStaticData::IRIndexPropertiesPtr& irIndexProperties,
                                                           const LTQuant::PriceSupplierPtr priceSupplier,
                                                           GlobalComponentCache& globalComponentCache)
    {
        // Only create the the convexity model if there exists a convexity model
        if(static_cast<bool>(convexityModelTable))
        {
            // 1. Retrieve parameters
            const double defaultBeta(10.0);
            const string defaultInstantaneousVolModelName(RebonatoVol::getName());
           
            double beta(defaultBeta);
            string instantaneousVolModelName(defaultInstantaneousVolModelName);
            LTQuant::GenericDataPtr lmmTable;

            convexityModelTable->permissive_get<double>("Beta", 0, beta, defaultBeta);
            convexityModelTable->permissive_get<string>("Vol Model", 0, instantaneousVolModelName, defaultInstantaneousVolModelName);
            convexityModelTable->permissive_get<LTQuant::GenericDataPtr>("LMM", 0, lmmTable);

          
     
            // 2. Create start/end dates of the forwards to use: use start/end dates of the futures
            vector<LT::date> futuresStartDates, futuresEndDates;

            for(size_t index(0); index < futuresTable->numItems() - 1; ++index)
            {
                const string description(futuresTable->get<string>("Description", index));   // Date description (e.g. 6M)
                if(!description.empty())
                {
                    futuresStartDates.push_back(futuresTable->get<LT::date>("Start Date", index));
                    futuresEndDates.push_back(futuresTable->get<LT::date>("End Date", index));
                }
            }  

             // Sort the futures start/end dates
            sort(futuresStartDates.begin(), futuresStartDates.end());
            sort(futuresEndDates.begin(), futuresEndDates.end());

            // 3. create the dates to calibrate the instantaneous volatility to: by default, it's the forwards start/end dates
            // vector<date> startDates, endDates;
            ImpliedVolQuotes impliedVolQuotes;

            LTQuant::GenericDataPtr tenorStructureTable;
            convexityModelTable->permissive_get<LTQuant::GenericDataPtr>("Tenor Structure", 0, tenorStructureTable, LTQuant::GenericDataPtr());
            
            vector<double> tenorStructure;
            LT::date startDate, endDate;

            // if there is a table of start/end dates referenced in the convexity model use it:
            if(tenorStructureTable)
            {
                LT_LOG << "Using custom tenor structure:" << endl;
                LT_LOG << "Start Date\tEnd Date" << endl;

                for(size_t index(0); index < tenorStructureTable->numItems() - 1; ++index)
                {
                    startDate = tenorStructureTable->get<LT::date>("Start Date", index);
                    endDate = tenorStructureTable->get<LT::date>("End Date", index);
                    try 
                    {
                        impliedVolQuotes.add(ImpliedVolQuote(valueDate,
                                                             startDate,
                                                             endDate,
                                                             tenorStructureTable->get<double>("Strike", index)));
                    } catch(...)
                    {
                        impliedVolQuotes.add(ImpliedVolQuote(valueDate,
                                                             startDate,
                                                             endDate));
                    }
                    tenorStructure.push_back(ModuleDate::getYearsBetween(valueDate, startDate));  
                    // LT_LOG << startDates[startDates.size()-1] << "\t" << endDates[endDates.size() - 1] << endl;
                }
            }
            else
            {
                for(size_t pt(0); pt < futuresStartDates.size(); ++pt)
                {
                    tenorStructure.push_back(ModuleDate::getYearsBetween(valueDate, futuresStartDates[pt]));
                }
                LT_LOG << "Using default tenor structure (futures dates)." << endl;
            }

            // 4. Create a constant Instantaneous Volatility:
            LTQuant::InstantaneousVolPtr instantaneousVolatility(InstantaneousVolFactory::create(instantaneousVolModelName,
                                                                                        tenorStructure));
            
            LT_LOG << "Instant Vol Type '" << instantaneousVolModelName << "' ";
            if(!instantaneousVolatility)
            {
                 LT_LOG << "NOT ";
            } 
            LT_LOG << "built successfully." << endl;
            
            // 5. Create Correlation Structure (could use a factory):
            LTQuant::CorrelationStructurePtr correlationStructure(new RebonatoCorrelation(beta));

            // 6. Create the Futures convexity model:
            return FuturesConvexityModelPtr(new FuturesConvexityModel(valueDate, 
                                                                      irIndexProperties,
                                                                      futuresStartDates,
                                                                      futuresEndDates,
                                                                      impliedVolQuotes,
                                                                      priceSupplier, 
                                                                      lmmTable,
                                                                      instantaneousVolatility,
                                                                      correlationStructure,
                                                                      globalComponentCache));
        }
        return FuturesConvexityModelPtr();
    }

    void FuturesConvexityModel::calibrateToSwaptionCube(/*const BaseModelPtr& model*/)
    {
        const size_t lookupIndex(m_priceSupplier->getLookupValue(m_currencyName, m_indexName)); 
        SwaptionCubePtr swaptionCube(m_priceSupplier->getSwaptionCube(lookupIndex));
        if(!static_cast<bool>(swaptionCube))
        {
            LT_LOG << "No swaption cube in the price supplied." << endl;
        }
		TwoQSwaptionCubePtr twoQSwaptionCube(std::tr1::dynamic_pointer_cast<TwoQSwaptionCube>(m_priceSupplier->getSwaptionCube(lookupIndex)));

        LT::date expiryDate, maturityDate;         
        double timeToExpiry, timeToMaturity, blackVol, forwardRate;

        /// Fills the caplet implied vol quotes with 2Q caplet vols
        if(static_cast<bool>(twoQSwaptionCube))
        {
            double alpha(0.0);
            // get the parameter of LMM displaced diffusion if it exists:
            if(m_lmmTable)
            {
                m_lmmTable->permissive_get<double>("Alpha", 0, alpha, 0.0);
            }


            LT_LOG << "Adding caplet vol quotes:" << endl << "Expiry\tBlack Vol\tFwd Rate" << endl;
            for(size_t k(0); k < m_impliedVolQuotes.size(); ++k)
            {
                expiryDate      = m_impliedVolQuotes[k].getExpiry();
                maturityDate    = m_impliedVolQuotes[k].getMaturity();
                timeToExpiry    = m_impliedVolQuotes[k].getTimeToExpiry();
                timeToMaturity  = m_impliedVolQuotes[k].getTimeToMaturity();

                //  twoQVol = twoQSwaptionCube->getAtmTwoQVolatility(expiryDate, maturityDate);    
                // Note: we could add the time to maturity as well to the quote, but this is not used
                //  capletImpliedVols.add(ImpliedVolQuote(timeToExpiry, twoQVol)); 

                if(m_impliedVolQuotes[k].useAtmStrike())
                { 
					forwardRate = swaptionCube->getATMForwardRate(expiryDate, ModuleDate::getMonthsBetween(expiryDate, maturityDate));
                    blackVol = twoQSwaptionCube->getSwaptionVolatility(expiryDate, maturityDate, forwardRate, alpha);
                    LT_LOG << "fwd rate: " << forwardRate << "\t black vol1:" << blackVol << endl;
                    
                    blackVol = twoQSwaptionCube->getAtmSwaptionVolatility(expiryDate, maturityDate, alpha); 
                    LT_LOG << "black vol2:" << blackVol << endl;
                }
                else
                {
                    blackVol = twoQSwaptionCube->getSwaptionVolatility(expiryDate, 
                                                                       maturityDate, 
                                                                       m_impliedVolQuotes[k].getStrike(),
                                                                       alpha);
                }

                // capletImpliedVols.add(ImpliedVolQuote(timeToExpiry, blackVol));
                m_impliedVolQuotes[k].setVolatility(blackVol);

                // DEBUG info:
                //forwardRate = (model->getTenorDiscountFactor(timeToExpiry, 0.25) / model->getTenorDiscountFactor(timeToMaturity, 0.25) - 1.0) / ModuleDate::getYearsBetween(expiryDate, maturityDate);
                //LT_LOG << timeToExpiry << "\t" << blackVol << "\t" << forwardRate << endl;
            }
        }
        else
        {
            LT_LOG << "NOT 2Q model - cannot use shift LMM." << endl;

            
            LT_LOG << "Adding caplet vol quotes:" << endl << "Expiry\tBlack Vol\tFwd Rate" << endl;
            for(size_t k(0); k < m_impliedVolQuotes.size(); ++k)
            {
                expiryDate      = m_impliedVolQuotes[k].getExpiry();
                maturityDate    = m_impliedVolQuotes[k].getMaturity();
                timeToExpiry    = m_impliedVolQuotes[k].getTimeToExpiry();
                timeToMaturity  = m_impliedVolQuotes[k].getTimeToMaturity();

                if(m_impliedVolQuotes[k].useAtmStrike())
                { 
                    forwardRate = swaptionCube->getATMForwardRate(expiryDate, ModuleDate::getMonthsBetween(expiryDate, maturityDate));
                    blackVol = swaptionCube->getSwaptionVolatility(expiryDate, maturityDate, forwardRate);
                    LT_LOG << "fwd rate: " << forwardRate << "\t black vol1:" << blackVol << endl;
                }
                else
                {
                    blackVol = swaptionCube->getSwaptionVolatility(expiryDate, maturityDate, m_impliedVolQuotes[k].getStrike());
                }

                // capletImpliedVols.add(ImpliedVolQuote(timeToExpiry, blackVol));
                m_impliedVolQuotes[k].setVolatility(blackVol);

                // DEBUG info:
                //forwardRate = (model->getTenorDiscountFactor(timeToExpiry, 0.25) / model->getTenorDiscountFactor(timeToMaturity, 0.25) - 1.0) / ModuleDate::getYearsBetween(expiryDate, maturityDate);
                //LT_LOG << timeToExpiry << "\t" << blackVol << "\t" << forwardRate << endl;
            }

        }

        // Fits the instantaneous volatility to the implied caplet volatilities (if any)
        if(!m_impliedVolQuotes.empty())
        {
            const double defaultEpsilon(1.e-3);
            double epsilon(defaultEpsilon);
            m_lmmTable->permissive_get<double>("Epsilon", 0, epsilon, defaultEpsilon);

            // Note: could get parameters of the solver used in the calibrator from a table
            m_instantaneousVolCalibrator->calibrate(m_instantaneousVolatility, m_impliedVolQuotes, epsilon);

            // DEBUG: output parameters:
            // LT_LOG << "Instant Vols parameters:" << endl;
           
            /*for(Properties::const_iterator iter(m_instantaneousVolatility->getProperties().begin()); 
                iter != m_instantaneousVolatility->getProperties().end(); ++iter)
            {
                LT_LOG << iter->first << ":\t" << boost::get<double>(iter->second.getValue()) << endl;
            }*/
            
            /*string props[] = { string("a"), string("b"), string("c"), string("d") }; 
            Properties properties = m_instantaneousVolatility->getProperties();
            for(size_t cnt(0); cnt <4; ++cnt)
            {
                LT_LOG << props[cnt] << "\t" << boost::get<double>(properties[props[cnt]].getValue()) << endl;
            }*/
           
        }   

        // Set LIBOR Market Model
        m_lmm = LiborMarketModelFactory::create(m_lmmTable, 
                                                m_valueDate, 
                                                getTenorStructure(), 
                                                m_instantaneousVolatility, 
                                                m_correlationStructure);
        
        // DEBUG: volatility structure:
        LT_LOG << "Volatility Structure:\r\nTtExpiry\tfwd fwd vol\tmodel vol" << endl;
        for(size_t k(0); k < m_impliedVolQuotes.size(); ++k)
        {
            const ImpliedVolQuote& volQuote(m_impliedVolQuotes[k]);
            timeToExpiry = volQuote.getTimeToExpiry();
            LT_LOG << timeToExpiry << "\t" << volQuote.getVolatility() << "\t" 
                << sqrt(m_lmm->getIntegralSigmaSquared(0, timeToExpiry, timeToExpiry) / timeToExpiry) << endl;
        }
    }

    // Hardcoded linear interpolation
    double FuturesConvexityModel::computeConvexitySpread(const BaseModelPtr& model, 
                                                         const double timeToExpiry) const
    {
        // Need to do some checking and decide what to do for extrapolation
        // Factor the following for accumulateGradient
        const ForwardRateVector::const_iterator upper(upper_bound(m_forwardRates.begin(), 
                                                                  m_forwardRates.end(), 
                                                                  timeToExpiry, 
                                                                  ::ForwardRateExpiryCompare()));
        if(upper == m_forwardRates.begin())
        {
            // straight line left extrapolation from (0,0)
            return (max(0.0, timeToExpiry) / m_forwardRates[0]->getTimeToExpiry()) * computeConvexitySpreadGivenForwardIndex(model, 0);
        }
        else if(upper == m_forwardRates.end())
        {
              LT_THROW_ERROR("Invalid forward rate iterator in computeConvexitySpreadGradient.");
            // not ideal
            // return (timeToExpiry - prevTimeToExpiry) * prevSpread;
        }

          
        const ForwardRateVector::const_iterator lower(upper - 1);
        const double prevTimeToExpiry((*lower)->getTimeToExpiry()); 
        const double prevSpread(computeConvexitySpreadGivenForwardIndex(model, lower - m_forwardRates.begin()));
        // if the previous stored time to expiry is close enough to the one for which
        //  we want to compute the convexity spread, return its spread directly.
        // It is expected this should be the case most of the time as the tenor structure
        //  matches the time to expiry of the futures
        if(abs(timeToExpiry - prevTimeToExpiry) < m_timeEpsilon)
        {
            return prevSpread;
        }
        
        // otherwise linear interpolation
        const double nextTimeToExpiry((*upper)->getTimeToExpiry()); 
        const double nextSpread(computeConvexitySpreadGivenForwardIndex(model, upper - m_forwardRates.begin()));
            
        // LT_LOG << "expiry: " << prevTime << ", Cvx spread: " << prevSpread << endl;

        // Linear interpolation between the two forward rates
        return prevSpread + (nextSpread - prevSpread) * (timeToExpiry - prevTimeToExpiry) / (nextTimeToExpiry - prevTimeToExpiry);
    
    }

    // Hardcoded linear interpolation
    void FuturesConvexityModel::accumulateConvexitySpreadGradient(const BaseModelPtr& model, 
                                                                  const double multiplier,
                                                                  const GradientIterator gradientBegin,
                                                                  const GradientIterator gradientEnd,
                                                                  const double timeToExpiry) const
    {
        // Some of the following should be factored with computeConvexitySpread
        const ForwardRateVector::const_iterator upper(upper_bound(m_forwardRates.begin(), 
                                                                  m_forwardRates.end(),
                                                                  timeToExpiry, 
                                                                  ::ForwardRateExpiryCompare()));
    
        if(upper == m_forwardRates.begin() && timeToExpiry > 0.0)
        {
            accumulateConvexitySpreadGradientGivenForwardIndex(model, 
                                                               multiplier * timeToExpiry / m_forwardRates[0]->getTimeToExpiry(),
                                                               gradientBegin,
                                                               gradientEnd,
                                                               0);
        }   
        else if(upper == m_forwardRates.end())
        {
            LT_THROW_ERROR("Invalid forward rate iterator in accumulateConvexitySpreadGradient.");
        }
    
        const ForwardRateVector::const_iterator lower(upper - 1);    
        const double prevTimeToExpiry((*lower)->getTimeToExpiry()); 

        // if the previous stored time to expiry is close enough to the one for which
        //  we want to compute the convexity spread, return its spread directly.
        // It is expected this should be the case most of the time as the tenor structure
        //  matches the time to expiry of the futures
        if(abs(timeToExpiry - prevTimeToExpiry) < m_timeEpsilon)
        {
            // Accumulate sensitivity to the previous knot-point
            accumulateConvexitySpreadGradientGivenForwardIndex(model, multiplier, gradientBegin, gradientEnd, lower - m_forwardRates.begin());
            return;
        }

        // otherwise compute the gradient for a linear interpolation
        const double nextTimeToExpiry((*upper)->getTimeToExpiry()); 
        const double lambda((timeToExpiry - prevTimeToExpiry) / (nextTimeToExpiry - prevTimeToExpiry));

        // Accumulate sensitivity to the previous knot-point
        accumulateConvexitySpreadGradientGivenForwardIndex(model, multiplier * (1.0 - lambda), gradientBegin, gradientEnd, lower - m_forwardRates.begin());

        // Accumulate sensitivity to the next knot-point
        accumulateConvexitySpreadGradientGivenForwardIndex(model, multiplier * lambda, gradientBegin, gradientEnd, upper - m_forwardRates.begin());
    
        /*
        // For Debug only: output non-zero gradient coordinates
        int index;
        LT_LOG << "Gradient @ " << timeToExpiry << " w/ format: '(index, non-0 val)'" << endl;
        for(GradientIterator iter(gradientBegin); iter != gradientEnd; ++iter)
        {
            if((*iter) != 0.0)
            {
                index = iter - gradientBegin;
                LT_LOG << "(" << index << ", " << (*iter) << ")  ";
            }
        }
        LT_LOG << endl;
        */
    }

    vector<LT::date> FuturesConvexityModel::getTenorStructure() const
    {
        /*vector<date> tenorStructure(m_startDates);
        if(!m_endDates.empty())
        {
            tenorStructure.push_back(m_endDates[m_endDates.size() - 1]);
        }
        return tenorStructure;
        */
        vector<LT::date> tenorStructure;
        return tenorStructure;
    }

    /* True but not used for now
    double FuturesConvexityModel:computeImpliedLiborGivenForwardIndex(const BaseModelPTr& model,
                                                                      const size_t index) const
    {
          return m_forwardRates[index]->getValue(model) * exp(computeSumUpToForwardIndex(model, index);
    }
    */ 

    double FuturesConvexityModel::computeConvexitySpreadGivenForwardIndex(const BaseModelPtr& model,
                                                                          const size_t index) const
    {
        checkIndex(index);
        return m_forwardRates[index]->getValue(*model) * (exp(computeSumUpToForwardIndex(model, index)) - 1.0);
    }

    /// The partial derivative of the spread S relative to an unknown K is:
    /// S'(K)  =  (exp[sum(K)] - 1) * F'(K)  +  iL(K) * sum'(K)
    /// where:
    /// - F  is the forward of the specified index
    /// - sum is the sum up to F position (inc.) in the tenor structure    
    /// - iL is the implied LIBOR, equal to 1 - Futures price / 100
    void FuturesConvexityModel::accumulateConvexitySpreadGradientGivenForwardIndex(const BaseModelPtr& model, 
                                                                                   const double multiplier,
                                                                                   const GradientIterator gradientBegin,
                                                                                   const GradientIterator gradientEnd,
                                                                                   const size_t index) const
   {
        checkIndex(index);

        double coef(exp(computeSumUpToForwardIndex(model, index)) - 1.0);
        m_forwardRates[index]->accumulateGradient(*model, multiplier * coef, gradientBegin, gradientEnd);

        coef = (1.0 + coef) * m_forwardRates[index]->getValue(*model); // computes implied LIBOR without recalculating the sum
        accumulateSumGradientUpToForwardIndex(model, multiplier * coef, gradientBegin, gradientEnd, index);
    }

    // Computes the sum from 0 to the specified forward index included of the quantities as in the function
    double FuturesConvexityModel::computeSumUpToForwardIndex(const BaseModelPtr& model, const size_t upperIndex) const
    {
        checkIndex(upperIndex);

        double coverageTimesForwardRate;
        double tj(0.0);
        double sum(0.0);

        for(size_t index(0); index <= upperIndex; ++index)
        {
            coverageTimesForwardRate = m_forwardRates[index]->getCoverage() * m_forwardRates[index]->getValue(*model);
            tj = m_forwardRates[index]->getTimeToExpiry();

            sum += (m_lmm->getIntegralSigmaSigmaRho(0.0, m_forwardRates[index]->getTimeToExpiry(), m_forwardRates[upperIndex]->getTimeToExpiry(), tj) 
                * coverageTimesForwardRate /  (1.0 + coverageTimesForwardRate));
        
            
        }

        LT_LOG << "exp-sum[" << upperIndex << "]\t=" << sum << std::endl;

        return sum;
    }

    // Computes the gradient of the sum computed in computeSumUpToForwardIndex
    void FuturesConvexityModel::accumulateSumGradientUpToForwardIndex(const BaseModelPtr& model, 
                                                                      const double multiplier,
                                                                      const GradientIterator gradientBegin,
                                                                      const GradientIterator gradientEnd,
                                                                      const size_t lastForwardIndex) const
    {
        checkIndex(lastForwardIndex);

        double coverageTimesForwardRate, coef;
        double tj(0.0);

        for(size_t index(0); index <= lastForwardIndex; ++index)
        {
            coverageTimesForwardRate = m_forwardRates[index]->getCoverage() * m_forwardRates[index]->getValue(*model);
            tj = m_forwardRates[index]->getTimeToExpiry();

            coef = m_lmm->getIntegralSigmaSigmaRho(0.0, m_forwardRates[index]->getTimeToExpiry(), m_forwardRates[lastForwardIndex]->getTimeToExpiry(), tj)
                * m_forwardRates[index]->getCoverage() / (coverageTimesForwardRate * coverageTimesForwardRate);
            
            m_forwardRates[index]->accumulateGradient(*model, multiplier * coef, gradientBegin, gradientEnd);
        }
    }

    void FuturesConvexityModel::checkIndex(const size_t index) const
    {
        if(index < 0 || index >= m_forwardRates.size())
        {
            LT_THROW_ERROR("Invalid index");
        }
    }

}
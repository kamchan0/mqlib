#include "stdafx.h"
#include "SwapMarketSolverCalculator.h"
#include "FunctorImpl.h"
#include "Dictionaries.h"
#include "Queries.h"

#include "Data/ConvertGenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"

#include "StructureInstrumentUtils.h"
#include "EquivalentCurveCalculator.h"
#include "EquivalentCurveFunctionsFactory.h"
#include "tktable/tktable.h"
#include "DataExtraction.h"

#include "Exception.h"
// #include "UtilsEnums.h"
#include "YieldCurve.h"
#include "FunctorEnvelope.h"
#include "FactoryEnvelope.h"
#include "DictionaryItem.h"
#include "IDeASystem.h"
#include "Packages.h"
#include "BaseModel.h"

#include "TradeConventionCalcH.h"
#include "FixedLeg.h"

using namespace LTQC;
using namespace std;

namespace IDeA
{
	
	SwapMarketSolverCalculator::SwapMarketSolverCalculator(const LT::TablePtr& ycTable, const LT::TablePtr& instrumentsTable, const LT::TablePtr& basisSwapsTable, FlexYCF::BaseModelPtr model)
            : m_yieldCurveTable(ycTable), m_swapTable(instrumentsTable), m_basisSwapTable(basisSwapsTable), tenor1m(0,0,0,1,0), tenor3m(0,0,0,3,0), tenor6m(0,0,0,6,0)
        {
            const LT::TablePtr parametersTable(IDeA::extract<LT::TablePtr>(*m_yieldCurveTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
            IDeA::IRCurveMktConventionPtr conventions(FlexYCF::createIRCurveMktConventions(parametersTable));				
		    IDeA::SwapMktConvention& swapConventions = conventions->m_swaps;
            m_swapTenor = swapConventions.m_floatFrequency;
            m_1mSwapFrequency = m_swapTenor == tenor1m;
			m_3mSwapFrequency = m_swapTenor == tenor3m;
            m_6mSwapFrequency = m_swapTenor == tenor6m;
			
					
            if( basisSwapsTable )
            {
				if( !m_3mSwapFrequency && !m_6mSwapFrequency )
				{
					LTQC_THROW(DataException, "SwapMarketSolver: one of the  basis swaps tenors has to match swap floating leg tenor, only 3m or 6m allowed: " << m_swapTenor.asTenorString().data());
				}

                size_t k = basisSwapsTable->rowsGet()-1;
                if( k > 2 )
                {
                    LTQC_THROW(DataException, "SwapMarketSolver: only 3m6m and 1m3m basis swaps are permited");
                }
				for(size_t i=0; i<k; ++i)
				{
					LT::Str tenor1Str = IDeA::extract<LT::Str>(*basisSwapsTable, IDeA_KEY(BASISSWAPSPORTFOLIO, FLOATTENOR1), i);
					LT::Str tenor2Str = IDeA::extract<LT::Str>(*basisSwapsTable, IDeA_KEY(BASISSWAPSPORTFOLIO, FLOATTENOR2), i);
					LT::Str spreadTenorStr = IDeA::extract<LT::Str>(*basisSwapsTable, IDeA_KEY(BASISSWAPSPORTFOLIO, SPREADTENOR), i);
					m_basisSwapTable = IDeA::extract<LT::TablePtr>(*basisSwapsTable, IDeA_KEY(BASISSWAPSPORTFOLIO, BASISSWAPS), i);

					
					Tenor tenor1(tenor1Str), tenor2(tenor2Str);
					if(( tenor1 == tenor3m && tenor2 == tenor6m) || ( tenor1 == tenor6m && tenor2 == tenor3m ))
					{
						m_3m6mBasisSwapTable = IDeA::extract<LT::TablePtr>(*basisSwapsTable, IDeA_KEY(BASISSWAPSPORTFOLIO, BASISSWAPS), i);
						m_3m6mSpreadFrequency = Tenor(spreadTenorStr);
					}
					else if(( tenor1 == tenor1m && tenor2 == tenor3m) || ( tenor1 == tenor3m && tenor2 == tenor1m ))
					{
						m_1m3mBasisSwapTable = IDeA::extract<LT::TablePtr>(*basisSwapsTable, IDeA_KEY(BASISSWAPSPORTFOLIO, BASISSWAPS), i);
						m_1m3mSpreadFrequency = Tenor(spreadTenorStr);
					}
					else
					{
						LTQC_THROW(DataException, "SwapMarketSolver: only 3m6m and 1m3m basis swaps are permited");
					}	
				}
            }
         
            extractTenorsAndRates();
            generateTenors();
            calculateNumberOfVariables(); 
            extractWeights();
            populateAnnuity(model);
            initializeMatrix();
            populateMarketQuotes();
        }
    
    void SwapMarketSolverCalculator::calculateNumberOfVariables() 
    {
        set<Tenor> allSwapTenors;
        allSwapTenors.insert(m_swapTenors.begin(),m_swapTenors.end());
        allSwapTenors.insert(m_swapInterpolatedTenors.begin(),m_swapInterpolatedTenors.end());
        m_noSwaps = allSwapTenors.size();

        allSwapTenors.clear();
        allSwapTenors.insert(m_3m6mBasisSwapTenors.begin(),m_3m6mBasisSwapTenors.end());
        allSwapTenors.insert(m_3m6mBasisSwapInterpolatedTenors.begin(),m_3m6mBasisSwapInterpolatedTenors.end());
        m_no3m6mSwaps = allSwapTenors.size();
		
		allSwapTenors.clear();
        allSwapTenors.insert(m_1m3mBasisSwapTenors.begin(),m_1m3mBasisSwapTenors.end());
        allSwapTenors.insert(m_1m3mBasisSwapInterpolatedTenors.begin(),m_1m3mBasisSwapInterpolatedTenors.end());
        m_no1m3mSwaps = allSwapTenors.size();
        
		m_noConstrains = m_swapsPortfolio.size() + m_swapInterpolatedTenors.size() + m_3m6mBasisSwaps.size() + m_3m6mBasisSwapInterpolatedTenors.size() + m_1m3mBasisSwaps.size() + m_1m3mBasisSwapInterpolatedTenors.size();
    }

    void SwapMarketSolverCalculator::extractTenorsAndRates()
    {
        extractSwapTenorsAndRates(m_swapTable);
        extractBasisSwapsTenorsAndRates(m_3m6mBasisSwapTable, m_3m6mWeights, m_3m6mBasisSwaps);
		extractBasisSwapsTenorsAndRates(m_1m3mBasisSwapTable, m_1m3mWeights, m_1m3mBasisSwaps);
    }
    
    void SwapMarketSolverCalculator::extractWeights()
    {
        m_weights.resize(numberOfConstrains());

        size_t offset = numberOfSwapConstrains();
        for(size_t i = 0; i < numberOfInterpolatedSwapConstrains(); ++i)
        {
            m_weights[i + offset] = 1.0;
        }

        offset += numberOfInterpolatedSwapConstrains();
        for(size_t i = 0; i <  numberOf3m6mBasisSwapConstrains(); ++i)
        {
            m_weights[i + offset] = m_3m6mWeights[i];
        }

        offset += numberOf3m6mBasisSwapConstrains();
        for(size_t i = 0; i < numberOf3m6mInterpolatedBasisSwapConstrains(); ++i)
        {
            m_weights[i + offset] = 1.0;
        }
		
		offset += numberOf3m6mInterpolatedBasisSwapConstrains();
        for(size_t i = 0; i <  numberOf1m3mBasisSwapConstrains(); ++i)
        {
            m_weights[i + offset] = m_1m3mWeights[i];
        }

        offset += numberOf1m3mBasisSwapConstrains();
        for(size_t i = 0; i < numberOf1m3mInterpolatedBasisSwapConstrains(); ++i)
        {
            m_weights[i + offset] = 1.0;
        }
    }

    void SwapMarketSolverCalculator::extractSwapTenorsAndRates(LT::TablePtr instrumentTable)
    {
        size_t k = instrumentTable->rowsGet()-1;
        m_weights.resize(k);
        
        for(size_t i = 0; i < k; ++i)
        {
            LT::Str tenorDesc = IDeA::extract<LT::Str>(*instrumentTable, IDeA_KEY(SWAPSPORTFOLIO, TENOR), i);
            double rate = IDeA::extract<double>(*instrumentTable, IDeA_KEY(SWAPSPORTFOLIO, RATE), i);
            m_swapsPortfolio.push_back( pair<string,double>(tenorDesc.string(),rate) );

            double weight = 1.0;
            IDeA::permissive_extract<double>(*instrumentTable, IDeA_KEY(SWAPSPORTFOLIO, WEIGHT), i, weight, weight);
            m_weights[i] = weight;

            LT::Str fixedLegTenor;
            IDeA::permissive_extract<LT::Str>(*instrumentTable, IDeA_KEY(SWAPSPORTFOLIO, FIXEDTENOR), i, fixedLegTenor, LT::Str("") );
            if( i > 0 && fixedLegTenor != m_fixedLegTenor )
            {
                LTQC_THROW(DataException, "SwapMarketSolver: fixed leg tenors have to be the same for all products, user input " << m_fixedLegTenor.string() << " and " << fixedLegTenor.string());   
            }
            if( i == 0)
            {
                m_fixedLegTenor = fixedLegTenor;
            }

            LT::Str fixedLegBasis;
            IDeA::permissive_extract<LT::Str>(*instrumentTable, IDeA_KEY(SWAPSPORTFOLIO, FIXEDBASIS), i, fixedLegBasis, LT::Str(""));
            if( i > 0 && fixedLegBasis != m_fixedLegBasis )
            {
                LTQC_THROW(DataException, "SwapMarketSolver: fixed leg tenors have to be the same for all products, user input " << m_fixedLegBasis.string() << " and " << fixedLegBasis.string());   
            }
            if( i == 0)
            {
                m_fixedLegBasis = fixedLegBasis;
            }
            
            LT::Str fixedLegCalendar;
            IDeA::permissive_extract<LT::Str>(*instrumentTable, IDeA_KEY(SWAPSPORTFOLIO, ACCRUALCALENDAR), i, fixedLegCalendar, LT::Str(""));
            if( i > 0 && fixedLegCalendar != m_fixedLegCalendar )
            {
                LTQC_THROW(DataException, "SwapMarketSolver: fixed leg tenors have to be the same for all products, user input " << m_fixedLegCalendar.string() << " and " << fixedLegCalendar.string());   
            }
            if( i == 0)
            {
                m_fixedLegCalendar = fixedLegCalendar;
            }
        }

    }

    void SwapMarketSolverCalculator::extractBasisSwapsTenorsAndRates(LT::TablePtr basisSwaps, LTQC::VectorDouble& w, vector< pair<string,double> >& swaps)
    {
        if(!basisSwaps)
            return;
        
        if( basisSwaps )
        {
            size_t k = basisSwaps->rowsGet()-1;
            w.resize(k);
        
            for(size_t i = 0; i < k; ++i)
            {
                LT::Str tenorDesc = IDeA::extract<LT::Str>(*basisSwaps, IDeA_KEY(BASISSWAPS, TENOR), i);
                double rate = IDeA::extract<double>(*basisSwaps, IDeA_KEY(BASISSWAPS, RATE), i);
                swaps.push_back( pair<string,double>(tenorDesc.string(),rate) );

                double weight = 1.0;
                IDeA::permissive_extract<double>(*basisSwaps, IDeA_KEY(BASISSWAPS, WEIGHT), i, weight, weight);
                w[i] = weight;   
            }
        }
    }

   
    
    void SwapMarketSolverCalculator::populateMarketQuotes()
    {
        m_marketQuotes.resize(numberOfConstrains());
        for(size_t i = 0; i < m_swapsPortfolio.size(); ++i)
        {
            const string& description = m_swapsPortfolio[i].first;
            const double& rate = m_swapsPortfolio[i].second;
            
            switch( toIRSwapProductType(description) )
            {
                case IRSwapProductType::Swap:
                {
                    string fwdTenor;
                    vector<string> tenors;
                    parseDescription(description, tenors, fwdTenor);
                    Tenor fwd(fwdTenor);
                    Tenor tenor(tenors[0]);
                    if( fwd.asYearFraction() > 0.0 )
                    {
                        m_marketQuotes[i] = rate * (annuity(tenor + fwd) - annuity(fwd));
                    }
                    else
                    {
                        m_marketQuotes[i] = rate * annuity(tenor);
                    }
                }
                break;
            
                case IRSwapProductType::Spread:
                case IRSwapProductType::Fly:
                    
                    m_marketQuotes[i] = rate;
                break;

                default:
                    LTQC_THROW(IDeA::MarketException, "Unable to swap product type from description: " << description);   
            }  
        }

        size_t offset = m_swapsPortfolio.size();
        for(size_t i = 0; i < m_swapInterpolatedTenors.size(); ++i)
        {
            m_marketQuotes[i + offset] = 0.0;
        }

        offset +=  m_swapInterpolatedTenors.size();
        for(size_t i = 0; i < m_3m6mBasisSwaps.size(); ++i)
        {
            const string& description = m_3m6mBasisSwaps[i].first;
            const double& rate = m_3m6mBasisSwaps[i].second;
            
            switch( toIRSwapProductType(description) )
            {
                case IRSwapProductType::Swap:
                {
                    string fwdTenor;
                    vector<string> tenors;
                    parseDescription(description, tenors, fwdTenor);
                    Tenor fwd(fwdTenor);
                    Tenor tenor(tenors[0]);
                    if( fwd.asYearFraction() > 0.0 )
                    {
                        if(m_3mSwapFrequency)
                        {
                            m_marketQuotes[i + offset] = - rate * (annuityBasis(tenor + fwd, m_3m6mSpreadFrequency) - annuityBasis(fwd, m_3m6mSpreadFrequency));
                        }
                        else
                        {
                            m_marketQuotes[i + offset] = rate * (annuityBasis(tenor + fwd, m_3m6mSpreadFrequency) - annuityBasis(fwd, m_3m6mSpreadFrequency));
                        }
                    }
                    else
                    {
                        if(m_3mSwapFrequency)
                        {
                            m_marketQuotes[i + offset] = - rate * annuityBasis(tenor, m_3m6mSpreadFrequency);
                        }
                        else
                        {
                            m_marketQuotes[i + offset] = rate * annuityBasis(tenor, m_3m6mSpreadFrequency);
                        }
                    }
                }
                break;
            
                case IRSwapProductType::Spread:
                case IRSwapProductType::Fly:
                     if(m_3mSwapFrequency)
                     {
                        m_marketQuotes[i + offset] = - rate;
                     }
                     else
                     {
                        m_marketQuotes[i + offset] = - rate;
                     }
                break;

                default:
                    LTQC_THROW(IDeA::MarketException, "Unable to swap product type from description: " << description);   
            }  
        }
        
        offset += m_3m6mBasisSwaps.size();
        for(size_t i = 0; i < m_3m6mBasisSwapInterpolatedTenors.size(); ++i)
        {
            m_marketQuotes[i + offset] = 0.0;
        }


		offset +=  m_3m6mBasisSwapInterpolatedTenors.size();
        for(size_t i = 0; i < m_1m3mBasisSwaps.size(); ++i)
        {
            const string& description = m_1m3mBasisSwaps[i].first;
            const double& rate = m_1m3mBasisSwaps[i].second;
            
            switch( toIRSwapProductType(description) )
            {
                case IRSwapProductType::Swap:
                {
                    string fwdTenor;
                    vector<string> tenors;
                    parseDescription(description, tenors, fwdTenor);
                    Tenor fwd(fwdTenor);
                    Tenor tenor(tenors[0]);
                    if( fwd.asYearFraction() > 0.0 )
                    {
                        if(m_1mSwapFrequency)
                        {
                            m_marketQuotes[i + offset] = - rate * (annuityBasis(tenor + fwd, m_1m3mSpreadFrequency) - annuityBasis(fwd, m_1m3mSpreadFrequency));
                        }
                        else
                        {
                            m_marketQuotes[i + offset] = rate * (annuityBasis(tenor + fwd, m_1m3mSpreadFrequency) - annuityBasis(fwd, m_1m3mSpreadFrequency));
                        }
                    }
                    else
                    {
                        if(m_1mSwapFrequency)
                        {
                            m_marketQuotes[i + offset] = - rate * annuityBasis(tenor, m_1m3mSpreadFrequency);
                        }
                        else
                        {
                            m_marketQuotes[i + offset] = rate * annuityBasis(tenor, m_1m3mSpreadFrequency);
                        }
                    }
                }
                break;
            
                case IRSwapProductType::Spread:
                case IRSwapProductType::Fly:
                     if(m_1mSwapFrequency)
                     {
                        m_marketQuotes[i + offset] = - rate;
                     }
                     else
                     {
                        m_marketQuotes[i + offset] = - rate;
                     }
                break;

                default:
                    LTQC_THROW(IDeA::MarketException, "Unable to swap product type from description: " << description);   
            }  
        }
        
        offset += m_1m3mBasisSwaps.size();
        for(size_t i = 0; i < m_1m3mBasisSwapInterpolatedTenors.size(); ++i)
        {
            m_marketQuotes[i + offset] = 0.0;
        }
    }

    size_t SwapMarketSolverCalculator::annuityIndex(const Tenor& tenor) const
    {
        return static_cast<size_t>(tenor.getYears()) * m_multiplier;
    }
    
    size_t SwapMarketSolverCalculator::annuityIndexBasis(const Tenor& tenor, const Tenor& basisTenor) const
    {
        const size_t& m = (*(m_basisMultiplier.find(basisTenor))).second;
        return static_cast<size_t>(tenor.getYears()) * m;
    }
    
    void SwapMarketSolverCalculator::populateAnnuity(FlexYCF::BaseModelPtr model)
    { 
        const LT::TablePtr detailsTable(IDeA::extract<LT::TablePtr>(*m_yieldCurveTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        const LT::TablePtr parametersTable(IDeA::extract<LT::TablePtr>(*m_yieldCurveTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		
        IDeA::IRCurveMktConventionPtr conventions(FlexYCF::createIRCurveMktConventions(parametersTable));				
		IDeA::SwapMktConvention& swapConventions = conventions->m_swaps;
        Tenor maxTenor = m_swapTenors.back();
        if( !m_swapInterpolatedTenors.empty() )
        {
            maxTenor = max(m_swapInterpolatedTenors.back(), maxTenor);
        }
        std::string maturity = maxTenor.asString().string();
		
        Tenor fixedFrequency(m_fixedLegTenor.empty() ? swapConventions.m_fixedFrequency.asString().string()  : m_fixedLegTenor.string());
	    string fixedBasis(m_fixedLegBasis.empty() ? swapConventions.m_fixedAccrualBasis.asString().data() : m_fixedLegBasis.string());
        string fixedCalendar(m_fixedLegCalendar.empty() ? swapConventions.m_fixedAccrualCalendar.string() : m_fixedLegCalendar.string());
        
        LT::date startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(valueDate), swapConventions.m_depRateMktConvention.m_fixingCalendar, swapConventions.m_depRateMktConvention.m_accrualValueCalendar, swapConventions.m_spotDays).getAsLTdate();
        LT::date endDate   = ModuleDate::addDatePeriod(startDate, maturity, ModuleDate::CalendarFactory::create(swapConventions.m_depRateMktConvention.m_accrualValueCalendar.string()));
        
        const FlexYCF::InstrumentComponent::CacheScopeSwitcher switcher(false);
        FlexYCF::FixedLegPtr fixedLeg(FlexYCF::FixedLeg::create(FlexYCF::FixedLeg::Arguments(valueDate, valueDate,startDate,endDate,fixedFrequency.asString().string(),fixedBasis,fixedCalendar )));

        m_annuity = fixedLeg->getAnnuity(*model);
        m_multiplier = static_cast<size_t>( 12/fixedFrequency.asMonths() );
        m_fixedLegTenor = fixedFrequency.asString();
        m_fixedLegBasis = LT::Str(fixedBasis);
        m_fixedLegCalendar = LT::Str(fixedCalendar);
        
        if(m_3m6mBasisSwapTable)
        {
            generateAnnuity(valueDate, startDate, endDate, m_3m6mSpreadFrequency, swapConventions.m_floatAccrualBasis.asString().data(), swapConventions.m_floatAccrualCalendar.string(), model);
            m_basisMultiplier[m_3m6mSpreadFrequency] = static_cast<size_t>( 12/m_3m6mSpreadFrequency.asMonths() );
		}

		if(m_1m3mBasisSwapTable)
		{
			generateAnnuity(valueDate, startDate, endDate, m_1m3mSpreadFrequency, swapConventions.m_floatAccrualBasis.asString().data(), swapConventions.m_floatAccrualCalendar.string(), model);
            m_basisMultiplier[m_1m3mSpreadFrequency] = static_cast<size_t>( 12/m_1m3mSpreadFrequency.asMonths() );
        }
    }
   
    void SwapMarketSolverCalculator::generateAnnuity(LT::date valueDate, LT::date startDate, LT::date endDate, const Tenor& frequency, const string& basis, const string& calendar, FlexYCF::BaseModelPtr model)
    {
        const FlexYCF::InstrumentComponent::CacheScopeSwitcher switcher(false);
        FlexYCF::FixedLegPtr fixedLeg(FlexYCF::FixedLeg::create(FlexYCF::FixedLeg::Arguments(valueDate, valueDate, startDate, endDate, frequency.asString().string(), basis, calendar )));

        m_annuityMap[frequency] = fixedLeg->getAnnuity(*model);
    }

    double SwapMarketSolverCalculator::annuity(const Tenor& tenor) const
    {
        return m_annuity[ annuityIndex(tenor) ].second;
    }

    double SwapMarketSolverCalculator::annuity(const Tenor& tenor1, const Tenor& tenor2) const
    {
        return annuity(tenor2) - annuity(tenor1);
    }
     
    double SwapMarketSolverCalculator::annuityBasis(const Tenor& tenor, const Tenor& basisTenor) const
    {
        const vector< pair<LT::date, double> >& m = (*(m_annuityMap.find(basisTenor))).second;
        return m[ annuityIndexBasis(tenor,basisTenor) ].second;
    }

    double SwapMarketSolverCalculator::annuityBasis(const Tenor& tenor1, const Tenor& tenor2, const Tenor& basisTenor) const
    {
        return annuity(tenor2, basisTenor) - annuity(tenor1,basisTenor);
    }
    
    size_t SwapMarketSolverCalculator::swapIndex(const Tenor& tenor) const
    {
        vector<Tenor>::const_iterator it = find( m_swapTenors.begin(), m_swapTenors.end(), tenor );
        size_t j = 0;
        if( it == m_swapTenors.end() )
        {
            vector<Tenor>::const_iterator it = find(m_swapInterpolatedTenors.begin(), m_swapInterpolatedTenors.end(), tenor);
            if( it == m_swapInterpolatedTenors.end())
            {
                LTQC_THROW(IDeA::MarketException, "Unable to find swap tenor "  << tenor.asTenorString().data() );
            }
            j = static_cast<size_t>( it - m_swapInterpolatedTenors.begin() ) + m_swapTenors.size();
        }
        else
        {
            j = static_cast<size_t>( it - m_swapTenors.begin() );
        }
        return j;
    }

    size_t SwapMarketSolverCalculator::basisSwapIndex3m6m(const Tenor& tenor) 
    { 
        const vector<Tenor>& basisTenors = m_3m6mBasisSwapTenors;
        vector<Tenor>::const_iterator it = find( basisTenors.begin(), basisTenors.end(), tenor );
        size_t j = 0;
        if( it == basisTenors.end())
        {
            vector<Tenor>::const_iterator it = find(m_3m6mBasisSwapInterpolatedTenors.begin(), m_3m6mBasisSwapInterpolatedTenors.end(), tenor);
            if( it == m_3m6mBasisSwapInterpolatedTenors.end())
            {
                LTQC_THROW(IDeA::MarketException, "Unable to find 3m6m basis swap tenor "  << tenor.asTenorString().data() );
            }
            j = static_cast<size_t>( it - m_3m6mBasisSwapInterpolatedTenors.begin() ) + basisTenors.size();
        }
        j = static_cast<size_t>( it - basisTenors.begin() );
        return j + numberOfAllSwaps(); 
    }
	
	size_t SwapMarketSolverCalculator::basisSwapIndex1m3m(const Tenor& tenor) 
    { 
        const vector<Tenor>& basisTenors = m_1m3mBasisSwapTenors;
        vector<Tenor>::const_iterator it = find( basisTenors.begin(), basisTenors.end(), tenor );
        size_t j = 0;
        if( it == basisTenors.end())
        {
            vector<Tenor>::const_iterator it = find(m_1m3mBasisSwapInterpolatedTenors.begin(), m_1m3mBasisSwapInterpolatedTenors.end(), tenor);
            if( it == m_1m3mBasisSwapInterpolatedTenors.end())
            {
                LTQC_THROW(IDeA::MarketException, "Unable to 1m3m find basis swap tenor "  << tenor.asTenorString().data() );
            }
            j = static_cast<size_t>( it - m_1m3mBasisSwapInterpolatedTenors.begin() ) + basisTenors.size();
        }
        j = static_cast<size_t>( it - basisTenors.begin() );
        return j + numberOfAllSwaps() + numberOfAll3m6mBasisSwaps(); 
    }

    void SwapMarketSolverCalculator::initializeMatrix()
    {
        const size_t noRows  = numberOfConstrains();
        const size_t noCols  = numberOfVariables();
        
        LTQC::Matrix matrix( noRows , noCols, 0.0 );
        const vector<Tenor>& swapTenors = m_swapTenors;

        size_t noSpotOrFwdSwap = 0;
        for(size_t i = 0; i < m_swapsPortfolio.size(); ++i )
        {   
            string fwdTenor;
            vector<string> tenors;
            parseDescription(m_swapsPortfolio[i].first, tenors, fwdTenor);
            Tenor fwd(fwdTenor);
            
            // swap
            if(tenors.size() == 1)
            {
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    matrix.at(i,swapIndex(Tenor(tenors[0]) + fwd)) = 1.0; 
                    matrix.at(i,swapIndex(fwd)) = -1.0; 
                }
                // spot starting
                else
                {
                    matrix.at(i,swapIndex(Tenor(tenors[0]))) = 1.0;   
                }
                ++noSpotOrFwdSwap;
            }
            // spread
            else if(tenors.size() == 2)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                bool tenor1Short = tenor1 < tenor2;

                const Tenor shortTenor = tenor1Short ? tenor1 : tenor2;
                const Tenor longTenor  = tenor1Short ? tenor2 : tenor1;
                
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuity1 = annuity(fwd, shortTenor + fwd);
                    double annuity2 = annuity(fwd, longTenor + fwd);

                    matrix.at(i,swapIndex(shortTenor + fwd)) = -1.0/annuity1; 
                    matrix.at(i,swapIndex(longTenor + fwd)) = 1.0/annuity2;
                    matrix.at(i,swapIndex(fwd)) = 1.0/annuity1 - 1.0/annuity2; 
                }
                // spot starting
                else
                {
                    double annuity1 = annuity(shortTenor);
                    double annuity2 = annuity(longTenor);

                    matrix.at(i,swapIndex(shortTenor)) = -1.0/annuity1; 
                    matrix.at(i,swapIndex(longTenor)) = 1.0/annuity2;   
                }
            }
            // fly
            else if(tenors.size() == 3)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                Tenor tenor3 = Tenor(tenors[2]);
                
                if( !(tenor1 < tenor2 &&  tenor2 < tenor3) && !(tenor3 < tenor2 &&  tenor2 < tenor1) )
                {
                    LTQC_THROW(IDeA::MarketException, "Unable to deal with swap fly");   
                }
                bool tenor1Short = tenor1 < tenor3;

                const Tenor leftTenor   = tenor1Short ? tenor1 : tenor3;
                const Tenor middleTenor = tenor2;
                const Tenor rightTenor  = tenor1Short ? tenor3 : tenor1;


                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuityLeft = annuity(fwd, leftTenor + fwd);
                    double annuityMiddle = annuity(fwd, middleTenor + fwd);
                    double annuityRight = annuity(fwd, rightTenor + fwd);

                    matrix.at(i,swapIndex(leftTenor + fwd)) = -1.0/annuityLeft; 
                    matrix.at(i,swapIndex(middleTenor + fwd)) = 2.0/annuityMiddle;  
                    matrix.at(i,swapIndex(rightTenor + fwd)) = -1.0/annuityRight; 
                    matrix.at(i,swapIndex(fwd)) = 1.0/annuityLeft + 1.0/annuityRight - 2.0/annuityMiddle; 
                }
                // spot starting
                else
                { 
                    double annuityLeft = annuity(leftTenor);
                    double annuityMiddle = annuity(middleTenor);
                    double annuityRight = annuity(rightTenor);

                    matrix.at(i,swapIndex(leftTenor)) = -1.0/annuityLeft; 
                    matrix.at(i,swapIndex(middleTenor)) = 2.0/annuityMiddle;  
                    matrix.at(i,swapIndex(rightTenor)) = -1.0/annuityRight;   
                }
            }
        }
         
        // interpolated swaps
        for(size_t i = 0; i < m_swapInterpolatedTenors.size(); ++i)
        {
            if(m_swapInterpolatedTenors[i] < m_swapTenors.front() )
            {
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapTenors.front())) = - annuity( m_swapInterpolatedTenors[i] )/annuity( m_swapTenors.front() );
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapInterpolatedTenors[i])) = 1.0;

            }
            else if(m_swapInterpolatedTenors[i] == m_swapTenors.front() )
            {
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(*++m_swapTenors.begin())) = - annuity( m_swapInterpolatedTenors[i] )/annuity( *++m_swapTenors.begin() );
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapInterpolatedTenors[i])) = 1.0;

            }
            else if(m_swapTenors.back() < m_swapInterpolatedTenors[i])
            {
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapTenors.back())) = - annuity( m_swapInterpolatedTenors[i] )/annuity( m_swapTenors.back() );
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapInterpolatedTenors[i])) = 1.0;
            }
            else if(m_swapTenors.back() == m_swapInterpolatedTenors[i])
            {
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(*--m_swapTenors.end())) = - annuity( m_swapInterpolatedTenors[i] )/annuity( *--m_swapTenors.end() );
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapInterpolatedTenors[i])) = 1.0;
            }
            else 
            {
                vector<Tenor>::const_iterator rightBound = lower_bound(m_swapTenors.begin(), m_swapTenors.end(), m_swapInterpolatedTenors[i]);
                vector<Tenor>::const_iterator leftBound = upper_bound(m_swapTenors.begin(), m_swapTenors.end(), m_swapInterpolatedTenors[i]);
                
                if( *rightBound == m_swapInterpolatedTenors[i])
                {
                    ++rightBound;
                }
                
                if(connectdTenors(m_swapInterpolatedTenors[i], *rightBound , m_swapsPortfolio))
                {
                    ++rightBound;
                    if( rightBound == m_swapTenors.end() )
                        --rightBound;
                }
                
                --leftBound;
                if( *leftBound == m_swapInterpolatedTenors[i] )
                {
                    --leftBound;
                }

                double weight = (rightBound->asYearFraction() - m_swapInterpolatedTenors[i].asYearFraction())/(rightBound->asYearFraction() - leftBound->asYearFraction());
                
                double annuityLeft = annuity(*leftBound);
                double annuityMiddle = annuity(m_swapInterpolatedTenors[i]);
                double annuityRight = annuity(*rightBound);
                 
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(*leftBound)) = - weight * annuityMiddle/annuityLeft;
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(*rightBound)) = - (1.0 - weight) * annuityMiddle/annuityRight;
                matrix.at(i + m_swapsPortfolio.size(), swapIndex(m_swapInterpolatedTenors[i])) = 1.0;
            }

        }

        // 3m6m basis swaps
        for(size_t i = m_swapsPortfolio.size()+m_swapInterpolatedTenors.size(); i < m_3m6mBasisSwaps.size() + m_swapsPortfolio.size() + m_swapInterpolatedTenors.size(); ++i )
        {   
            string fwdTenor;
            vector<string> tenors;
            parseDescription(m_3m6mBasisSwaps[i - m_swapsPortfolio.size() - m_swapInterpolatedTenors.size()].first, tenors, fwdTenor);
            Tenor fwd(fwdTenor);
            
            // swap
            if(tenors.size() == 1)
            {
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    matrix.at(i,swapIndex(Tenor(tenors[0]) + fwd)) = 1.0; 
                    matrix.at(i,swapIndex(fwd)) = -1.0;
                    matrix.at(i,basisSwapIndex3m6m(Tenor(tenors[0]) + fwd)) = -1.0; 
                    matrix.at(i,basisSwapIndex3m6m(fwd)) = 1.0;
                }
                // spot starting
                else
                { 
                    matrix.at(i,swapIndex(Tenor(tenors[0]))) = 1.0;   
                    matrix.at(i,basisSwapIndex3m6m(Tenor(tenors[0]))) = -1.0;
                }
            }
             // spread
            else if(tenors.size() == 2)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                bool tenor1Short = tenor1 < tenor2;

                const Tenor shortTenor = tenor1Short ? tenor1 : tenor2;
                const Tenor longTenor  = tenor1Short ? tenor2 : tenor1;
                
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuity1 = annuityBasis(fwd, shortTenor + fwd, m_3m6mSpreadFrequency);
                    double annuity2 = annuityBasis(fwd, longTenor + fwd, m_3m6mSpreadFrequency);
                    
                    matrix.at(i,swapIndex(shortTenor + fwd)) = -1.0/annuity1; 
                    matrix.at(i,swapIndex(longTenor + fwd)) = 1.0/annuity2;
                    matrix.at(i,swapIndex(fwd)) = 1.0/annuity1 - 1.0/annuity2; 

                    matrix.at(i,basisSwapIndex3m6m(shortTenor + fwd)) = 1.0/annuity1; 
                    matrix.at(i,basisSwapIndex3m6m(longTenor + fwd)) = -1.0/annuity2;
                    matrix.at(i,basisSwapIndex3m6m(fwd)) = -1.0/annuity1 + 1.0/annuity2;
                }
                // spot starting
                else
                {
                    double annuity1 = annuityBasis(shortTenor, m_3m6mSpreadFrequency);
                    double annuity2 = annuityBasis(longTenor, m_3m6mSpreadFrequency);
                    
                    matrix.at(i,swapIndex(shortTenor)) = -1.0/annuity1; 
                    matrix.at(i,swapIndex(longTenor)) = 1.0/annuity2;

                    matrix.at(i,basisSwapIndex3m6m(shortTenor)) = 1.0/annuity1;
                    matrix.at(i,basisSwapIndex3m6m(longTenor)) = -1.0/annuity2;
                }
            }
            // fly
            else if(tenors.size() == 3)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                Tenor tenor3 = Tenor(tenors[2]);
                
                if( !(tenor1 < tenor2 &&  tenor2 < tenor3) && !(tenor3 < tenor2 &&  tenor2 < tenor1) )
                {
                    LTQC_THROW(IDeA::MarketException, "Unable to deal with swap fly");   
                }
                bool tenor1Short = tenor1 < tenor3;

                const Tenor leftTenor   = tenor1Short ? tenor1 : tenor3;
                const Tenor middleTenor = tenor2;
                const Tenor rightTenor  = tenor1Short ? tenor3 : tenor1;


                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuityLeft = annuityBasis(fwd, leftTenor + fwd, m_3m6mSpreadFrequency);
                    double annuityMiddle = annuityBasis(fwd, middleTenor + fwd, m_3m6mSpreadFrequency);
                    double annuityRight = annuityBasis(fwd, rightTenor + fwd, m_3m6mSpreadFrequency);
                    
                    matrix.at(i,swapIndex(leftTenor + fwd)) = -1.0/annuityLeft; 
                    matrix.at(i,swapIndex(middleTenor + fwd)) = 2.0/annuityMiddle;  
                    matrix.at(i,swapIndex(rightTenor + fwd)) = -1.0/annuityRight; 
                    matrix.at(i,swapIndex(fwd)) = 1.0/annuityLeft + 1.0/annuityRight - 2.0/annuityMiddle;

                    matrix.at(i,basisSwapIndex3m6m(leftTenor + fwd)) = 1.0/annuityLeft;
                    matrix.at(i,basisSwapIndex3m6m(middleTenor + fwd)) = -2.0/annuityMiddle;  
                    matrix.at(i,basisSwapIndex3m6m(rightTenor + fwd)) = 1.0/annuityRight;
                    matrix.at(i,basisSwapIndex3m6m(fwd)) = -1.0/annuityLeft - 1.0/annuityRight + 2.0/annuityMiddle;
                }
                // spot starting
                else
                { 
                    double annuityLeft = annuityBasis(leftTenor,m_3m6mSpreadFrequency);
                    double annuityMiddle = annuityBasis(middleTenor,m_3m6mSpreadFrequency);
                    double annuityRight = annuityBasis(rightTenor,m_3m6mSpreadFrequency);
                   
                    matrix.at(i,swapIndex(leftTenor)) = -1.0/annuityLeft; 
                    matrix.at(i,swapIndex(middleTenor)) = 2.0/annuityMiddle;  
                    matrix.at(i,swapIndex(rightTenor)) = -1.0/annuityRight;   

                    matrix.at(i,basisSwapIndex3m6m(leftTenor)) = 1.0/annuityLeft;
                    matrix.at(i,basisSwapIndex3m6m(middleTenor)) = -2.0/annuityMiddle;  
                    matrix.at(i,basisSwapIndex3m6m(rightTenor)) = 1.0/annuityRight;
                }
            }
        }

        // interpolated basis swaps
        size_t offset =  m_3m6mBasisSwaps.size() + m_swapsPortfolio.size() + m_swapInterpolatedTenors.size();
        for(size_t i = 0; i < m_3m6mBasisSwapInterpolatedTenors.size(); ++i)
        {
            vector<Tenor>::const_iterator firstElement = m_3m6mBasisSwapTenors.begin(), lastElement = m_3m6mBasisSwapTenors.end();
            --lastElement;
            if(m_3m6mBasisSwapInterpolatedTenors[i] < *firstElement )
            {
                
                if(connectdTenors(m_3m6mBasisSwapInterpolatedTenors[i], *firstElement, m_3m6mBasisSwaps))
                {
                    ++firstElement;
                }

                matrix.at(i + offset, basisSwapIndex3m6m(m_3m6mBasisSwapInterpolatedTenors[i])) = 1.0;
                matrix.at(i + offset, basisSwapIndex3m6m(*firstElement)) = - 1.0;
                matrix.at(i + offset, swapIndex(m_3m6mBasisSwapInterpolatedTenors[i])) = - 1.0;
                matrix.at(i + offset, swapIndex(*firstElement)) = 1.0;
            }
            else if(m_3m6mBasisSwapInterpolatedTenors[i] == *firstElement )
            {
               ++firstElement;
               if(connectdTenors(m_3m6mBasisSwapInterpolatedTenors[i], *firstElement, m_3m6mBasisSwaps))
               {
                   ++firstElement;
               }
                
               matrix.at(i + offset, basisSwapIndex3m6m(m_3m6mBasisSwapInterpolatedTenors[i])) = 1.0;
               matrix.at(i + offset, basisSwapIndex3m6m(*firstElement)) = - 1.0;
               matrix.at(i + offset, swapIndex(m_3m6mBasisSwapInterpolatedTenors[i])) = - 1.0;
               matrix.at(i + offset, swapIndex(*firstElement)) = 1.0;
            }
            else if(*lastElement < m_3m6mBasisSwapInterpolatedTenors[i])
            {
                if(connectdTenors(m_3m6mBasisSwapInterpolatedTenors[i], *lastElement, m_3m6mBasisSwaps))
                {
                    --lastElement;
                }
				
				if(i < m_3m6mBasisSwapInterpolatedTenors.size() - 1)
				{
					LTQC_THROW(IDeA::MarketException, "Can not extrapolate 3m6m spreads for " << m_3m6mBasisSwapInterpolatedTenors[i].asString().data() << " and " << m_3m6mBasisSwapInterpolatedTenors[i+1].asString().data());   
				}

                matrix.at(i + offset, basisSwapIndex3m6m(m_3m6mBasisSwapInterpolatedTenors[i])) = 1.0;
                matrix.at(i + offset, basisSwapIndex3m6m(*lastElement)) = - 1.0;
                matrix.at(i + offset, swapIndex(m_3m6mBasisSwapInterpolatedTenors[i])) = - 1.0;
                matrix.at(i + offset, swapIndex(*lastElement)) = 1.0;

            }
            else if(*lastElement == m_3m6mBasisSwapInterpolatedTenors[i])
            {
                --lastElement;
                if(connectdTenors(m_3m6mBasisSwapInterpolatedTenors[i], *lastElement, m_3m6mBasisSwaps))
                {
                    --lastElement;
                }

                matrix.at(i + offset, basisSwapIndex3m6m(m_3m6mBasisSwapInterpolatedTenors[i])) = 1.0;
                matrix.at(i + offset, basisSwapIndex3m6m(*lastElement)) = - 1.0;
                matrix.at(i + offset, swapIndex(m_3m6mBasisSwapInterpolatedTenors[i])) = - 1.0;
                matrix.at(i + offset, swapIndex(*lastElement)) = 1.0;
            }
            else 
            {
                vector<Tenor>::const_iterator rightBound = lower_bound(m_3m6mBasisSwapTenors.begin(), m_3m6mBasisSwapTenors.end(), m_3m6mBasisSwapInterpolatedTenors[i]);
                vector<Tenor>::const_iterator leftBound = upper_bound(m_3m6mBasisSwapTenors.begin(), m_3m6mBasisSwapTenors.end(), m_3m6mBasisSwapInterpolatedTenors[i]);
                
                if( *rightBound == m_3m6mBasisSwapInterpolatedTenors[i] )
                {
                    ++rightBound;
                }
                  
                if(connectdTenors(m_3m6mBasisSwapInterpolatedTenors[i], *rightBound , m_3m6mBasisSwaps))
                {
                    ++rightBound;
                    if( rightBound == m_3m6mBasisSwapTenors.end() )
                        --rightBound;
                }
                
                --leftBound;
                if( *leftBound == m_3m6mBasisSwapInterpolatedTenors[i] )
                {
                    --leftBound;
                }

                double weight = (rightBound->asYearFraction() - m_3m6mBasisSwapInterpolatedTenors[i].asYearFraction())/(rightBound->asYearFraction() - leftBound->asYearFraction());
                
             

               matrix.at(i + offset, basisSwapIndex3m6m(m_3m6mBasisSwapInterpolatedTenors[i])) = 1.0;
               matrix.at(i + offset, swapIndex(m_3m6mBasisSwapInterpolatedTenors[i])) = - 1.0;
               matrix.at(i + offset, basisSwapIndex3m6m(*leftBound)) = -weight;
               matrix.at(i + offset, swapIndex(*leftBound)) = weight;
               matrix.at(i + offset, basisSwapIndex3m6m(*rightBound)) = -(1.0 - weight);
               matrix.at(i + offset, swapIndex(*rightBound)) = 1.0 - weight;

            }

        }


		 // 1m3m basis swaps
        for(size_t i = m_swapsPortfolio.size() + m_swapInterpolatedTenors.size() + m_3m6mBasisSwaps.size() + m_3m6mBasisSwapInterpolatedTenors.size(); i < m_1m3mBasisSwaps.size() + m_swapsPortfolio.size() + m_swapInterpolatedTenors.size() + m_3m6mBasisSwaps.size() + m_3m6mBasisSwapInterpolatedTenors.size(); ++i )
        {   
            string fwdTenor;
            vector<string> tenors;
            parseDescription(m_1m3mBasisSwaps[i - m_swapsPortfolio.size() - m_swapInterpolatedTenors.size() - m_3m6mBasisSwaps.size() - m_3m6mBasisSwapInterpolatedTenors.size()].first, tenors, fwdTenor);
            Tenor fwd(fwdTenor);
            
            // swap
            if(tenors.size() == 1)
            {
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
					if(m_3mSwapFrequency)
					{
						matrix.at(i,swapIndex(Tenor(tenors[0]) + fwd)) = 1.0; 
						matrix.at(i,swapIndex(fwd)) = -1.0;
						matrix.at(i,basisSwapIndex1m3m(Tenor(tenors[0]) + fwd)) = -1.0; 
						matrix.at(i,basisSwapIndex1m3m(fwd)) = 1.0;
					}
					else if(m_6mSwapFrequency)
					{
						matrix.at(i,basisSwapIndex3m6m(Tenor(tenors[0]) + fwd)) = 1.0; 
						matrix.at(i,basisSwapIndex3m6m(fwd)) = -1.0;
						matrix.at(i,basisSwapIndex1m3m(Tenor(tenors[0]) + fwd)) = -1.0; 
						matrix.at(i,basisSwapIndex1m3m(fwd)) = 1.0;
					}
                }
                // spot starting
                else
                { 
					if(m_3mSwapFrequency)
					{
						matrix.at(i,swapIndex(Tenor(tenors[0]))) = 1.0;   
						matrix.at(i,basisSwapIndex1m3m(Tenor(tenors[0]))) = -1.0;
					}
					else if(m_6mSwapFrequency)
					{
						matrix.at(i,basisSwapIndex3m6m(Tenor(tenors[0]))) =  1.0;
						matrix.at(i,basisSwapIndex1m3m(Tenor(tenors[0]))) = -1.0;
					}
                }
            }
             // spread
            else if(tenors.size() == 2)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                bool tenor1Short = tenor1 < tenor2;

                const Tenor shortTenor = tenor1Short ? tenor1 : tenor2;
                const Tenor longTenor  = tenor1Short ? tenor2 : tenor1;
                
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuity1 = annuityBasis(fwd, shortTenor + fwd, m_1m3mSpreadFrequency);
                    double annuity2 = annuityBasis(fwd, longTenor + fwd, m_1m3mSpreadFrequency);
                    
					if(m_3mSwapFrequency)
					{
						matrix.at(i,swapIndex(shortTenor + fwd)) = -1.0/annuity1; 
						matrix.at(i,swapIndex(longTenor + fwd)) = 1.0/annuity2;
						matrix.at(i,swapIndex(fwd)) = 1.0/annuity1 - 1.0/annuity2; 

						matrix.at(i,basisSwapIndex1m3m(shortTenor + fwd)) = 1.0/annuity1; 
						matrix.at(i,basisSwapIndex1m3m(longTenor + fwd)) = -1.0/annuity2;
						matrix.at(i,basisSwapIndex1m3m(fwd)) = -1.0/annuity1 + 1.0/annuity2;
					}
					else if(m_6mSwapFrequency)
					{
						matrix.at(i,basisSwapIndex3m6m(shortTenor + fwd)) = 1.0/annuity1; 
						matrix.at(i,basisSwapIndex3m6m(longTenor + fwd)) = -1.0/annuity2;
						matrix.at(i,basisSwapIndex3m6m(fwd)) = -1.0/annuity1 + 1.0/annuity2;

						matrix.at(i,basisSwapIndex1m3m(shortTenor + fwd)) = 1.0/annuity1; 
						matrix.at(i,basisSwapIndex1m3m(longTenor + fwd)) = -1.0/annuity2;
						matrix.at(i,basisSwapIndex1m3m(fwd)) = -1.0/annuity1 + 1.0/annuity2;
					}
                }
                // spot starting
                else
                {
                    double annuity1 = annuityBasis(shortTenor, m_1m3mSpreadFrequency);
                    double annuity2 = annuityBasis(longTenor, m_1m3mSpreadFrequency);
                    if(m_3mSwapFrequency)
					{
						matrix.at(i,swapIndex(shortTenor)) = -1.0/annuity1; 
						matrix.at(i,swapIndex(longTenor)) = 1.0/annuity2;

						matrix.at(i,basisSwapIndex1m3m(shortTenor)) = 1.0/annuity1;
						matrix.at(i,basisSwapIndex1m3m(longTenor)) = -1.0/annuity2;
					}
					else if(m_6mSwapFrequency)
					{
						matrix.at(i,basisSwapIndex3m6m(shortTenor)) = -1.0/annuity1; 
						matrix.at(i,basisSwapIndex3m6m(longTenor)) = 1.0/annuity2;

						matrix.at(i,basisSwapIndex1m3m(shortTenor)) = 1.0/annuity1;
						matrix.at(i,basisSwapIndex1m3m(longTenor)) = -1.0/annuity2;
					}
                }
            }
            // fly
            else if(tenors.size() == 3)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                Tenor tenor3 = Tenor(tenors[2]);
                
                if( !(tenor1 < tenor2 &&  tenor2 < tenor3) && !(tenor3 < tenor2 &&  tenor2 < tenor1) )
                {
                    LTQC_THROW(IDeA::MarketException, "Unable to deal with swap fly");   
                }
                bool tenor1Short = tenor1 < tenor3;

                const Tenor leftTenor   = tenor1Short ? tenor1 : tenor3;
                const Tenor middleTenor = tenor2;
                const Tenor rightTenor  = tenor1Short ? tenor3 : tenor1;


                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuityLeft = annuityBasis(fwd, leftTenor + fwd, m_1m3mSpreadFrequency);
                    double annuityMiddle = annuityBasis(fwd, middleTenor + fwd, m_1m3mSpreadFrequency);
                    double annuityRight = annuityBasis(fwd, rightTenor + fwd, m_1m3mSpreadFrequency);

                    if(m_3mSwapFrequency)
					{
						matrix.at(i,swapIndex(leftTenor + fwd)) = -1.0/annuityLeft; 
						matrix.at(i,swapIndex(middleTenor + fwd)) = 2.0/annuityMiddle;  
						matrix.at(i,swapIndex(rightTenor + fwd)) = -1.0/annuityRight; 
						matrix.at(i,swapIndex(fwd)) = 1.0/annuityLeft + 1.0/annuityRight - 2.0/annuityMiddle;

						matrix.at(i,basisSwapIndex1m3m(leftTenor + fwd)) = 1.0/annuityLeft;
						matrix.at(i,basisSwapIndex1m3m(middleTenor + fwd)) = -2.0/annuityMiddle;  
						matrix.at(i,basisSwapIndex1m3m(rightTenor + fwd)) = 1.0/annuityRight;
						matrix.at(i,basisSwapIndex1m3m(fwd)) = -1.0/annuityLeft - 1.0/annuityRight + 2.0/annuityMiddle;
					 }
					else if(m_6mSwapFrequency)
					{
						matrix.at(i,basisSwapIndex3m6m(leftTenor + fwd)) = -1.0/annuityLeft; 
						matrix.at(i,basisSwapIndex3m6m(middleTenor + fwd)) = 2.0/annuityMiddle;  
						matrix.at(i,basisSwapIndex3m6m(rightTenor + fwd)) = -1.0/annuityRight; 
						matrix.at(i,basisSwapIndex3m6m(fwd)) = 1.0/annuityLeft + 1.0/annuityRight - 2.0/annuityMiddle;

						matrix.at(i,basisSwapIndex1m3m(leftTenor + fwd)) = 1.0/annuityLeft;
						matrix.at(i,basisSwapIndex1m3m(middleTenor + fwd)) = -2.0/annuityMiddle;  
						matrix.at(i,basisSwapIndex1m3m(rightTenor + fwd)) = 1.0/annuityRight;
						matrix.at(i,basisSwapIndex1m3m(fwd)) = -1.0/annuityLeft - 1.0/annuityRight + 2.0/annuityMiddle;
					 }
                }
                // spot starting
                else
                { 
                    double annuityLeft = annuityBasis(leftTenor,m_1m3mSpreadFrequency);
                    double annuityMiddle = annuityBasis(middleTenor,m_1m3mSpreadFrequency);
                    double annuityRight = annuityBasis(rightTenor,m_1m3mSpreadFrequency);

                    if(m_3mSwapFrequency)
					{
						matrix.at(i,swapIndex(leftTenor)) = -1.0/annuityLeft; 
						matrix.at(i,swapIndex(middleTenor)) = 2.0/annuityMiddle;  
						matrix.at(i,swapIndex(rightTenor)) = -1.0/annuityRight;   

						matrix.at(i,basisSwapIndex1m3m(leftTenor)) = 1.0/annuityLeft;
						matrix.at(i,basisSwapIndex1m3m(middleTenor)) = -2.0/annuityMiddle;  
						matrix.at(i,basisSwapIndex1m3m(rightTenor)) = 1.0/annuityRight;
					}
					else if(m_6mSwapFrequency)
					{
						matrix.at(i,basisSwapIndex3m6m(leftTenor)) = -1.0/annuityLeft; 
						matrix.at(i,basisSwapIndex3m6m(middleTenor)) = 2.0/annuityMiddle;  
						matrix.at(i,basisSwapIndex3m6m(rightTenor)) = -1.0/annuityRight;   

						matrix.at(i,basisSwapIndex1m3m(leftTenor)) = 1.0/annuityLeft;
						matrix.at(i,basisSwapIndex1m3m(middleTenor)) = -2.0/annuityMiddle;  
						matrix.at(i,basisSwapIndex1m3m(rightTenor)) = 1.0/annuityRight;
					}
                }
            }
        }
		
        // interpolated basis swaps
        offset =  m_1m3mBasisSwaps.size() + m_3m6mBasisSwaps.size() + m_3m6mBasisSwapInterpolatedTenors.size() + m_swapsPortfolio.size() + m_swapInterpolatedTenors.size();
        for(size_t i = 0; i < m_1m3mBasisSwapInterpolatedTenors.size(); ++i)
        {
            vector<Tenor>::const_iterator firstElement = m_1m3mBasisSwapTenors.begin(), lastElement = m_1m3mBasisSwapTenors.end();
            --lastElement;
            if(m_1m3mBasisSwapInterpolatedTenors[i] < *firstElement )
            {
                
                if(connectdTenors(m_1m3mBasisSwapInterpolatedTenors[i], *firstElement, m_1m3mBasisSwaps))
                {
                    ++firstElement;
                }
			
				if(m_3mSwapFrequency)
				{
					matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
					matrix.at(i + offset, basisSwapIndex1m3m(*firstElement)) = - 1.0;
					matrix.at(i + offset, swapIndex(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
					matrix.at(i + offset, swapIndex(*firstElement)) = 1.0;
			    }
				else if(m_6mSwapFrequency)
				{
					matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
					matrix.at(i + offset, basisSwapIndex1m3m(*firstElement)) = - 1.0;
					matrix.at(i + offset, basisSwapIndex3m6m(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
					matrix.at(i + offset, basisSwapIndex3m6m(*firstElement)) = 1.0;
				}
            }
            else if(m_1m3mBasisSwapInterpolatedTenors[i] == *firstElement )
            {
               ++firstElement;
               if(connectdTenors(m_1m3mBasisSwapInterpolatedTenors[i], *firstElement, m_3m6mBasisSwaps))
               {
                   ++firstElement;
               }
                
			   if(m_3mSwapFrequency)
			   {
				   matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
				   matrix.at(i + offset, basisSwapIndex1m3m(*firstElement)) = - 1.0;
				   matrix.at(i + offset, swapIndex(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
				   matrix.at(i + offset, swapIndex(*firstElement)) = 1.0;
			   }
			   else if(m_6mSwapFrequency)
			   {
				   matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
				   matrix.at(i + offset, basisSwapIndex1m3m(*firstElement)) = - 1.0;
				   matrix.at(i + offset, basisSwapIndex3m6m(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
				   matrix.at(i + offset, basisSwapIndex3m6m(*firstElement)) = 1.0;
			   }
            }
            else if(*lastElement < m_1m3mBasisSwapInterpolatedTenors[i])
            {
                if(connectdTenors(m_1m3mBasisSwapInterpolatedTenors[i], *lastElement, m_1m3mBasisSwaps))
                {
                    --lastElement;
                }

				if(m_3mSwapFrequency)
			    {
					matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
					matrix.at(i + offset, basisSwapIndex1m3m(*lastElement)) = - 1.0;
					matrix.at(i + offset, swapIndex(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
					matrix.at(i + offset, swapIndex(*lastElement)) = 1.0;
				}
				else if(m_6mSwapFrequency)
				{
					matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
					matrix.at(i + offset, basisSwapIndex1m3m(*lastElement)) = - 1.0;
					matrix.at(i + offset, basisSwapIndex3m6m(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
					matrix.at(i + offset, basisSwapIndex3m6m(*lastElement)) = 1.0;
				}
            }
            else if(*lastElement == m_1m3mBasisSwapInterpolatedTenors[i])
            {
                --lastElement;
                if(connectdTenors(m_1m3mBasisSwapInterpolatedTenors[i], *lastElement, m_1m3mBasisSwaps))
                {
                    --lastElement;
                }
				if(m_3mSwapFrequency)
			    {
					matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
					matrix.at(i + offset, basisSwapIndex1m3m(*lastElement)) = - 1.0;
					matrix.at(i + offset, swapIndex(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
					matrix.at(i + offset, swapIndex(*lastElement)) = 1.0;
				}
				else if(m_6mSwapFrequency)
				{
					matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
					matrix.at(i + offset, basisSwapIndex1m3m(*lastElement)) = - 1.0;
					matrix.at(i + offset, basisSwapIndex3m6m(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
					matrix.at(i + offset, basisSwapIndex3m6m(*lastElement)) = 1.0;
				}
            }
            else 
            {
                vector<Tenor>::const_iterator rightBound = lower_bound(m_1m3mBasisSwapTenors.begin(), m_1m3mBasisSwapTenors.end(), m_1m3mBasisSwapInterpolatedTenors[i]);
                vector<Tenor>::const_iterator leftBound = upper_bound(m_1m3mBasisSwapTenors.begin(), m_1m3mBasisSwapTenors.end(), m_1m3mBasisSwapInterpolatedTenors[i]);
                
                if( *rightBound == m_1m3mBasisSwapInterpolatedTenors[i] )
                {
                    ++rightBound;
                }
                  
                if(connectdTenors(m_1m3mBasisSwapInterpolatedTenors[i], *rightBound , m_1m3mBasisSwaps))
                {
                    ++rightBound;
                    if( rightBound == m_1m3mBasisSwapTenors.end() )
                        --rightBound;
                }
                
                --leftBound;
                if( *leftBound == m_1m3mBasisSwapInterpolatedTenors[i] )
                {
                    --leftBound;
                }

                double weight = (rightBound->asYearFraction() - m_1m3mBasisSwapInterpolatedTenors[i].asYearFraction())/(rightBound->asYearFraction() - leftBound->asYearFraction());
                
             
				if(m_3mSwapFrequency)
			    {
				   matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
				   matrix.at(i + offset, swapIndex(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
				   matrix.at(i + offset, basisSwapIndex1m3m(*leftBound)) = -weight;
				   matrix.at(i + offset, swapIndex(*leftBound)) = weight;
				   matrix.at(i + offset, basisSwapIndex1m3m(*rightBound)) = -(1.0 - weight);
				   matrix.at(i + offset, swapIndex(*rightBound)) = 1.0 - weight;
				}
				else if(m_6mSwapFrequency)
				{
				   matrix.at(i + offset, basisSwapIndex1m3m(m_1m3mBasisSwapInterpolatedTenors[i])) = 1.0;
				   matrix.at(i + offset, basisSwapIndex3m6m(m_1m3mBasisSwapInterpolatedTenors[i])) = - 1.0;
				   matrix.at(i + offset, basisSwapIndex1m3m(*leftBound)) = -weight;
				   matrix.at(i + offset, basisSwapIndex3m6m(*leftBound)) = weight;
				   matrix.at(i + offset, basisSwapIndex1m3m(*rightBound)) = -(1.0 - weight);
				   matrix.at(i + offset, basisSwapIndex3m6m(*rightBound)) = 1.0 - weight;

				}
            }
        }

        if( !noSpotOrFwdSwap )
        {
            LTQC_THROW(IDeA::MarketException, "At least one spot/fwd starting swap is needed, but none provided");   
        }
        m_matrix = matrix;
    }

    void SwapMarketSolverCalculator::generateTenors()
    {
        set<Tenor> all3m6mTenors;
        vector<Tenor> tmp;

        for(size_t i = 0; i < m_3m6mBasisSwaps.size(); ++i )
        {   
            vector<Tenor> result;
            string fwdTenor;
            vector<string> tenors;
            parseDescription(m_3m6mBasisSwaps[i].first, tenors, fwdTenor);
        
            Tenor fwd(fwdTenor);
            if( fwd.asYearFraction() > 0.0 )
            {
                result.push_back(fwd);
            }
            for(size_t i = 0; i < tenors.size(); ++i)
            {
                Tenor tenor(tenors[i]);
                result.push_back(tenor + fwd);
            }
            all3m6mTenors.insert( result.begin(), result.end() );
        }
        m_3m6mBasisSwapTenors.insert(m_3m6mBasisSwapTenors.end(), all3m6mTenors.begin(), all3m6mTenors.end());
		
		set<Tenor> all1m3mTenors;
		for(size_t i = 0; i < m_1m3mBasisSwaps.size(); ++i )
        {   
            vector<Tenor> result;
            string fwdTenor;
            vector<string> tenors;
            parseDescription(m_1m3mBasisSwaps[i].first, tenors, fwdTenor);
        
            Tenor fwd(fwdTenor);
            if( fwd.asYearFraction() > 0.0 )
            {
                result.push_back(fwd);
            }
            for(size_t i = 0; i < tenors.size(); ++i)
            {
                Tenor tenor(tenors[i]);
                result.push_back(tenor + fwd);
            }
            all1m3mTenors.insert( result.begin(), result.end() );
        }
        m_1m3mBasisSwapTenors.insert(m_1m3mBasisSwapTenors.end(), all1m3mTenors.begin(), all1m3mTenors.end());
        
		set<Tenor> allSwapTenors;
        for(size_t i = 0; i < m_swapsPortfolio.size(); ++i )
        {   
            vector<Tenor> result;
            string fwdTenor;
            vector<string> tenors;
            parseDescription(m_swapsPortfolio[i].first, tenors, fwdTenor);
        
            Tenor fwd(fwdTenor);
            if( fwd.asYearFraction() > 0.0 )
            {
                result.push_back(fwd);
            }
            for(size_t i = 0; i < tenors.size(); ++i)
            {
                Tenor tenor(tenors[i]);
                result.push_back(tenor + fwd);
            }
            allSwapTenors.insert( result.begin(), result.end() );
        }
        m_swapTenors.insert(m_swapTenors.end(), allSwapTenors.begin(), allSwapTenors.end());
        
        
        set<Tenor> interpolatedTenors;
        vector< pair<string,double> > swapsPortfolio;
        swapsPortfolio.insert(swapsPortfolio.end(), m_swapsPortfolio.begin() ,m_swapsPortfolio.end());
    
        while( isInputPortfolioUnderspecified( swapsPortfolio, allSwapTenors, interpolatedTenors)  )
        {
            swapsPortfolio.push_back(pair<string,double>(string(interpolatedTenors.begin()->asString().data()), 0.0));
            vector<Tenor>::const_iterator it = find(m_swapInterpolatedTenors.begin(), m_swapInterpolatedTenors.end(), *interpolatedTenors.begin());
            if( it == m_swapInterpolatedTenors.end() )
            {
                m_swapInterpolatedTenors.push_back(*interpolatedTenors.begin());
            }
            interpolatedTenors.clear();
        }

       interpolatedTenors.clear();
       vector< pair<string,double> > basisSwapsPortfolio;
       basisSwapsPortfolio.insert(basisSwapsPortfolio.end(), m_3m6mBasisSwaps.begin() ,m_3m6mBasisSwaps.end());
    
       while( isInputPortfolioUnderspecified( basisSwapsPortfolio, all3m6mTenors, interpolatedTenors)  )
       {
            basisSwapsPortfolio.push_back(pair<string,double>(string(interpolatedTenors.begin()->asString().data()), 0.0));
            m_3m6mBasisSwapInterpolatedTenors.push_back(*interpolatedTenors.begin());
            interpolatedTenors.clear();
       }
   
	   interpolatedTenors.clear();
       basisSwapsPortfolio.clear();
       basisSwapsPortfolio.insert(basisSwapsPortfolio.end(), m_1m3mBasisSwaps.begin() ,m_1m3mBasisSwaps.end());
    
       while( isInputPortfolioUnderspecified( basisSwapsPortfolio, all1m3mTenors, interpolatedTenors)  )
       {
         basisSwapsPortfolio.push_back(pair<string,double>(string(interpolatedTenors.begin()->asString().data()), 0.0));
         m_1m3mBasisSwapInterpolatedTenors.push_back(*interpolatedTenors.begin());
         interpolatedTenors.clear();
       }

	   if(m_6mSwapFrequency)
		{
			set<Tenor> allBasisTenorsOnly;
			set_difference(m_1m3mBasisSwapTenors.begin(),m_1m3mBasisSwapTenors.end(),m_3m6mBasisSwapTenors.begin(),m_3m6mBasisSwapTenors.end(),inserter(allBasisTenorsOnly, allBasisTenorsOnly.end()));
			m_3m6mBasisSwapInterpolatedTenors.insert(m_3m6mBasisSwapInterpolatedTenors.end(), allBasisTenorsOnly.begin(), allBasisTenorsOnly.end());

			allBasisTenorsOnly.clear();
			set_difference(m_3m6mBasisSwapTenors.begin(),m_3m6mBasisSwapTenors.end(),allSwapTenors.begin(),allSwapTenors.end(),inserter(allBasisTenorsOnly, allBasisTenorsOnly.begin()));
			m_swapInterpolatedTenors.insert(m_swapInterpolatedTenors.end(), allBasisTenorsOnly.begin(), allBasisTenorsOnly.end());

			std::sort(m_3m6mBasisSwapInterpolatedTenors.begin(),m_3m6mBasisSwapInterpolatedTenors.end());
		} 
		else if(m_3mSwapFrequency)
		{
			set<Tenor> allBasisTenorsOnly;
			set_difference(m_1m3mBasisSwapTenors.begin(),m_1m3mBasisSwapTenors.end(),allSwapTenors.begin(),allSwapTenors.end(),inserter(allBasisTenorsOnly, allBasisTenorsOnly.end()));
			m_swapInterpolatedTenors.insert(m_swapInterpolatedTenors.end(), allBasisTenorsOnly.begin(), allBasisTenorsOnly.end());

			allBasisTenorsOnly.clear();
			set_difference(m_3m6mBasisSwapTenors.begin(),m_3m6mBasisSwapTenors.end(),allSwapTenors.begin(),allSwapTenors.end(),inserter(allBasisTenorsOnly, allBasisTenorsOnly.begin()));
			m_swapInterpolatedTenors.insert(m_swapInterpolatedTenors.end(), allBasisTenorsOnly.begin(), allBasisTenorsOnly.end());

			std::sort(m_swapInterpolatedTenors.begin(),m_swapInterpolatedTenors.end());
		}
    }
    
    VectorDouble SwapMarketSolverCalculator::solve()
    {  
        LTQC::Matrix w;
        w.makeDiagonalMatrix(m_weights);

        LTQC::Matrix aT = m_matrix.transpose();
        LTQC::Matrix aTw;
        aTw.mult(aT,w);

        LTQC::Matrix aTwa;
        aTwa.mult(aTw, m_matrix);
        aTwa.inverse();

        LTQC::Matrix result;
        result.mult(aTwa, aTw);
        
        VectorDouble column;
        dot_and_assign(result, m_marketQuotes, false, column);
       
        
       
        // convert to spot par basis swap rates
		auto itBasis = m_1m3mBasisSwapTenors.begin(), itBasisEnd = m_1m3mBasisSwapTenors.end();
        for(size_t i = 0; itBasis != itBasisEnd ; ++itBasis, ++i)
        {      
            size_t basisIndex = basisSwapIndex1m3m(*itBasis);
            if(m_6mSwapFrequency)
            {
                column[basisIndex] = (column[basisSwapIndex3m6m(*itBasis)] - column[basisIndex]) /annuityBasis(*itBasis, m_1m3mSpreadFrequency);
            }
            else if(m_3mSwapFrequency)
            {
                column[basisIndex] = (column[swapIndex(*itBasis)] - column[basisIndex]) /annuityBasis(*itBasis, m_1m3mSpreadFrequency);
            }
        }

        itBasis = m_3m6mBasisSwapTenors.begin(), itBasisEnd = m_3m6mBasisSwapTenors.end();
        for(size_t i = 0; itBasis != itBasisEnd ; ++itBasis, ++i)
        {      
            size_t basisIndex = basisSwapIndex3m6m(*itBasis);
            if(m_3mSwapFrequency)
            {
                column[basisIndex] = (column[basisIndex] - column[swapIndex(*itBasis)]) /annuityBasis(*itBasis, m_3m6mSpreadFrequency);
            }
            else
            {
                column[basisIndex] = (column[swapIndex(*itBasis)] - column[basisIndex]) /annuityBasis(*itBasis, m_3m6mSpreadFrequency);
            }
        }
      
		

        // convert to spot par swap rates
        set<Tenor> allSwapTenors;
        allSwapTenors.insert(m_swapTenors.begin(),m_swapTenors.end());
        allSwapTenors.insert(m_swapInterpolatedTenors.begin(),m_swapInterpolatedTenors.end());
        set<Tenor>::const_iterator it = allSwapTenors.begin(), itEnd = allSwapTenors.end();
        for( ; it != itEnd ; ++it)
        {
            column[swapIndex(*it)] /= annuity(*it);
        }
        
        return column;
    }
    
   /* VectorDouble SwapMarketSolverCalculator::computeSwapParRates(const VectorDouble& solution) const
    {
        VectorDouble parRates(m_swapTenors.size());
        vector<Tenor>::const_iterator it = m_swapTenors.begin(), itEnd = m_swapTenors.end();
        for(size_t i = 0 ; it != itEnd ; ++it, ++i)
        {
            parRates[i] = solution[swapIndex(*it)] / annuity(*it);
        }
    }
     
    VectorDouble SwapMarketSolverCalculator::computeBasisSwapParRates(const VectorDouble& solution) const
    {
        VectorDouble parRates(solution.size());
        vector<Tenor>::const_iterator it = m_3m6mswapTenors.begin(), itEnd = m_swapTenors.end();
        for( ; it != itEnd ; ++it)
        {
            column[swapIndex(*it)] /= annuity(*it);
        }
    }*/

    double SwapMarketSolverCalculator::implyQuote(const VectorDouble& parSwapRates, const string& productDescription) const
    {
        string fwdTenor;
        vector<string> tenors;
        parseDescription(productDescription, tenors, fwdTenor);
        Tenor fwd(fwdTenor);

        // swap
        if(tenors.size() == 1)
        {
            // fwd starting
            if( fwd.asYearFraction() > 0.0 )
            {
                 Tenor t(tenors[0]);
                 Tenor t2(t + fwd);

                 double s2 = parSwapRates[swapIndex(t2)]; 
                 double s1 = parSwapRates[swapIndex(fwd)];
                  
                 double fwdAnnuity = annuity(fwd,t2);
                 
                 return s2 * annuity(t2)/fwdAnnuity - s1 * annuity(fwd)/fwdAnnuity;
            }
            // spot starting
            else
            {
                return parSwapRates[swapIndex(Tenor(tenors[0]))];   
            }
        }
        // spread
        else if(tenors.size() == 2)
        {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                bool tenor1Short = tenor1 < tenor2;

                const Tenor shortTenor = tenor1Short ? tenor1 : tenor2;
                const Tenor longTenor  = tenor1Short ? tenor2 : tenor1;
                
                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuity1 = annuity(fwd, shortTenor + fwd);
                    double annuity2 = annuity(fwd, longTenor + fwd);

                    double s2 = parSwapRates[swapIndex(shortTenor + fwd)];
                    double s3 = parSwapRates[swapIndex(longTenor + fwd)];
                    double s1 = parSwapRates[swapIndex(fwd)];

                    return s3 * annuity(longTenor + fwd)/annuity2 - s1 * annuity(fwd)/annuity2 - s2 * annuity(shortTenor + fwd)/annuity1 + s1 * annuity(fwd)/annuity1;
                }
                // spot starting
                else
                {
                    double s1 = parSwapRates[swapIndex(shortTenor)];
                    double s2 = parSwapRates[swapIndex(longTenor)];

                    return s2 - s1;
                }
        }
        // fly
        else if(tenors.size() == 3)
        {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                Tenor tenor3 = Tenor(tenors[2]);
                
                if( !(tenor1 < tenor2 &&  tenor2 < tenor3) && !(tenor3 < tenor2 &&  tenor2 < tenor1) )
                {
                    LTQC_THROW(IDeA::MarketException, "Unable to deal with swap fly");   
                }
                bool tenor1Short = tenor1 < tenor3;

                const Tenor leftTenor   = tenor1Short ? tenor1 : tenor3;
                const Tenor middleTenor = tenor2;
                const Tenor rightTenor  = tenor1Short ? tenor3 : tenor1;


                // fwd starting
                if( fwd.asYearFraction() > 0.0 )
                {
                    double annuityLeft = annuity(fwd, leftTenor + fwd);
                    double annuityMiddle = annuity(fwd, middleTenor + fwd);
                    double annuityRight = annuity(fwd, rightTenor + fwd);

                    double s1 = parSwapRates[swapIndex(leftTenor + fwd)];
                    double s2 = parSwapRates[swapIndex(middleTenor + fwd)];
                    double s3 = parSwapRates[swapIndex(rightTenor + fwd)];
                    double s0 = parSwapRates[swapIndex(fwd)];

                    double fwd1 = s1 * annuity(leftTenor + fwd)/annuityLeft - s0 * annuity(fwd)/annuityLeft;
                    double fwd2 = s2 * annuity(middleTenor + fwd)/annuityMiddle - s0 * annuity(fwd)/annuityMiddle;
                    double fwd3 = s3 * annuity(rightTenor + fwd)/annuityRight - s0 * annuity(fwd)/annuityRight;

                    return 2.0 * fwd2 - fwd1 - fwd3;
                }
                // spot starting
                else
                { 
                    double s1 = parSwapRates[swapIndex(leftTenor)];
                    double s2 = parSwapRates[swapIndex(middleTenor)];
                    double s3 = parSwapRates[swapIndex(rightTenor)];

                    return 2.0 * s2 - s1 - s3;
                }
         }
        else
        {
            LTQC_THROW(IDeA::MarketException, "Unable to deal with swap description " << productDescription);   
        }
        return 0.0;
    }
    
	LT::TablePtr SwapMarketSolverCalculator::generateParRatesOutputTable()
	{
		LT::TablePtr result = LT::TablePtr(new LT::Table("ParRates", 4, 2, LT::Table::rowOriented) );

		VectorDouble parRates = solve();
		const vector<Tenor>& tenors = m_swapTenors;
		size_t k = tenors.size();
		size_t row = 0;
        LT::TablePtr tbl = LT::TablePtr(new LT::Table("Swaps", k+1, 5) );
        tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
        tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
        tbl->at(0,2) = "Spread";
        tbl->at(0,3) = IDeA_KEY(SWAPSPORTFOLIO,FIXEDTENOR).getName();
        tbl->at(0,4) = IDeA_KEY(SWAPSPORTFOLIO,FIXEDBASIS).getName();
            
        for(size_t i = 1; i < k + 1;++i)
        {
            tbl->at(i,0) = tenors[i-1].asString();
            tbl->at(i,1) = parRates[i-1];
            tbl->at(i,2) = 0.0;
            tbl->at(i,3) = m_fixedLegTenor;
            tbl->at(i,4) = m_fixedLegBasis;
        }
		result->at(row,0) = "Swaps";
		result->at(row,1) = tbl;
		++row;

		if(m_3m6mBasisSwapTable)
		{
			const vector<Tenor>& tenors = m_3m6mBasisSwapTenors;
			size_t k =  tenors.size();
        
			LT::TablePtr tbl = LT::TablePtr(new LT::Table("3m6mBasisSwaps", k+1, 2) );
			tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
			tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
       
            
			for(size_t i = 1; i < k + 1;++i)
			{
				tbl->at(i,0) = tenors[i-1].asString();
				tbl->at(i,1) = parRates[basisSwapIndex3m6m(tenors[i-1])];
			}
			result->at(row,0) = "3m6mBasisSwaps";
			result->at(row,1) = tbl;
			++row;
		}

		if(m_1m3mBasisSwapTable)
		{
			const vector<Tenor>& tenors = m_1m3mBasisSwapTenors;
			size_t k =  tenors.size();
        
			LT::TablePtr tbl = LT::TablePtr(new LT::Table("1m3mBasisSwaps", k+1, 2) );
			tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
			tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
       
            
			for(size_t i = 1; i < k + 1;++i)
			{
				tbl->at(i,0) = tenors[i-1].asString();
				tbl->at(i,1) = parRates[basisSwapIndex1m3m(tenors[i-1])];
			}
			result->at(row,0) = "1m3mBasisSwaps";
			result->at(row,1) = tbl;
			++row;
		}

		if(!m_swapInterpolatedTenors.empty())
		{
			 const vector<Tenor>& tenors = m_swapInterpolatedTenors;
     
			size_t k =  tenors.size();
        
			LT::TablePtr tbl = LT::TablePtr(new LT::Table("InterpolatedSwaps", k+1, 2) );
			tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
			tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
       
            
			for(size_t i = 1; i < k + 1;++i)
			{
				tbl->at(i,0) = tenors[i-1].asString();
				tbl->at(i,1) = parRates[swapIndex(tenors[i-1])];
			}

			result->at(row,0) = "InterpolatedSwaps";
			result->at(row,1) = tbl;
		}
		return result;
	}

    LT::TablePtr SwapMarketSolverCalculator::generateOutputTable()
    {

        VectorDouble parRates = solve();
        const vector<Tenor>& tenors = m_swapTenors;
        size_t k = tenors.size();

        LT::TablePtr tbl = LT::TablePtr(new LT::Table("Swaps", k+1, 5) );
        tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
        tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
        tbl->at(0,2) = "Spread";
        tbl->at(0,3) = IDeA_KEY(SWAPSPORTFOLIO,FIXEDTENOR).getName();
        tbl->at(0,4) = IDeA_KEY(SWAPSPORTFOLIO,FIXEDBASIS).getName();
            
        for(size_t i = 1; i < k + 1;++i)
        {
            tbl->at(i,0) = tenors[i-1].asString();
            tbl->at(i,1) = parRates[i-1];
            tbl->at(i,2) = 0.0;
            tbl->at(i,3) = m_fixedLegTenor;
            tbl->at(i,4) = m_fixedLegBasis;
        }

        return tbl;
    }
    
    LT::TablePtr SwapMarketSolverCalculator::generateInterpolatedSwapsOutputTable()
    {
        VectorDouble parRates = solve();
        const vector<Tenor>& tenors = m_swapInterpolatedTenors;
     
        size_t k =  tenors.size();
        
        LT::TablePtr tbl = LT::TablePtr(new LT::Table("InterpolatedSwaps", k+1, 2) );
        tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
        tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
       
            
        for(size_t i = 1; i < k + 1;++i)
        {
            tbl->at(i,0) = tenors[i-1].asString();
            tbl->at(i,1) = parRates[swapIndex(tenors[i-1])];
        }

        return tbl;
    }
    
    LT::TablePtr SwapMarketSolverCalculator::generateBasisSwapsOutputTable()
    {
        if(!m_basisSwapTable)
            return LT::TablePtr(new LT::Table("BasisSwaps", 0, 0) );

        VectorDouble parRates = solve();
        const vector<Tenor>& tenors = m_3m6mBasisSwapTenors;
        set<Tenor> allSwapTenors;
        allSwapTenors.insert(m_swapTenors.begin(),m_swapTenors.end());
        allSwapTenors.insert(m_swapInterpolatedTenors.begin(),m_swapInterpolatedTenors.end());

        size_t noSwaps =  allSwapTenors.size();
        size_t k =  tenors.size();
        
        LT::TablePtr tbl = LT::TablePtr(new LT::Table("BasisSwaps", k+1, 2) );
        tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
        tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
       
            
        for(size_t i = 1; i < k + 1;++i)
        {
            tbl->at(i,0) = tenors[i-1].asString();
            tbl->at(i,1) = parRates[basisSwapIndex3m6m(tenors[i-1])];
        }

        return tbl;
    }

    LT::TablePtr SwapMarketSolverCalculator::generateAnnuityOutputTable()
    {
       
        size_t k = m_annuity.size();

        LT::TablePtr tbl = LT::TablePtr(new LT::Table("Annuity", k+1, 2) );
        tbl->at(0,0) = "Date";
        tbl->at(0,1) = "Annuity";
            
        for(size_t i = 1; i < k + 1;++i)
        {
            tbl->at(i,0) = LT::Str(m_annuity[i-1].first.toExcelString());
            tbl->at(i,1) = m_annuity[i-1].second;
        }

        return tbl;
    }
   
    LT::TablePtr SwapMarketSolverCalculator::generateSpreadLegAnnuityOutputTable()
    {
        if(!m_basisSwapTable)
            return LT::TablePtr(new LT::Table("SpreadLegAnnuity", 0, 0) );

        const vector< pair<LT::date, double> >& annuity = m_annuityMap[m_3m6mSpreadFrequency];
        size_t k = annuity.size();

        LT::TablePtr tbl = LT::TablePtr(new LT::Table("SpreadLegAnnuity", k+1, 2) );
        tbl->at(0,0) = "Date";
        tbl->at(0,1) = "SpreadLegAnnuity";
            
        for(size_t i = 1; i < k + 1;++i)
        {
            tbl->at(i,0) = LT::Str(annuity[i-1].first.toExcelString());
            tbl->at(i,1) = annuity[i-1].second;
        }

        return tbl;
    }

    LT::TablePtr SwapMarketSolverCalculator::generateImpliedQuotesOutputTable()
    {
        VectorDouble parRates = solve();
        size_t k = m_swapsPortfolio.size();

        LT::TablePtr tbl = LT::TablePtr(new LT::Table("SwapsPortfolio", k+1, 6) );
        tbl->at(0,0) = IDeA_KEY(SWAPSPORTFOLIO,TENOR).getName();
        tbl->at(0,1) = IDeA_KEY(SWAPSPORTFOLIO,RATE).getName();
        tbl->at(0,2) = IDeA_KEY(SWAPSPORTFOLIO,WEIGHT).getName();
        tbl->at(0,3) = IDeA_KEY(SWAPSPORTFOLIO,FIXEDTENOR).getName();
        tbl->at(0,4) = IDeA_KEY(SWAPSPORTFOLIO,FIXEDBASIS).getName();
        tbl->at(0,5) = IDeA_KEY(SWAPSPORTFOLIO,ACCRUALCALENDAR).getName();
            
        for(size_t i = 1; i < k + 1;++i)
        {
            const string& description = m_swapsPortfolio[i-1].first;
            
            tbl->at(i,0) = LT::Str(description);
            tbl->at(i,1) = implyQuote(parRates, description);
            tbl->at(i,2) = m_weights[i-1];
            tbl->at(i,3) = m_fixedLegTenor;
            tbl->at(i,4) = m_fixedLegBasis;
            tbl->at(i,5) = m_fixedLegCalendar;
        }

        return tbl;
    }


    IRSwapProductType SwapMarketSolverCalculator::toIRSwapProductType(const string& description) 
    {
        string fwd;
        vector<string> tenors;
        parseDescription(description, tenors, fwd);
        if( tenors.size() == 1)
        {
            return IRSwapProductType::Swap;
        }
        if( tenors.size() == 2)
        {
            return IRSwapProductType::Spread;
        }
        if( tenors.size() == 3)
        {
            return IRSwapProductType::Fly;
        }
        LTQC_THROW(IDeA::MarketException, "Unable to recognize swap product type from description: " << description);   
    }

    void SwapMarketSolverCalculator::parseDescription(const string& description, vector<string>& tenors, string& fwdTenor)
    {
        size_t fwd = description.find_first_of("|");
        
        // spot starting products
        if( fwd == string::npos )
        {
            parseDescription(description, tenors);
        }
        // forward starting
        else
        {
            fwdTenor = description.substr(0,fwd);
            string tmp = description.substr(fwd+1);
            parseDescription(tmp, tenors);
        }    
    }
   
    void SwapMarketSolverCalculator::parseDescription(const string& description, vector<string>& tenors)
    {
        // swap
        if(description.size() <= 3)
        {
            tenors.push_back(description);
        }
        else if(description.size() <= 9)
        {
            size_t k = description.find_first_of("y");
            if( k == string::npos )
            {
                k = description.find_first_of("Y");
                if( k == string::npos )
                {
                    LTQC_THROW(IDeA::MarketException, "Unable to deal with swap spread/fly " << description);   
                }
            }
            tenors.push_back( description.substr(0,k+1) );
            string tmp = description.substr(k+1);
            k = tmp.find_first_of("y");
            if( k == string::npos )
            {
                k = tmp.find_first_of("Y");
                if( k == string::npos )
                {
                    LTQC_THROW(IDeA::MarketException, "Unable to deal with swap spread/fly " << description);   
                }
            }
            tenors.push_back( tmp.substr(0,k+1) );

            string tmp1 = tmp.substr(k+1);
            if( tmp1.length() > 1 && tmp1.length() <= 3 )
            {
                tenors.push_back( tmp1 );
            }
        }
        else
        {
            LTQC_THROW(IDeA::MarketException, "description not recognized: " << description);  
        }
    }

    bool SwapMarketSolverCalculator::findConnectedTenors(const vector< pair<string,double> >& portfolio, set<Tenor>& connected)
    {
        size_t k = connected.size();

        for(size_t i = 0; i < portfolio.size(); ++i )
        {   
            string fwdTenor;
            vector<string> tenors;
            parseDescription(portfolio[i].first, tenors, fwdTenor);
            Tenor fwd(fwdTenor);
            
            if(tenors.size() == 1 && fwd.asYearFraction() > 0.0 )
            { 
                 set<Tenor>::const_iterator it1 = connected.find(fwd), it2 = connected.find(Tenor(tenors[0]) + fwd);
                 if( it1 != connected.end() || it2 != connected.end() )
                 {
                     connected.insert(fwd);
                     connected.insert(Tenor(tenors[0]) + fwd);
                 }
            }
            
            if( tenors.size() == 2 )
            { 
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                bool tenor1Short = tenor1 < tenor2;

                const Tenor shortTenor = tenor1Short ? tenor1 : tenor2;
                const Tenor longTenor  = tenor1Short ? tenor2 : tenor1;

                if( fwd.asYearFraction() > 0.0 )
                {
                    set<Tenor>::const_iterator it1 = connected.find(fwd), it2 = connected.find(shortTenor + fwd), it3 = connected.find(longTenor + fwd);
                    if( it1 != connected.end() && it2 != connected.end() || it1 != connected.end() && it3 != connected.end() || it2 != connected.end() && it3 != connected.end())
                    {
                        connected.insert(fwd);
                        connected.insert(shortTenor + fwd);
                        connected.insert(longTenor + fwd);
                    }
                }
                else
                {
                    set<Tenor>::const_iterator it1 = connected.find(shortTenor), it2 = connected.find(longTenor);
                    if( it1 != connected.end() || it2 != connected.end() )
                    {
                        connected.insert(shortTenor);
                        connected.insert(longTenor);
                    }
                }
            }
            
            if(tenors.size() == 3)
            {
                Tenor tenor1 = Tenor(tenors[0]);
                Tenor tenor2 = Tenor(tenors[1]);
                Tenor tenor3 = Tenor(tenors[2]);
                
               
                bool tenor1Short = tenor1 < tenor3;

                const Tenor leftTenor   = tenor1Short ? tenor1 : tenor3;
                const Tenor middleTenor = tenor2;
                const Tenor rightTenor  = tenor1Short ? tenor3 : tenor1;

                if( fwd.asYearFraction() > 0.0 )
                {
                    set<Tenor>::const_iterator it1 = connected.find(fwd), it2 = connected.find(leftTenor + fwd), it3 = connected.find(middleTenor + fwd), it4 = connected.find(rightTenor + fwd);
                    if( it1 != connected.end() && it2 != connected.end() && it3 != connected.end()  ||
                        it1 != connected.end() && it2 != connected.end() && it4 != connected.end()  ||
                        it1 != connected.end() && it3 != connected.end() && it4 != connected.end()  ||
                        it2 != connected.end() && it3 != connected.end() && it4 != connected.end() )
                    {
                        connected.insert(fwd);
                        connected.insert(leftTenor + fwd);
                        connected.insert(middleTenor + fwd);
                        connected.insert(rightTenor + fwd);
                    }
                }
                else
                { 
                    set<Tenor>::const_iterator it1 = connected.find(leftTenor), it2 = connected.find(middleTenor), it3 = connected.find(rightTenor);
                    if( it1 != connected.end() && it2 != connected.end()   ||
                        it1 != connected.end() && it3 != connected.end()   ||
                        it3 != connected.end() && it3 != connected.end() )
                    {
                        connected.insert(leftTenor);
                        connected.insert(middleTenor);
                        connected.insert(rightTenor);
                    }
                }
            }
        }
        return connected.size() > k;
    }

    bool SwapMarketSolverCalculator::isInputPortfolioUnderspecified(const vector< pair<string,double> >& portfolio, const set<Tenor>& allTenors, set<Tenor>& unconnected)
    {
        if ( !portfolio.size() )
            return false;

        set<Tenor> connected;
       
       
        for(size_t i = 0; i < portfolio.size(); ++i )
        {   
            string fwdTenor;
            vector<string> tenors;
            parseDescription(portfolio[i].first, tenors, fwdTenor);
            Tenor fwd(fwdTenor);
            
            if(tenors.size() == 1 && fwd.asYearFraction() <= 0.0 )
            { 
                 connected.insert(tenors[0]);
            }
        }
        
        if( !connected.size() )
        {
            LTQC_THROW(IDeA::MarketException, "At least one swap needed, but none provided" );  
        }
        
        while( findConnectedTenors(portfolio, connected) ) {}
      
        set_difference(allTenors.begin(),allTenors.end(),connected.begin(),connected.end(),inserter(unconnected, unconnected.begin()));
        
        return !unconnected.empty();
    }

    bool SwapMarketSolverCalculator::connectdTenors(const Tenor& tenor1, const Tenor& tenor2, const vector< pair<string,double> >& portfolio)
    {
        if ( !portfolio.size() )
            return false;
        if ( tenor1 == tenor2 )
            return true;
       
        for(size_t i = 0; i < portfolio.size(); ++i )
        {   
            string fwdTenor;
            vector<string> tenors;
            parseDescription(portfolio[i].first, tenors, fwdTenor);
            Tenor fwd(fwdTenor);
            
            if(tenors.size() == 1 && fwd.asYearFraction() > 0.0)
            { 
                 if( (tenor1 == Tenor(tenors[0]) && tenor2 == Tenor(tenors[0]) + fwd) || (tenor2 == Tenor(tenors[0]) && tenor1 == Tenor(tenors[0]) + fwd) )
                     return true;
            }
            if(tenors.size() == 2 && fwd.asYearFraction() <= 0.0)
            { 
                 if( (tenor1 == Tenor(tenors[0]) && tenor2 == Tenor(tenors[1])) || (tenor2 == Tenor(tenors[0]) && tenor1 == Tenor(tenors[1])) )
                     return true;
            }
        }
        return false;
    }
}
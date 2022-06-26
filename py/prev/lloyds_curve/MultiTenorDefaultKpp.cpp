/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "MultiTenorDefaultKpp.h"
#include "MultiTenorModel.h"
#include "NullDeleter.h"
#include "CurveType.h"
#include "RelatedCurveTypes.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

	namespace
	{
		ostream& operator<<(ostream& out, MultiTenorDefaultKpp::InstrumentSetGrid& instrumentSetGrid)
		{
			return instrumentSetGrid.print(out);
		}
	}
    bool MultiTenorDefaultKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                                const BaseModelPtr baseModel)
    {
		const MultiTenorModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorModel>(baseModel));
     
        m_numberOfInputVariables = instruments.size(); // useful when adding knot-points to each curve
        m_baseRate = multiTenorModel->getBaseRate();


        if(!multiTenorModel)
        {
            LT_THROW_ERROR( "Could not cast the BaseModel pointer to a MultiTenorModel pointer in MultiTenorKpp::createKnotPoints(...)" );    
        }
        else
        {
            // Initialize the grid of instrument set
            fillInstrumentSetGrid(instruments);

            // 0. Check the base rate and the discount are in the sparse set grid's headings
            InstrumentSetGrid::XSet curveTypes(m_instrumentSetGrid.getXHeadings());
            if(curveTypes.find(m_baseRate) == curveTypes.end()
               /* || curveTypes.find(CurveType::Discount()) == curveTypes.end()*/ )
            {
                LT_THROW_ERROR("The multi-tenor model does not work if no instrument contributes to the base rate or the discounting rate");
            }

            // put a hook for further treatment, like merging lines whose maturities are "close"

            // Place knot-points:
            //  1. first on the base rate curve
            addKnotPointsToBaseRateCurve(multiTenorModel);

            //  2. then on the discount spread curve (if different from base rate)
            if(m_baseRate != CurveType::Discount())
            {
                addKnotPointsToDiscountSpreadCurve(multiTenorModel);
            }

            //  3. remove some instruments from the grid if there are still too many
            if(m_instrumentSetGrid.set_count() > instruments.size())
            {
                removeInstrumentsFromSetGridCells();
            }

           
            //  3. on the tenor curves of the spread surface
            addKnotPointsToSpreadSurface(multiTenorModel);

            return true;
        }

//        return false;   // no knot-points placed for now
    }
 
    
    void MultiTenorDefaultKpp::fillInstrumentSetGrid(const CalibrationInstruments &instruments)
    {
        m_instrumentSetGrid.clear();
     
        for(CalibrationInstruments::const_iterator instrIter(instruments.begin()); instrIter != instruments.end(); ++instrIter)
        {
            CurveTypes resultingCurveTypes = determineCurveTypes(RelatedCurveTypes::get(*instrIter));
        
            // Add several points (x, y) referencing the instrument, where:
            // 1. x is the curve type(s) the instrument is sensitive to in this model (depending on the chosen primary rate)
            // 2. y is the unique maturity of the instrument
            for(CurveTypes::const_iterator curveTypeIter(resultingCurveTypes.begin()); curveTypeIter != resultingCurveTypes.end(); ++curveTypeIter)
            {
                m_instrumentSetGrid.add(*curveTypeIter, (*instrIter)->getEndDate(), *instrIter);
            }
        }
    }

    // Determine the model curve types an instrument is sensitive to
    // Simple implementation for now, no additions of different curve types
    // Instead, just add the curveType if it is not the base rate
    vector<CurveTypeConstPtr> MultiTenorDefaultKpp::determineCurveTypes(const CurveTypes& relatedCurveTypes) const
    {
        CurveTypes resultingCurveTypes;  // curve types to which we should add a point for this instrument's related curve types
        const CurveTypeConstPtr BaseRate(m_baseRate);
        
        CurveTypes::const_iterator iter(relatedCurveTypes.begin());

        switch(relatedCurveTypes.size())
        {
            // Primary Instruments: Cash and Interest Rate Swap
            case 1:
                resultingCurveTypes.push_back(*iter);
                resultingCurveTypes.push_back(m_baseRate);
                break;
            // Secondary Instruments
            case 2:
                if(find(relatedCurveTypes.begin(), relatedCurveTypes.end(), CurveType::Discount()) != relatedCurveTypes.end())
                {
                    // LTQC::Currency Basis Swap
                    resultingCurveTypes.push_back(*iter);
                    ++iter;
                    resultingCurveTypes.push_back(*iter);
                }
                else
                {
                    // Tenor Basis Swap
                    if(find(relatedCurveTypes.begin(), relatedCurveTypes.end(), m_baseRate) != relatedCurveTypes.end())
                    {
                        // put the curve type that is not the base rate                    
                        if((*iter) == m_baseRate)
                        {
                            ++iter;
                        }
                        resultingCurveTypes.push_back(*iter);
                    }
                    else
                    {
                        // put both curve types
                        resultingCurveTypes.push_back(*iter);
                        ++iter;
                        resultingCurveTypes.push_back(*iter);
                    }
                    
                }
                break;
            default:
                break;
            }
        return resultingCurveTypes;
    }

    void MultiTenorDefaultKpp::addKnotPointsToBaseRateCurve(const MultiTenorModelPtr multiTenorModel) 
    {
        // Don't need to add the (0.0, 0.0) fixed knot-point at the beginning as TypedCurve 
        //  objects take care of it in their constructors by default   

        const double InitBaseRate = 0.05;

        // Get the instruments that contribute to the base rate
        const InstrumentSetGrid::YSetMap PrimaryInstruments(m_instrumentSetGrid.getYSetMap(m_baseRate));

        for(InstrumentSetGrid::YSetMap::const_iterator primIter(PrimaryInstruments.begin());
            primIter != PrimaryInstruments.end();
            ++primIter)
        {
            // Retrieve the first instrument of the set
            CalibrationInstrumentConstPtr instrument(*(primIter->second.begin()));

            const LT::date Maturity(instrument->getEndDate());
            const double MaturityFlow(getTenorFromPeriod(multiTenorModel->getValueDate(), Maturity));    // or use basis?
    
            multiTenorModel->addKnotPoint( m_baseRate, 
                                              KnotPoint(MaturityFlow, MaturityFlow * InitBaseRate, false) );


            // get rid of other references to this instrument in the set grid if we need need to eliminate input variables
            if(m_instrumentSetGrid.set_count() > m_numberOfInputVariables)
            {
                InstrumentSetGrid::XSetMap thisMaturityInstruments(m_instrumentSetGrid.getXSetMap(Maturity));

                for(InstrumentSetGrid::XSetMap::iterator matCurvTypeIter(thisMaturityInstruments.begin());
                    matCurvTypeIter != thisMaturityInstruments.end();
                    ++matCurvTypeIter)
                {
                    if(matCurvTypeIter->first != m_baseRate) // avoid to eliminate the primary rate from grid? useful if we want to add all the points at the end only
                    {
                        set<CalibrationInstrumentConstPtr>::iterator instrPos(matCurvTypeIter->second.find(instrument));
                        if(instrPos != matCurvTypeIter->second.end())
                        {
                            m_instrumentSetGrid.remove(matCurvTypeIter->first, Maturity, *instrPos);
                        }
                    }
                }

            }
        }
    }
    
    void MultiTenorDefaultKpp::addKnotPointsToDiscountSpreadCurve(const MultiTenorModelPtr multiTenorModel) 
    {
        const double InitDiscountSpread(0.0);

        // Leave the discount spread curve flat at 0 if discount is the base rate
        if(m_baseRate != CurveType::Discount())
        {
            const InstrumentSetGrid::YSetMap DiscountingInstruments(m_instrumentSetGrid.getYSetMap(CurveType::Discount()));

            for(InstrumentSetGrid::YSetMap::const_iterator instrIter(DiscountingInstruments.begin());
                instrIter != DiscountingInstruments.end();
                ++instrIter)
            {
                // Retrieve the first instrument of the set
                CalibrationInstrumentConstPtr instrument(*(instrIter->second.begin()));

                const LT::date Maturity(instrument->getEndDate());
                const double MaturityFlow(getTenorFromPeriod(multiTenorModel->getValueDate(), Maturity));    // or use basis?

                // Add the maturity to the discount spread curve
                multiTenorModel->addKnotPoint(CurveType::Discount(), 
                    KnotPoint(MaturityFlow, InitDiscountSpread, false));

                if(m_instrumentSetGrid.set_count() > m_numberOfInputVariables)
                {
                    InstrumentSetGrid::XSetMap thisMaturityInstruments(m_instrumentSetGrid.getXSetMap(Maturity));

                    for(InstrumentSetGrid::XSetMap::iterator matCurvTypeIter(thisMaturityInstruments.begin());
                        matCurvTypeIter != thisMaturityInstruments.end();
                        ++matCurvTypeIter)
                    {
                        if(matCurvTypeIter->first != CurveType::Discount()) // if we want to keep points on the discount curve sparse set grid
                        {
                            set<CalibrationInstrumentConstPtr>::iterator instrPos(matCurvTypeIter->second.find(instrument));
                            if(instrPos != matCurvTypeIter->second.end())
                            {
                                m_instrumentSetGrid.remove(matCurvTypeIter->first, Maturity, *instrPos);
                            }
                        }
                    }
                }   // end of for-loop the other curves the instrument contributes to
            }
        } // end of for-loop on instruments contributing to discount curve
    }
    

    void MultiTenorDefaultKpp::removeInstrumentsFromSetGridCells()
    {
        
        // Remove the cells that have the least instruments on the curves that have the most sets
        m_instrumentSetGrid.getXHeadings();
        InstrumentSetGrid::XSet curveTypes(m_instrumentSetGrid.getXHeadings());
        
        // we avoid removing remove knot points from Base Rate or Discount curves because 
        // in the current design they are already added to these curves
        curveTypes.erase(m_baseRate);
        curveTypes.erase(CurveType::Discount());

        while(m_instrumentSetGrid.set_count() > m_numberOfInputVariables) //  *!*  or  # input variables + # decision dates soon  *!*
        {
            int maxInstrumentSet = 0;
            set<CurveTypeConstPtr> mostPopulatedCurveTypeSet;  // This set will contain the curve types that have the maximum number of sets
        
            for(InstrumentSetGrid::const_x_iterator curveTypeIter(curveTypes.begin()); curveTypeIter != curveTypes.end(); ++curveTypeIter)
            {
                const long ThisInstrumentSetCount(m_instrumentSetGrid.x_set_count(*curveTypeIter));
                
                if((ThisInstrumentSetCount > maxInstrumentSet) && (ThisInstrumentSetCount > 2))
                {
                    maxInstrumentSet = ThisInstrumentSetCount;          //  update the max # of sets
                    mostPopulatedCurveTypeSet.insert(*curveTypeIter);   //  add this curve type to the set that tracks the most populated curve types
                }
            }

            CurveTypeConstPtr eCurveTypeWhereToRemoveFrom;
            // If only one curve type in the set of the most populated, it is from this curve that one cell will be removed
            if(mostPopulatedCurveTypeSet.empty())
            {
                // Do something else that would work
            
            }
            else if(mostPopulatedCurveTypeSet.size() == 1)
            {
                eCurveTypeWhereToRemoveFrom = *(mostPopulatedCurveTypeSet.begin());    
            }
            else
            {
                ////------------------------------------------------------------------------------------------
                // Select the curve that instruments are the least sensitive to, i.e. the one with the least 
                // instruments, adding the number of instruments from each set
                eCurveTypeWhereToRemoveFrom = *(mostPopulatedCurveTypeSet.begin());                  // ----* Take the first one for now, leaving a more evolved selection for later.... *---------
            }

            size_t minInstrumentSetCount = 10000;   // this init value should be enough
            LT::date maturityWhereToRemoveFrom;
            

            // Find the set on the determined curve, that is not on the ends, and with the least instruments in it
            const InstrumentSetGrid::YSetMap matInstrSetMap(m_instrumentSetGrid.getYSetMap(eCurveTypeWhereToRemoveFrom));
            
            InstrumentSetGrid::YSetMap::const_iterator curveTypeIter(matInstrSetMap.begin());
            InstrumentSetGrid::YSetMap::const_iterator endPoint(matInstrSetMap.end());
            --endPoint;     // now points to the last point on curve

            // Loop through all the points on the curve between its ends
            for(++curveTypeIter; curveTypeIter != endPoint; ++curveTypeIter)
            {
                const size_t ThisInstrSetCount(curveTypeIter->second.size());     
                if(ThisInstrSetCount < minInstrumentSetCount)       // new minimum found
                {
                    minInstrumentSetCount = ThisInstrSetCount;          // update minimum
                    maturityWhereToRemoveFrom = curveTypeIter->first;   // store the corresponding maturity
                }
            }

            // Remove it from the intrument set grid
            m_instrumentSetGrid.remove(eCurveTypeWhereToRemoveFrom, maturityWhereToRemoveFrom);
        }
    }

    void MultiTenorDefaultKpp::addKnotPointsToSpreadSurface(const MultiTenorModelPtr multiTenorModel)
    {
        const double InitSurfaceSpreadRate(0.0010); // Spread to initilize the surface with

        const InstrumentSetGrid::XSet CurveTypeSet(m_instrumentSetGrid.getXHeadings());
        
        for(InstrumentSetGrid::XSet::const_iterator curvTypeIter(CurveTypeSet.begin());
            curvTypeIter != CurveTypeSet.end();
            ++curvTypeIter)
        {
            if(*curvTypeIter != CurveType::Discount())
            {
                const InstrumentSetGrid::YSetMap ThisCurveInstruments(m_instrumentSetGrid.getYSetMap(*curvTypeIter));
                for(InstrumentSetGrid::YSetMap::const_iterator matInstrIter(ThisCurveInstruments.begin());
                    matInstrIter != ThisCurveInstruments.end();
                    ++matInstrIter)
                {
                    const double MaturityFlow(getTenorFromPeriod(multiTenorModel->getValueDate(), matInstrIter->first));
                    if(*curvTypeIter == m_baseRate)
                    {
                         // always set the base rate spread curve to 0.0
                        // multiTenorModel->addKnotPoint(*curvTypeIter, KnotPoint(MaturityFlow, 0.0, false));
                    }
                    else
                    {
                        multiTenorModel->addKnotPoint(*curvTypeIter, KnotPoint(MaturityFlow, MaturityFlow * InitSurfaceSpreadRate, false));
                    }
                }
            
            }
        }
    }   // addKnotPointsToSpreadSurface

    /*
    ostream& operator<<(ostream& out, MultiTenorDefaultKpp::InstrumentSetGrid& instrumentSetGrid)
    {
        const MultiTenorDefaultKpp::InstrumentSetGrid::XSet Tenors(instrumentSetGrid.getXHeadings());
        const MultiTenorDefaultKpp::InstrumentSetGrid::YSet Maturities(instrumentSetGrid.getYHeadings());

        for(MultiTenorDefaultKpp::InstrumentSetGrid::const_x_iterator xIter(Tenors.begin()); 
            xIter != Tenors.end(); ++xIter)
            {
                out << "\t" << (**xIter);
            }
            out << endl;

            for(MultiTenorDefaultKpp::InstrumentSetGrid::const_y_iterator yIter(Maturities.begin());
                yIter != Maturities.end(); ++yIter)
            {
                out << (*yIter);
                const MultiTenorDefaultKpp::InstrumentSetGrid::XSetMap xSetMap(instrumentSetGrid.getXSetMap(*yIter));

                for(MultiTenorDefaultKpp::InstrumentSetGrid::const_x_iterator xIter(Tenors.begin()); 
                    xIter != Tenors.end(); ++xIter)
                {
                    MultiTenorDefaultKpp::InstrumentSetGrid::XSetMap::const_iterator setIter(xSetMap.find(*xIter));

                    out << "\t";

                    if(setIter != xSetMap.end())
                    {
                        const MultiTenorDefaultKpp::InstrumentSetGrid::ValueSet valueSet(setIter->second);
                        MultiTenorDefaultKpp::InstrumentSetGrid::ValueSet::const_iterator vIter(valueSet.begin());
                        
                        out << (*vIter);
                        
                        for(++vIter; vIter != valueSet.end(); ++vIter)
                        {
                            out << ", " << (**vIter);
                        }
                    }
                }
                out << endl;
            }
            
            return out; 
        //return instrumentSetGrid.print(out);
    }*/

}   //  FlexYCF

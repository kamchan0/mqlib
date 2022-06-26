/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GENERICIRMARKETDATA_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GENERICIRMARKETDATA_H_INCLUDED
#pragma once

#include "MarketData\IRMarketData.h"
#include "Data\GenericData.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS(GenericIRMarketData)


    // provides a wrapper around the table yield curve
    // data so that the Risk code can operate on the data in the correct
    // way.
    // The way the risk code operates on the underlying points is by manipulating
    // the value within the MarketDataPoints objects which are copied 
    // from the underlying data and when these change as a result of blipping operations
    // the table data which is used by the framework is not refreshed automatically
    // therefore we need to copy the data back before using the underlying table again.
	// the internal state of the object is managed using the m_builtPoints variables
	// this is set to true if any of the risk style object access functions has been called
    class GenericIRMarketData : 
                public IRMarketData,         
				public std::tr1::enable_shared_from_this<GenericIRMarketData>,
                public IClone<GenericIRMarketData>
    {
    public:   
        GenericIRMarketData();
        GenericIRMarketData(LT::date valueDate, const Index& index);

        virtual ~GenericIRMarketData();
        
        virtual void addToPricer(PriceSupplierPtr priceSupplier, std::string constructionMethod = "");
                const std::string &getModelName() const;

        virtual size_t getNumberOfSections() const;
        virtual eSectionType getSectionType(size_t sectionIndex) const;
        virtual const PointList& getList(size_t sectionIndex) const;
        virtual PointList& getList(size_t sectionIndex);
        virtual const DateDoublePointGrid& getDateDoubleGrid(size_t sectionIndex) const;
        virtual DateDoublePointGrid& getDateDoubleGrid(size_t sectionIndex);
        virtual GenericIRMarketDataPtr clone() const;
        virtual eIRSectionType getIRSectionType(size_t sectionIndex) const;
		/// override getData so that we can force syncronization
        virtual const LTQuant::GenericDataPtr &getData() const;
		virtual double getBlipSignMultiplier(size_t sectionIndex) const;
		// *** smelly code warning ***
		// for FlexYCF risk to work with the data being held in the GenericIRMarketData object
		// internally as a GenericData object we need to force sync back to table after blipping
		// otherwise the reset blip is not applied back to the "real" data
		// *** smelly code warning ***
		virtual void syncMarketData();
    private:
        /// copy constructor required as we need cloning
        /// support so that price supplier can work properly for risk
        explicit GenericIRMarketData(const GenericIRMarketData& other);
        void copyDataFromTable() const;
        void copyDataBackToTable() const;

        mutable std::vector<eIRSectionType> m_sectionTypes;
		mutable std::vector<PointList> m_pointLists;
        mutable std::vector<DateDoublePointGrid> m_pointGrids;
        mutable std::vector<double> m_blipSigns;
        mutable bool m_builtPoints;
    };

    DECLARE_SMART_PTRS(GenericIRMarketData)
}

#endif //__LIBRARY_PRICERS_FLEXYCF_GENERICIRMARKETDATA_H_INCLUDED
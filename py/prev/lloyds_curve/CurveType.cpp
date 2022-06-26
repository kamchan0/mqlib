/*****************************************************************************
    
	CurveType

	Implementation of the CurveType class

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "CurveType.h"
#include "NullDeleter.h"
#include "TenorUtils.h"

//	IDeA
#include "DictYieldCurve.h"

using namespace LTQC;

namespace FlexYCF
{
	CurveType::CurveType(const std::string& description, 
                         const double yearFraction,
                         const bool isTenor) :
        m_description(description),
        m_yearFraction(yearFraction),
        m_isTenor(isTenor)
    {
    }

	CurveType::CurveType(const std::string& description,
					     const double yearFraction,
					     const bool isTenor,
						 const IDeA::DictionaryKey& key):
        m_description(description),
        m_yearFraction(yearFraction),
        m_isTenor(isTenor),
		m_key(key)
	{
	}

    double CurveType::getYearFraction() const
    {
        if(m_yearFraction < 0)
        {
			throw std::exception("The year fraction for this curve type is invalid.");
        }
        return m_yearFraction;
    }
    
	CurveTypeConstPtr CurveType::getFromDescription(const std::string& description)
    {
		// Note: cannot build a Null curve type from description
		if(compareDescriptions(description, LT::Str(CurveType::AllTenors()->getDescription())))
			return CurveType::AllTenors();
        if(compareDescriptions(description, LT::Str(CurveType::Discount()->getDescription())))
            return CurveType::Discount();
        // Handles 1D as ON
		if(compareDescriptions(description, LT::Str(CurveType::ON()->getDescription()))
			|| compareDescriptions(FlexYCF::tenorEquivalency(description), LT::Str("1D")))
            return CurveType::ON();
		if(compareDescriptions(description, LT::Str(CurveType::_2D()->getDescription())))
			return CurveType::_2D();
        if(compareDescriptions(description, LT::Str(CurveType::_1W()->getDescription())))
            return CurveType::_1W();
        if(compareDescriptions(description, LT::Str(CurveType::_2W()->getDescription())))
            return CurveType::_2W();
        if(compareDescriptions(description, LT::Str(CurveType::_1M()->getDescription())))
            return CurveType::_1M();
        if(compareDescriptions(description, LT::Str(CurveType::_2M()->getDescription())))
            return CurveType::_2M();
        if(compareDescriptions(description, LT::Str(CurveType::_3M()->getDescription())))
            return CurveType::_3M();
        if(compareDescriptions(description, LT::Str(CurveType::_4M()->getDescription())))
            return CurveType::_4M();
        if(compareDescriptions(description, LT::Str(CurveType::_5M()->getDescription())))
            return CurveType::_5M();
        if(compareDescriptions(description, LT::Str(CurveType::_6M()->getDescription())))
            return CurveType::_6M();
        if(compareDescriptions(description, LT::Str(CurveType::_7M()->getDescription())))
            return CurveType::_7M();
        if(compareDescriptions(description, LT::Str(CurveType::_8M()->getDescription())))
            return CurveType::_8M();
        if(compareDescriptions(description, LT::Str(CurveType::_9M()->getDescription())))
            return CurveType::_9M();
        if(compareDescriptions(description, LT::Str(CurveType::_10M()->getDescription())))
            return CurveType::_10M();
        if(compareDescriptions(description, LT::Str(CurveType::_11M()->getDescription())))
            return CurveType::_11M();
        if(compareDescriptions(description, LT::Str(CurveType::_1Y()->getDescription()))
			|| compareDescriptions(description, LT::Str("12M")))
            return CurveType::_1Y();
        LT_THROW_ERROR( "Invalid curve type description: '" << description << "'." )
    }

    CurveTypeConstPtr CurveType::getFromYearFraction(const double yearFraction)
    {
		std::vector<std::pair<double, CurveTypeConstPtr> >::const_iterator lower(lower_bound(CurveType::Tenors::instance().m_midPoints.begin(), 
                                                                                   CurveType::Tenors::instance().m_midPoints.end(),
                                                                                   yearFraction,
                                                                                   Tenors::CompareMid()));
        return lower->second;
    }

	// Note: the internal order is: Null < All Tenors < Discount < tenor curve types,
	// where tenor curve types are ordered ascendingly
    CurveTypeConstPtr CurveType::Null()
    {
        // Magic invalid year fraction - will throw an exception if attempt to access it
        static const CurveType Null_("Null", -10.0, false);        
        return CurveTypeConstPtr(&Null_, NullDeleter());
    }

	CurveTypeConstPtr CurveType::AllTenors()
	{
		// Magic invalid year fraction - will throw an exception if attempt to access it
        static const CurveType AllTenors_("All Tenors", -2., false);
		return CurveTypeConstPtr(&AllTenors_, NullDeleter());
	}

    CurveTypeConstPtr CurveType::Discount()
    {
        // Magic invalid year fraction - will throw an exception if attempt to access it
        static const CurveType Discount_("Discount", -1.0, false, IDeA_KEY(CURVESINTERPOLATION, FUNDING)); 
        return CurveTypeConstPtr(&Discount_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::ON()
    {
        static const CurveType ON_("ON", 0.004, true, IDeA_KEY(CURVESINTERPOLATION, 1D));            // 1 / 250
        return CurveTypeConstPtr(&ON_, NullDeleter());
    }

    CurveTypeConstPtr  CurveType::_2D()
    {
        static const CurveType _2D_("2D", 0.008, true, IDeA_KEY(CURVESINTERPOLATION, 2D));            // 2 / 250
        return CurveTypeConstPtr(&_2D_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_1W()
    {
        static const CurveType _1W_("1W", 0.019230769230769, true, IDeA_KEY(CURVESINTERPOLATION, 1W));   // 1 / 52
        return CurveTypeConstPtr(&_1W_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_2W()
    {
        static const CurveType _2W_("2W", 0.038461538461539, true, IDeA_KEY(CURVESINTERPOLATION, 2W));   // 2 /52
        return CurveTypeConstPtr(&_2W_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_1M()
    {
        static const CurveType _1M_("1M", 0.083333333333333, true, IDeA_KEY(CURVESINTERPOLATION, 1M));   // 1 / 12
        return CurveTypeConstPtr(&_1M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_2M()
    {
        static const CurveType _2M_("2M", 0.16666666666666649, true, IDeA_KEY(CURVESINTERPOLATION, 2M));   // 2 / 12
        return CurveTypeConstPtr(&_2M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_3M()
    {
        static const CurveType _3M_("3M", 0.25, true, IDeA_KEY(CURVESINTERPOLATION, 3M));    // 3 / 12
        return CurveTypeConstPtr(&_3M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_4M()
    {
        static const CurveType _4M_("4M", 0.333333333333333, true, IDeA_KEY(CURVESINTERPOLATION, 4M));   // 4 / 12
        return CurveTypeConstPtr(&_4M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_5M()
    {
        static const CurveType _5M_("5M", 0.416666666666667, true, IDeA_KEY(CURVESINTERPOLATION, 5M));   // 5 / 12
        return CurveTypeConstPtr(&_5M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_6M()
    {
        static const CurveType _6M_("6M", 0.5, true, IDeA_KEY(CURVESINTERPOLATION,6M )); // 6 / 12
        return CurveTypeConstPtr(&_6M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_7M()
    {
        static const CurveType _7M_("7M", 0.583333333333333, true, IDeA_KEY(CURVESINTERPOLATION, 7M));    // 7 / 12
        return CurveTypeConstPtr(&_7M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_8M()
    {
        static const CurveType _8M_("8M", 0.666666666666667, true, IDeA_KEY(CURVESINTERPOLATION, 8M));    // 8 / 12
        return CurveTypeConstPtr(&_8M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_9M()
    {
        static const CurveType _9M_("9M", 0.75, true, IDeA_KEY(CURVESINTERPOLATION, 9M));    // 9 / 12
        return CurveTypeConstPtr(&_9M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_10M()
    {
        static const CurveType _10M_("10M",  0.833333333333333, true, IDeA_KEY(CURVESINTERPOLATION, 10M));  // 10 / 12
        return CurveTypeConstPtr(&_10M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_11M()
    {
        static const CurveType _11M_("11M", 0.916666666666667, true, IDeA_KEY(CURVESINTERPOLATION, 11M));  // 11 / 12
        return CurveTypeConstPtr(&_11M_, NullDeleter());
    }

    CurveTypeConstPtr CurveType::_1Y()
    {
        static const CurveType _1Y_("1Y", 1.0, true, IDeA_KEY(CURVESINTERPOLATION, 12M));
        return CurveTypeConstPtr(&_1Y_, NullDeleter());
    }

	std::ostream& CurveType::print(std::ostream& out) const
    {
        out << m_description;
        /*if(*this != *Discount() && *this != *Null())
        {
            out << ", year fraction = " << getYearFraction();
        }*/
        return out;
    }
}
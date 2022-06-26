/*****************************************************************************

    CurveType

	CurveType represents the 'type' of a curve, for such types as found in MultiTenorModel.
    In particular, there is a CurveType for each LTQC::Tenor (ON, 1M, 3M...), discount, 
	and special types "Null" and "AllTenors" and usual maturities.

    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CURVETYPE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CURVETYPE_H_INCLUDED
#pragma once

//	#include "CashInstrument.h"

//	LTQuantLib
#include "LTQuantInitial.h"
#include "LT/compare.h"
#include "LT/const_string.h"

//	IDeA
#include "DictionaryManager.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CurveType )


    class CurveType : private DevCore::NonCopyable
    {
    public:
        inline std::string getDescription() const 
        {
            return m_description;
        }

        inline bool isTenor() const
        {
            return m_isTenor;
        }

		inline const IDeA::DictionaryKey& getKey() const
		{
			return m_key;
		}

        double getYearFraction() const;
         
        static CurveTypeConstPtr getFromDescription(const std::string& description);
		
		static inline CurveTypeConstPtr getFromDescription(const LT::Str& description)
		{
			return CurveType::getFromDescription(description.string());
		}

        // Returns the appriopriate (tenor) CurveType according to the 
        // year fraction passed as parameter using the mid-points between
        // two consecutive tenors to find the most appropriate curve type.
        static CurveTypeConstPtr getFromYearFraction(const double yearFraction);

        // Special curve types
        static CurveTypeConstPtr Null();
		static CurveTypeConstPtr AllTenors();
        static CurveTypeConstPtr Discount();

        // Tenors - tenors.push_back any new Tenor to the inner static Tenors object 
        // so that it computes the mid-points between tenors automatically
        static CurveTypeConstPtr ON();
        static CurveTypeConstPtr _2D();
        static CurveTypeConstPtr _1W();
        static CurveTypeConstPtr _2W();
        static CurveTypeConstPtr _1M();
        static CurveTypeConstPtr _2M();
        static CurveTypeConstPtr _3M();
        static CurveTypeConstPtr _4M();
        static CurveTypeConstPtr _5M();
        static CurveTypeConstPtr _6M();
        static CurveTypeConstPtr _7M();
        static CurveTypeConstPtr _8M();
        static CurveTypeConstPtr _9M();
        static CurveTypeConstPtr _10M();
        static CurveTypeConstPtr _11M();
        static CurveTypeConstPtr _1Y();

        
        // Null < AllTenors < Discount < ON < 1W < ..
        static bool LessThan(const CurveType& lhs, const CurveType& rhs)
        {
            return lhs.m_yearFraction < rhs.m_yearFraction;
        }

        static bool Equals(const CurveType& lhs, const CurveType& rhs)
        {
            return lhs.m_yearFraction == rhs.m_yearFraction;
        }
        
        // A inner struct to use to order CurveTypeConstPtr's inside STL containers
        struct DereferenceLess
        {
            bool operator()(const CurveTypeConstPtr lhs, const CurveTypeConstPtr rhs) const
            {
                return CurveType::LessThan(*lhs, *rhs);    
            }
        };

        // Useful for testing:
        std::ostream& print(std::ostream& out) const;

    private:
		static inline bool compareDescriptions(const LT::Str& lhs, const LT::Str& rhs)
		{
			return (0 == LT::compareStrings(lhs, rhs, LT::compare_strings_flag_ignore_whitespace | LT::compare_strings_flag_caseless));
		}

        CurveType(const std::string& description,
                  const double yearFraction,
                  const bool isTenor);
		CurveType(const std::string& description,
                  const double yearFraction,
                  const bool isTenor,
				  const IDeA::DictionaryKey& key);

        const std::string			m_description;
        const double				m_yearFraction;
        const bool					m_isTenor;
		const IDeA::DictionaryKey	m_key;

    public:
        class Tenors
        {
        public:
            typedef std::vector<CurveTypeConstPtr> Container;
            typedef Container::const_iterator const_iterator;
        
        private:
            Container tenors;

        public:
            static const Tenors& instance()
            {
                const static Tenors tenors;
                return tenors;
            }
  
            static const_iterator begin()
            {
                return instance().tenors.begin();
            }

            static const_iterator end()
            {
                return instance().tenors.end();
            }

        private:
            Tenors()
            {
                // ensure the tenors are push in ascending order.
                tenors.push_back(ON());
				tenors.push_back(_2D());
                tenors.push_back(_1W());
                tenors.push_back(_2W());
                tenors.push_back(_1M());
                tenors.push_back(_2M());
                tenors.push_back(_3M());
                tenors.push_back(_4M());
                tenors.push_back(_5M());
                tenors.push_back(_6M());
                tenors.push_back(_7M());
                tenors.push_back(_8M());
                tenors.push_back(_9M());
                tenors.push_back(_10M());
                tenors.push_back(_11M());
                tenors.push_back(_1Y());
              
                // fill midPoints

                for(size_t k(1); k < tenors.size(); ++k)
                {
                    m_midPoints.push_back(
                        std::pair<double, CurveTypeConstPtr>(
                            0.5 * (tenors[k-1]->getYearFraction() + tenors[k]->getYearFraction()), tenors[k-1]
                                            )
                                        );
                }
                m_midPoints.push_back(std::pair<double, CurveTypeConstPtr>(9999.9, tenors[tenors.size() - 1]));
            }
        
        public:
            std::vector<std::pair<double, CurveTypeConstPtr> > m_midPoints; // middle-points between two consecutive tenors
          
            struct CompareMid
            {
            public:
                bool operator()(const std::pair<double, CurveTypeConstPtr> lhs, 
								const std::pair<double, CurveTypeConstPtr> rhs)
                {
                    return operator()(lhs.first, rhs.first);
                }

                bool operator()(const std::pair<double, CurveTypeConstPtr> lhs, const double rhs)
                {
                    return operator()(lhs.first, rhs);
                }

                bool operator()(const double lhs, const std::pair<double, CurveTypeConstPtr> rhs)
                {
                    return operator()(lhs, rhs.first);
                }

            private:
                bool operator()(const double lhs, const double rhs)
                {
                    return lhs < rhs;
                }
            };  // CompareMid

        };  // Tenors

    }; // CurveType


    typedef std::vector<CurveTypeConstPtr> CurveTypes;

	namespace 
	{
		// Null < AllTenors < Discount < ON < 1W < ...
		bool operator<(const CurveType& lhs, const CurveType& rhs)
		{
			return CurveType::LessThan(lhs, rhs);
			//return (lhs.getYearFraction() < rhs.getYearFraction());
		}

		bool operator<(const CurveTypeConstPtr& lhs, const CurveTypeConstPtr& rhs)
		{
			return (*lhs < *rhs);
		}

		bool operator>(const CurveType& lhs, const CurveType& rhs)
		{
			return (rhs < lhs);
		}

		bool operator>(const CurveTypeConstPtr& lhs, const CurveTypeConstPtr& rhs)
		{
			return (*lhs > *rhs);
		}

		bool operator==(const CurveType& lhs, const CurveType& rhs)
		{
			return CurveType::Equals(lhs, rhs); 
		}

		bool operator==(const CurveTypeConstPtr& lhs, const CurveTypePtr& rhs)
		{
			return CurveType::Equals(*lhs, *rhs);
		}

		bool operator!=(const CurveType& lhs, const CurveType& rhs)
		{
			return !CurveType::Equals(lhs, rhs);
		}

		bool operator!=(const CurveTypeConstPtr& lhs, const CurveTypeConstPtr& rhs)
		{
			return !CurveType::Equals(*lhs, *rhs);
		}

		std::ostream& operator<<(std::ostream& out, const CurveType& curveType)
		{
			return curveType.print(out);
		}
	    
		std::ostream& operator<<(std::ostream& out, const CurveTypeConstPtr& curveType)
		{
			return curveType->print(out);
		}
	}
}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_CURVETYPE_H_INCLUDED
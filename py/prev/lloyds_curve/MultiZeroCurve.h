/*****************************************************************************
    Todo: - Add header file description


    @Originator

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/


#ifndef __MultiZeroCurve_H__
#define __MultiZeroCurve_H__

// standard
#include <vector>

#include "Pricers\ZeroCurve.h"
// IDeA
#include "Configuration.h"
#include "FundingPolicy.h"

// QuantCore

namespace FlexYCF
{
    // Forward declarations
    class CloneLookup;
}

namespace IDeA 
{

	IDeA_FWD_DECLARE_SMART_PTRS(MultiZeroCurve);
    class MultiZeroCurve : public LTQuant::ZeroCurve, public std::tr1::enable_shared_from_this<MultiZeroCurve>
    {
    public:

        explicit MultiZeroCurve(LTQuant::PriceSupplier* parent, const FundingPolicyConstPtr& fp);
        virtual ~MultiZeroCurve() {};

        // Realise ZeroCurve interfaces
        virtual double getDiscountFactor(LT::date flowDate) const;
        virtual double getDiscountFactor2(double flowTime) const;
        virtual LTQuant::ZeroCurvePtr clone() const;

        // Disabled methods
        virtual double getTenorDiscountFactor(LT::date flowDate, double tenor) const;
        virtual double getTenorDiscountFactor2(double flowTime, double tenor) const;
        virtual double getForwardRate(LT::date flowDate1, LT::date flowDate2, ModuleDate::DayCounterPtr const& basis) const;
        virtual double getForwardRate2(double flowTime1, double flowTime2, ModuleDate::DayCounterPtr const& basis) const;
        virtual double getStructureFactor(const LT::date& flowDate) const;
        virtual double getStructureFactor(double flowTime) const;
        virtual void getForwardRateDecomposition(const LT::date& flowDate1, const LT::date& flowDate2, const ModuleDate::DayCounterPtr& basis, double& numDf, double& denomDf, double& tenor) const;
        virtual void zeroCurveUpdateNotify(const string& currency, const string& indexName);
        virtual void spotRateUpdateNotify(const string& main, const string& money);
        virtual LTQuant::IRMarketDataPtr getMarketData() const;
        virtual void refresh();

	    FlexYCF::BaseModelPtr getModel() const;
	    LTQuant::GenericDataPtr MultiZeroCurve::getModelParametersData() const;
	    LTQuant::GenericDataPtr MultiZeroCurve::getSpineCurvesDetails() const;
	    ModuleStaticData::IRIndexPropertiesPtr MultiZeroCurve::getMergedIRIndexProperties() const;
		virtual std::vector<std::pair<std::string,double> > discountFactorGradient(LT::date flowDate) const;
    private:

        explicit MultiZeroCurve(const MultiZeroCurve& copyFrom);
        MultiZeroCurve(const MultiZeroCurve& copyFrom, FlexYCF::CloneLookup& lookup);

        MZCCompositionPolicyConstPtr     m_compositionPolicy;
    };
}
#endif

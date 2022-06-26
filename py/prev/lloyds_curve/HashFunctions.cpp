/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Ian Hotchkiss

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ForwardRate.h"
#include "DiscountedForwardRate.h"
#include "DiscountedForwardRateArguments.h"
#include "DiscountFactorArguments.h"
#include "FixedLegArguments.h"
#include "FloatingLegArguments.h"
#include "TenorDiscountFactorArguments.h"
#include "DaysOverBasisArguments.h"
#include "forwardratearguments.h"
#include "InflationIndex.h"

#include "DevCore/Hash.h"

using namespace LTQC;

namespace DevCore {
	template <class Arguments>
	inline void hash_combine(size_t& seed, const FlexYCF::GenericInstrumentComponent<Arguments>& c)
	{
		seed ^= hash_value(c) + DevCore::hash_offset(seed);
	}

	template<>
	inline void hash_combine(size_t& seed, const FlexYCF::ForwardRate& r)
	{
		seed ^= hash_value(r) + DevCore::hash_offset(seed);
	}
}

namespace FlexYCF
{
	using DevCore::hash_combine;

	size_t hash_value(const ForwardRate& forwardRate)
	{
		return hash_value(forwardRate.getArguments());
	}
    
    size_t hash_value(const DiscountedForwardRatePtr discountedForwardRate)
	{
		return hash_value(discountedForwardRate->getArguments());
	}

	size_t hash_value(const DiscountedForwardRateArguments& discountedForwardRateArgs)
	{
		size_t seed(0);
		hash_combine(seed, *(discountedForwardRateArgs.getDiscountFactor()));
		hash_combine(seed, *(discountedForwardRateArgs.getForwardRate()));
		return seed;
	}

	size_t hash_value(const DiscountFactorPtr discountFactor)
	{
		return hash_value(discountFactor->getArguments());
	}
  
    size_t hash_value(const DiscountFactorArguments& discountFactorArgs)
	{
		size_t seed(0);
		hash_combine(seed, discountFactorArgs.getFlowTime());
		hash_combine(seed, discountFactorArgs.getCurrency().data());
        hash_combine(seed, discountFactorArgs.getIndex().data());
		return seed;
	}

	size_t hash_value(const std::pair<double, DiscountFactorPtr>& cvgDfPair)
	{
		size_t seed(0);
		hash_combine(seed, cvgDfPair.first);
		hash_combine(seed, *(cvgDfPair.second));
		return seed;
	}

    size_t hash_value(const TenorDiscountFactorArguments& tenorDiscountFactorArgs)
	{
		size_t seed(0);
		hash_combine(seed, tenorDiscountFactorArgs.getFlowTime());
		hash_combine(seed, tenorDiscountFactorArgs.getTenor());
        hash_combine(seed, tenorDiscountFactorArgs.getCurrency().data());
        hash_combine(seed, tenorDiscountFactorArgs.getIndex().data());
		return seed;
	}
	
    size_t hash_value(const LT::date& date_)
    {
		static std::hash<unsigned int> hasher;
        return hasher(date_.day_number());
    }

	size_t hash_value(const DaysOverBasisArguments& daysOverBasisArguments)
    {
        size_t seed(0);
        hash_combine(seed, daysOverBasisArguments.getStartDate().day_number());
        hash_combine(seed, daysOverBasisArguments.getEndDate().day_number());
        hash_combine(seed, daysOverBasisArguments.getBasis()->getName());   
        return seed;
	}

	size_t hash_value(const ForwardRateArguments& forwardRateArgs)
	{
		size_t seed(0);
		hash_combine(seed, *(forwardRateArgs.getStartDateTenorDiscountFactor()));
		hash_combine(seed, *(forwardRateArgs.getEndDateTenorDiscountFactor()));
		return seed;
	}

	size_t hash_value(const InflationIndexArguments& inflationIndexArgs)
	{
		size_t seed(0);
		hash_combine(seed, inflationIndexArgs.getForward1Time());
		hash_combine(seed, inflationIndexArgs.getForward2Time());
		hash_combine(seed, inflationIndexArgs.getWeight());
		return seed;
	}

	size_t hash_value(const InflationIndexPtr inflationIndex)
	{
		return hash_value(inflationIndex->getArguments());
	}
}
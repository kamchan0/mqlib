/*****************************************************************************
    spine data cache
    @kamingc
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __SPINEDATACACHE_H__
#define __SPINEDATACACHE_H__

#include <vector>
#include <map>
#include "Library/PublicInc/Date.h"
#include "LTQuantInitial.h"

namespace FlexYCF
{
	class SpineDataCache;
	class BaseModel;

	FWD_DECLARE_SMART_PTRS(SpineDataCache)
	FWD_DECLARE_SMART_PTRS(BaseModel)
	typedef std::vector<std::vector<std::pair<double, double>>> knot_points_container;

	class SpineDataCache {
	public:
		BaseModelPtr model_;
		knot_points_container xy_;
		std::vector<Date> dates_;
		std::vector<std::string> instruments_;
		std::vector<double> df_;
		std::vector<SpineDataCachePtr> childern_;
	};
	DECLARE_SMART_PTRS(SpineDataCache)
}



#endif __SPINEDATACACHE_H__
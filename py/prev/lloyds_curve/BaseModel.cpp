/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "BaseModel.h"
#include "BaseKnotPointPlacement.h"
#include "LeastSquaresResiduals.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "DateUtils.h"
#include "NullDeleter.h"
#include "CurveFormulation.h"
#include "FlexYCFCloneLookup.h"
#include "SpineDataCache.h"

// #include "UtilsEnums.h"
#include "FactoryEnvelopeH.h"
#include "YieldCurve.h"
#include "FXForward.h"
#include "LegacyToyModel.h"
#include "QLZeroCurve.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "TblConversion.h"

// LTQC
#include "QCUtils.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace FlexYCF
{
    using namespace LTQuant;


    BaseModel::BaseModel()
    {
     	initializeLeastSquaresResiduals();
	}

	BaseModel::BaseModel(const LT::date& valueDate) :
        m_valueDate(valueDate), m_parent(0)
    {
		initializeLeastSquaresResiduals();
    }

	BaseModel::BaseModel(const LTQuant::GenericData& masterTable, const FlexYCFZeroCurvePtr parent):
		StructureSurfaceHolder(masterTable), m_parent(parent)
	{
		initializeLeastSquaresResiduals();
        const GenericDataPtr curveDetailsTable(IDeA::extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        m_valueDate = IDeA::extract<LT::date>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE));

		const GenericDataPtr curveParametersTable(IDeA::extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
        IDeA::permissive_extract<GenericDataPtr>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, YC_DEPENDENTMARKETDATA), m_dependentMarketData);
        IDeA::permissive_extract<GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, DEPENDENCIES), m_dependencies);
		
		std::string emptyStr, asset, market;
		IDeA::permissive_extract<std::string>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS, ASSET), asset, emptyStr);
		IDeA::permissive_extract<std::string>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS, MARKET), market, emptyStr);
		if(!asset.empty() && !market.empty())
		{
			m_primaryAssetDomainFunding.reset(new IRAssetDomain(LTQC::Currency(asset.c_str()), market.c_str()));
			m_primaryAssetDomainIndex   = m_primaryAssetDomainFunding;
		}

        m_isJacobianSupported = true;
	}

    BaseModelPtr BaseModel::getZeroCurveModel(const ZeroCurvePtr zc)
    {
        // returns the model embedded in the curve
        BaseModelPtr bm;

        FlexYCFZeroCurvePtr flexYcfZeroCurve(std::tr1::dynamic_pointer_cast<FlexYCFZeroCurve>(zc));
        ZeroCurveIFPtr zi(new QLZeroCurve(zc));

        if(flexYcfZeroCurve)
        {
            bm = flexYcfZeroCurve->getModel();            
        }
        else
        {
            bm.reset(new FlexYCF::LegacyToyModel(zi));
        }

        return bm;
    }

    /**
        @brief Pseudo copy constructor with lookup for clones.

        Essentially a copy constructor but uses a lookup to ensure that directed graph relationships are maintained.

        Currently the knot point placement algorithm (m_knotPointPlacement) is shared between original and clone. This is acceptable as 
        it has no state and only describes behavior. This decision may need reviewing if the complexity of knot point placement increases.

        @param original The original instance to create a copy of.
        @param lookup   A lookup for previously created clones.
    */
    BaseModel::BaseModel(BaseModel const& original, CloneLookup& lookup) :
        StructureSurfaceHolder(static_cast<const StructureSurfaceHolder&>(original)), 
        m_valueDate(original.m_valueDate), 
        m_knotPointPlacement(original.m_knotPointPlacement),
        m_jacobian(original.m_jacobian),
        m_dependentMarketData(original.m_dependentMarketData),
		m_dependencies(original.m_dependencies),
        m_isJacobianSupported(original.m_isJacobianSupported),
		m_dependentModels(original.m_dependentModels),
		m_dependentFXRates(original.m_dependentFXRates),
		m_primaryAssetDomainFunding(original.m_primaryAssetDomainFunding),
		m_primaryAssetDomainIndex(original.m_primaryAssetDomainIndex)
    {
        // Contained instances may be referring back to use, using NullDeleter
        lookup.allowNullDeleter(&original, this);

        // These instances may refer back to the base model
        m_leastSquaresResiduals = lookup.get(original.m_leastSquaresResiduals);
        m_fullInstruments.assign(original.m_fullInstruments, lookup);
        m_parent = lookup.get(original.m_parent);
    }

	double BaseModel::getTenorDiscountFactor(double flowTime, double tenor, const LTQC::Currency& ccy, const LT::Str& index) const
	{
		
		USE(ccy)
		USE(index)
		return getTenorDiscountFactor(flowTime,tenor);
	}
  
	bool BaseModel::hasDependentIRMarketData(const LT::Str& currency, const LT::Str& index) const
    {
        bool exists = false;
        if(m_parent && hasDependentMarketData())
        {
            for (size_t i = 1; i < m_dependentMarketData->table->rowsGet(); ++i) 
            {
                AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
                LT::Str market = IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1); 

                if (adType == IDeA::AssetDomainType::IR )
                {
                    if ((asset.compareCaseless(currency) == 0) && (market.compareCaseless(index) == 0))
                    {
                        exists = true;
                        break;
                    }
                }
            }
        }
        return exists;
    }

    bool BaseModel::hasDependentFXSpotMarketData(const LT::Str& fgnCcy, const LT::Str& domCcy) const
    {
        bool exists = false;
        if(m_parent && hasDependentMarketData())
        {
            for (size_t i = 1; i < m_dependentMarketData->table->rowsGet(); ++i) 
            {
                AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
                LT::Str market = IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1); 

                if (adType == IDeA::AssetDomainType::FXSPOT )
                {
                    if (((asset.compareCaseless(fgnCcy) == 0) && (market.compareCaseless(domCcy) == 0)) || 
                        ((asset.compareCaseless(domCcy) == 0) && (market.compareCaseless(fgnCcy) == 0)))
                    {
                        exists = true;
                        break;
                    }
                }
            }
        }
        return exists;
    }

    BaseModelPtr BaseModel::resolveYieldCurveModel(AssetDomainConstPtr ad)
    {
        // resolve the given yield curve
        BaseModelPtr bm;

        // resolve from the price supplier market-data
        size_t id = m_parent->getParent()->getLookupValue(ad->primaryDomain().data(), ad->secondaryDomain().data());
        ZeroCurvePtr zc = m_parent->getParent()->getZeroCurve(id);

        if(zc)
        {
            bm = BaseModel::getZeroCurveModel(zc);
        }
        else
        {
            LTQC_THROW( ModelException, "Yield Curve " <<  ad->discriminator().data() << " not found");
            
        }

        return bm;
    }

    double BaseModel::resolveFXSpotRates(AssetDomainConstPtr ad)
    {
        // resolve the given yield curve
        double fxRate = 0;
        bool exists = false;
        // resolve from the price supplier market-data
        exists = m_parent->getParent()->hasFXLookupValue(ad->primaryDomain().data(), ad->secondaryDomain().data());
        if(exists)
        {
            fxRate = m_parent->getParent()->getFXSpotRate(ad->primaryDomain().data(), ad->secondaryDomain().data());
        }
        else
        {
            LTQC_THROW( ModelException, "FXSpot " <<  ad->discriminator().data() << " is not available in market data");
        }
        return fxRate;
    }

    void BaseModel::prepareForSolve()
    {
        // here we iterate through the list of dependent market data and create the dependent models
        if(m_parent && hasDependentMarketData())
        {
            m_dependentModels.clear();
            m_dependentFXRates.clear();

            for (size_t i = 1; i < m_dependentMarketData->table->rowsGet(); ++i) 
            {
                AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
                LT::Str market = IDeA::extract<LT::Str>(m_dependentMarketData->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1); 

                if (adType == IDeA::AssetDomainType::IR )
                {
                    AssetDomainConstPtr ad(new IRAssetDomain(asset, market));                    
                    BaseModelPtr bm = resolveYieldCurveModel(ad);
                    m_dependentModels[ad] = bm;
                }
                else if (adType == IDeA::AssetDomainType::FXSPOT )
                {
                    AssetDomainConstPtr ad(new FXSpotAssetDomain(asset, market));                    
                    double fxRate = resolveFXSpotRates(ad);
                    m_dependentFXRates[ad] = fxRate;

                    // also store the inverse
                    AssetDomainConstPtr adInv(new FXSpotAssetDomain(market, asset));                    
                    double fxRateInv = resolveFXSpotRates(adInv);
                    m_dependentFXRates[adInv] = fxRateInv;
                }
                else
                {
                    LTQC_THROW( IDeA::ModelException, "Dependent market data of type " << adType.asString().data() << " is not supported" );
                }
            }
        }
    }


	void BaseModel::initializeLeastSquaresResiduals()
	{
		m_leastSquaresResiduals = LeastSquaresResidualsPtr(new LeastSquaresResiduals(BaseModelPtr(this, NullDeleter())));
	}

    LT::date BaseModel::getValueDate() const
    {
        return m_valueDate;
    }

    LeastSquaresResidualsPtr BaseModel::getLeastSquaresResiduals() const
    {
        return m_leastSquaresResiduals;
    }

    void BaseModel::setKnotPointPlacement(const BaseKnotPointPlacementPtr kppPtr)
    {
        m_knotPointPlacement = kppPtr;
    }

    BaseKnotPointPlacementPtr BaseModel::getKnotPointPlacement() const
    {
        return m_knotPointPlacement;
    }

    bool BaseModel::placeKnotPoints(CalibrationInstruments& instruments)
    {
		m_fullInstruments = instruments;
		
		//	Make sure the full instruments are marked as not placed: 
		instruments.setAsPlaced(false);

        // first filter the instrument list to remove any 
        // instrument that we won't solve for
        m_knotPointPlacement->selectInstruments(instruments, BaseModelPtr(this, NullDeleter()));

		//	At this point only those instruments that have been 
		//	placed on a curve remain. Set them as placed:
		instruments.setAsPlaced(true);

        const bool kppResult( m_knotPointPlacement->createKnotPoints(instruments, BaseModelPtr(this, NullDeleter())) );

        // logically, onKnotPointsPlaced should only be called when the placement did succeed!
        if(kppResult)
        {
            if(static_cast<bool>(m_leastSquaresResiduals))
            {
                m_leastSquaresResiduals->addInstrumentResiduals(instruments); // added with the new LSResiduals design:
				m_knotPointPlacement->onLeastSquaresResidualsAdded(*m_leastSquaresResiduals);
            }

            onKnotPointsPlaced();
        }
        return kppResult;
    }

	void BaseModel::setValueDate(const LT::date valueDate)
	{
		m_valueDate = valueDate;
	}

	LTQuant::GenericDataPtr BaseModel::getSpineCurvesDetails() const
	{
		const LTQuant::GenericDataPtr defaultSpineCurvesDetailsData(new LTQuant::GenericData("Default Spine Curves Data", 0));
		//	defaultSpineCurvesDetailsData->set<std::string>(
		return defaultSpineCurvesDetailsData;
	}

	void BaseModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem, IKnotPointFunctor&)
	{
		// by default, just add variables to problem
		addVariablesToProblem(problem);
	}

    std::ostream& BaseModel::print(std::ostream& out) const
    {
        out << "BaseModel.";
        return out;;
    }

    LT::TablePtr BaseModel::getJacobian( const bool includeHeadings ) const
    {
        LT::TablePtr initialJacobian = LTQC::TblConversion_toTbl( getJacobian( ) );
        LT::TablePtr jacobianWithPotentialHeadings;
        if( !includeHeadings )
        {
            jacobianWithPotentialHeadings = initialJacobian;
        }
        else
        {
            const size_t nJacobianRows = initialJacobian->rowsGet( );
            jacobianWithPotentialHeadings.reset( new LT::Table( nJacobianRows + 1, initialJacobian->colsGet( ) + 1 ) );
            jacobianWithPotentialHeadings->at( 0, 0 ) = LT::Str( "Instruments vs Model unknowns" );
            size_t nPlacedInstruments( 0 );
            
            for( size_t j = 0; j < nJacobianRows; ++j )
            {
                jacobianWithPotentialHeadings->at( 0, j + 1 ) = LT::Str( "y" + LTQC::convertIntegerToString( static_cast<int>(j + 1) ) );
            }
            
            for( size_t i = 0; i < m_fullInstruments.size( ) && nPlacedInstruments < nJacobianRows; ++i )
            {
                const CalibrationInstrumentConstPtr& instrument = m_fullInstruments[ i ];
                if( instrument->wasPlaced( ) )
                {
                    jacobianWithPotentialHeadings->at( nPlacedInstruments + 1, 0 ) = LT::Str( instrument->getName( ) + " " + instrument->getDescription( ) );
                    for( size_t j = 0; j < nJacobianRows; ++j )
                    {
                        jacobianWithPotentialHeadings->at( nPlacedInstruments + 1, j + 1 ) = initialJacobian->at( nPlacedInstruments, j );
                    }
                    ++nPlacedInstruments;
                }
            }

		    if( nPlacedInstruments != nJacobianRows )
		    {
                const int iPlacedInstruments = static_cast< int >( nPlacedInstruments );
                const int iJacobianRows = static_cast< int >( nJacobianRows );
                LTQC_THROW( IDeA::ModelException, "The number of placed partial instruments (" << LTQC::convertIntegerToString( iPlacedInstruments ).data( ) << ") is not equal to the number of rows in the Jacobian (" << LTQC::convertIntegerToString( iJacobianRows ).data( ) << ")." );
	        }
        }
        return jacobianWithPotentialHeadings;
    }

    BaseModelConstPtr BaseModel::getDependentModel(const AssetDomain& id) const
    {
        AssetDomainConstPtr ptrId = LT::refPtr(id);
        StrBMMap::const_iterator it = m_dependentModels.find(ptrId);
        if (it == m_dependentModels.end()) 
        {
            LTQC_THROW( IDeA::ModelException, "Unable to find dependent model for " << id.discriminator().data() );
        }

        return it->second;
    }
    
    bool BaseModel::hasDependentModel(const AssetDomain& id) const
    {
        bool exists = true;
        AssetDomainConstPtr ptrId = LT::refPtr(id);
        StrBMMap::const_iterator it = m_dependentModels.find(ptrId);
        if (it == m_dependentModels.end()) 
        {
            exists = false;
        }

        return exists;
    }
	 
	bool BaseModel::hasDependentModel(const string& ccy, const string& index) const
	{
		string tmp;
		IRAssetDomain::buildDiscriminator(ccy,index,tmp);
		return hasDependentModel(*AssetDomain::createAssetDomain(LT::Str(tmp)));
	}
    
	double BaseModel::getDependentFXRate(const AssetDomain& id) const
    {
        AssetDomainConstPtr ptrId = LT::refPtr(id);
        StrFXRateMap::const_iterator it = m_dependentFXRates.find(ptrId);
        if (it == m_dependentFXRates.end()) 
        {
            LTQC_THROW( IDeA::ModelException, "Unable to find dependent fx spot rate " << id.discriminator().data() );
        }

        return it->second;
    }

    void BaseModel::finishCalibration()
    {
        //once calibration is done cleanup the residuals, instruments and knot points
        //as they hold instrument ptrs which reference GlobalCache elements we no longer need
        m_leastSquaresResiduals->finishCalibration();

        for_each(m_fullInstruments.begin(),m_fullInstruments.end(),[&](CalibrationInstrumentPtr& it)
            //as we can't create a shared_ptr from this ensure that the sharaed_ptr wrapper created below does not destroy us
        {it->finishCalibration(BaseModelPtr(this,NullDeleter()));});
		setCalibrated();
    }

	void BaseModel::restoreModelSpineData(SpineDataCachePtr& sdp) {
		sdp->model_->assignSpineInternalData(sdp);
		sdp->model_->update();
		for (auto p = sdp->childern_.begin(); p != sdp->childern_.end(); ++p)
			BaseModel::restoreModelSpineData(*p);
	}

}   //  FlexYCF
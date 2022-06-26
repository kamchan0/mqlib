/*****************************************************************************
    
	FlexYCFZeroCurve

	Implementation of FLexYCFZeroCurve


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

//	FlexYCF
#include "FlexYCFZeroCurve.h"
#include "GenericIRMarketData.h"
#include "BaseModel.h"
#include "BaseModelFactory.h"
#include "InflationModel.h"
#include "CalibrationInstruments.h"
#include "CalibrationInstrumentFactory.h"
#include "BaseKnotPointPlacement.h"
#include "KnotPointPlacementFactory.h"
#include "BaseSolver.h"
#include "BaseInitialization.h"
#include "InitializationFactory.h"
#include "ConstantInitialization.h"
#include "InflationInitialization.h"
#include "SolverFactory.h"
#include "Timer.h"
#include "GlobalComponentCache.h"
#include "InstrumentComponent.h"
#include "NullDeleter.h"
#include "FuturesConvexityModel.h"
#include "Futures.h"
#include "ModelSelection.h"
#include "FlexYCFCloneLookup.h"
#include "SpineDataCache.h"

//	LTQuantLib
#include "Maths/LevenbergMarquardtSolver.h"
#include "DateUtils.h"
#include "Maths/CurveInterp.h"
#include "Pricers/ZeroCurve.h"
#include "Pricers/PriceSupplier.h"
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"

// ModuleStaticData
#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

    namespace 
    {
        string getDefaultModelName(const GenericDataPtr masterTable, 
                                   const string& /* constructionMethod */)
        {
            GenericDataPtr detailsData(masterTable->get<GenericDataPtr>("Curve Details", 0));
            string type(detailsData->get<string>("Type", 0));
            if(_stricmp(type.c_str(), "INFLATION") == 0 || _stricmp(type.c_str(), "IL") == 0)
            {
                return "Inflation";
            }
            return "Stub";
        }

        string getDefaultKppName(const string& /* modelName */, 
                                 const LTQuant::GenericDataPtr /* masterTable */, 
                                 const string& /* constructionMethod */)
        {
            return "SingleCurveDefault";
        }

        string getDefaultSolverName(const string& /* modelName */, 
                                    const LTQuant::GenericDataPtr /* masterTable */, 
                                    const string& /* constructionMethod */)
        {
            return LTQuant::LevenbergMarquardtSolver::getName();
        }
        
		LTQC::VectorDouble createShockedRates(const FlexYCF::CalibrationInstruments& oldPartialInstruments,
											  const LTQuant::GenericDataPtr& newMasterTable)
		{
			using namespace FlexYCF;

			const LTQuant::GenericDataPtr instrumentListData(IDeA::extract<LTQuant::GenericDataPtr>(*newMasterTable, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST)));

			LTQC::VectorDouble shockedRates(oldPartialInstruments.size());

			// Get the rates of the target curve
			//	Remember existing cache to set it back later (create a RAII class for that, for proper handling when exceptions occur)
			GlobalComponentCache* const existingCache(InstrumentComponent::getGlobalComponentCache());
			
			//	Create a new cache:
			const GlobalComponentCache cache(GlobalComponentCache::createCache(newMasterTable));

			//	Update the instrument list with the shifted rates
			//CalibrationInstrumentFactory::loadInstrumentList(shockedInstruments, shiftedMasterTable, cache); 
			
			for(size_t k(0); k < oldPartialInstruments.size(); ++k)
			{
				//  Find the corresponding instruments in the instrument list table
				//	and set the rate difference to shockedRates
				shockedRates[k] = oldPartialInstruments[k]->getDifferenceWithNewRate(*instrumentListData);
			}

			//	Set the cache back to what it was
			InstrumentComponent::setGlobalComponentCache(existingCache);

			return shockedRates;
		}
    }
}

using namespace ModuleDate;

namespace LTQuant
{
    using namespace FlexYCF;

    FlexYCFZeroCurve::FlexYCFZeroCurve(PriceSupplier* parent, GenericIRMarketDataPtr marketData, const string& constructionMethod, ModuleStaticData::IRIndexPropertiesPtr indexProp) : 
        ZeroCurve(parent, marketData->getValueDate(), indexProp),
        m_marketData(marketData),
        m_constructionMethod(constructionMethod),
		m_requiresRebuildFromData(true),
        m_calibInstrumentsExist(false),
		m_refreshType(FlexYCF::FromSolver),
		m_requiresRefreshFromData(false),
        m_lightweightClone(false)
    {
    }

	// copy contructor
	// used from the clone method 
	// mostly do a shallow copy to keep things fast but we do have to rebuild the instrument
	// list as we need to have the different rates
	FlexYCFZeroCurve::FlexYCFZeroCurve(const FlexYCFZeroCurve& copyFrom) :
		ZeroCurve(copyFrom),
        m_marketData(copyFrom.m_marketData),
        m_constructionMethod(copyFrom.m_constructionMethod),
		m_requiresRebuildFromData(true),
        m_calibInstrumentsExist(false),
		m_refreshType(FlexYCF::FromSolver),
		m_requiresRefreshFromData(false),
        m_lightweightClone(true)
	{
		// as we are sharing data, it's safer to rebuild instruments at this point rather than using lazyInit
        //rebuildCurveFromData();
		//m_requiresRebuildFromData = false;
	}

    /**
        @brief Create a clone.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param lookup A lookup of previously created clones.
        @return       A clone of this instance.
    */
    FlexYCF::ICloneLookupPtr FlexYCFZeroCurve::cloneWithLookup(FlexYCF::CloneLookup& lookup) const
    {
        LT_THROW_ERROR("Should never come here");
    }

    /**
        @brief Get the pillar points used in constructing the curve, ie the unfixed spine curve knotpoints

        @param[out] pillarPoints The list of sorted unique pillar points
    */
    
    void FlexYCFZeroCurve::getPillarPoints(list<double>& pillarPoints) const
    {
        pillarPoints.clear();
        lazyInit();
        m_model->getSpineCurvesUnfixedKnotPoints(pillarPoints);
        pillarPoints.sort();
        pillarPoints.unique();
    }
    /**
        @brief Create a clone of this curve.

        This clone is only deep-enough to enable the rates to be manipulated in the clone without influencing the
        original instance. A lookup is used to ensure that the directed graph relationships of the original curve
        are retained in the clone.

        The clone will create its own copy of the solver from scratch. This assumes the solver is not linked to the other 
        member data in anyway (which it currently isn't). If this design changes then the solver will need a full clone as well.
    */
    FlexYCFZeroCurve::FlexYCFZeroCurve(FlexYCFZeroCurve const& copyFrom, FlexYCF::CloneLookup& lookup) :
        ZeroCurve(copyFrom),        
        m_marketData(copyFrom.m_marketData),
        m_constructionMethod(copyFrom.m_constructionMethod),
        m_fullInstruments(lookup.get(copyFrom.m_fullInstruments)),
        m_partialInstruments(lookup.get(copyFrom.m_partialInstruments)),
        m_requiresRebuildFromData(false),
        m_calibInstrumentsExist(false),
        m_refreshType(copyFrom.m_refreshType),
		m_requiresRefreshFromData(false),
        m_lightweightClone(true)
    {
        lookup.allowNullDeleter(&copyFrom, this);
        m_model = lookup.get(copyFrom.m_model);

		GenericDataPtr const masterTable(m_marketData->getData());

        // Create a new solver instance for the clone to use. We will create this directly from the generic data. Note that buiding the yield
        // curve modifies the generic data so we will be guaranteed to have a model name.
        string modelName; 
		bool exists = IDeA::permissive_extract<std::string>(*masterTable, IDeA_KEY(YIELDCURVE, MODEL), modelName, string());
        if (!exists)
            LT_THROW_ERROR("Expecting '" << IDeA_KEY(YIELDCURVE, MODEL).getName().cStr() << "' to be populated when cloning FlexYCF");

        // Need the model parameters table. Note that build the yield curve modifies the generic data so we will be guaranteed to have a model
        // parameters table.
		GenericDataPtr modelParametersTable;
        exists = IDeA::permissive_extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS), modelParametersTable);
        if (!exists)
            LT_THROW_ERROR("Expecting '" << IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS).getName().cStr() << "' to be populated when cloning FlexYCF");

        // Work out the default solver name
        string const defaultSolverName(getDefaultSolverName(modelName, masterTable, m_constructionMethod));
        string solverName(defaultSolverName);
		IDeA::permissive_extract<std::string>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, SOLVER), solverName, defaultSolverName);

		// Retrieve solver parameters
		GenericDataPtr solverParamsTable;
		IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, SOLVERPARAMETERS), solverParamsTable);

		// Build solver for this clone to use
        m_solver = SolverFactory::createSolver(solverName, solverParamsTable);    

        // Irrelevant what value is used here as it is overwritten in the next solveCurve invocation
        m_solver->setState(BaseSolver::FIRST_SOLVING);
		finishCalibInstruments();
		InstrumentComponent::setGlobalComponentCache(0);
    }

	// similar to BaseModelFactory::createBaseModel
    void FlexYCFZeroCurve::rebuildCurveFromData()
    {

 		LTQuant::Timer firstTotalBuildTimer;
		firstTotalBuildTimer.start();
        

        m_calibInstrumentsExist=true;
 		LTQuant::Timer firstSolvingTimerBuildData;
		firstSolvingTimerBuildData.start();
        
		const GenericDataPtr masterTable(m_marketData->getData());
        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
       
		GenericDataPtr modelParametersTable;
        const GenericDataPtr curveDetailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        GenericDataPtr instrumentsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST)));
        const LT::date buildDate(IDeA::extract<LT::date>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS,BUILDDATE)));
		
        // ----------------------------------------------------------------------------------------------------------------------
        // -1. Create model params
        // ----------------------------------------------------------------------------------------------------------------------
        // ** Retrieve the name of the model from combination of construction method and data **
       
        // Retrieve the model name if specified in the master table.
        // If not, use the default model name for now to ensure backward compatibility

        string modelName; 
		const bool modelNameSpecified(IDeA::permissive_extract<std::string>(*masterTable, IDeA_KEY(YIELDCURVE, MODEL), modelName, string()));

        if(!modelNameSpecified)
        {
            modelName = getDefaultModelFromInstrumentListData(instrumentsTable);
			IDeA::inject<std::string>(*masterTable, IDeA_KEY(YIELDCURVE, MODEL), modelName);
			// new impl. we set the model name to the params table if none is specified
            //  for the cached model to be able to build a this default model
        }
        const bool modelParamsSpecified(IDeA::permissive_extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS), modelParametersTable));

        if (!modelParamsSpecified) 
		{
			// set defaults from model selection
			modelParametersTable = getDefaultParametersFromModelName(modelName, curveDetailsTable, instrumentsTable);
			IDeA::inject<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS), modelParametersTable);    
		}
		LTQuant::Timer timer;

        // ----------------------------------------------------------------------------------------------------------------------
        // 0. Create the GlobalComponentCache here and use it in the CalibrationInstrumentFactory::loadInstrumentList function
        // ----------------------------------------------------------------------------------------------------------------------
		
		GlobalComponentCache* gcc = InstrumentComponent::getGlobalComponentCache();
		GlobalComponentCache globalComponentCache = GlobalComponentCache::createCache(masterTable, buildDate);
		InstrumentComponent::setGlobalComponentCache(&globalComponentCache);

        // ----------------------------------------------------------------------------------------------------------------------
        // 1. Load the calibration instruments
        // ----------------------------------------------------------------------------------------------------------------------
        m_fullInstruments.reset(new CalibrationInstruments);
        CalibrationInstrumentFactory::loadInstrumentList(*m_fullInstruments, 
                                                         masterTable, 
                                                         globalComponentCache,
                                                         getParent());
    
        m_partialInstruments.reset(new CalibrationInstruments(*m_fullInstruments));

        // ----------------------------------------------------------------------------------------------------------------------
        // 2. Create model
        // ----------------------------------------------------------------------------------------------------------------------
        // Create the model as specified by the name
        m_model = ModelFactory::createModel(modelName, masterTable, FlexYCFZeroCurvePtr(this, ModuleDate::NullDeleter()));
        
        // ----------------------------------------------------------------------------------------------------------------------
        // 3. prepare model for the solver
        // ----------------------------------------------------------------------------------------------------------------------
        m_model->prepareForSolve();

        // ----------------------------------------------------------------------------------------------------------------------
        // 4. Create the knot-point selection algorithm, set it to the model (or do that within the model factory) ...
        // ----------------------------------------------------------------------------------------------------------------------
        string kppName;
        if(modelParametersTable->doesTagExist(IDeA_PARAM(FLEXYC_MODELPARAMETERS, KPP)))
        {
			kppName = IDeA::extract<std::string>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, KPP));
		}
        else
        {
            kppName = getDefaultKppName(modelName, masterTable, m_constructionMethod);
        }
        const BaseKnotPointPlacementPtr knotPointPlacement(KnotPointPlacementFactory::createKnotPointPlacement(kppName, masterTable));

        m_model->setKnotPointPlacement(knotPointPlacement);
          
        // ... and place knot-points
        m_model->placeKnotPoints(*m_partialInstruments);   
 
		// ----------------------------------------------------------------------------------------------------------------------
        // 5. Finalize the model
        // ----------------------------------------------------------------------------------------------------------------------
		m_model->finalize();
	

		// ----------------------------------------------------------------------------------------------------------------------
        // 6. Create the initialization
        // ----------------------------------------------------------------------------------------------------------------------
		LTQuant::Timer firstSolvingTimer;
		firstSolvingTimer.start();

		//	Retrieve initialization name
		const string defaultInitializationName(std::tr1::dynamic_pointer_cast<InflationModel>(m_model) ? 
												InflationInitialization::getName() :
												ConstantInitialization::getName() );	// to be replaced with toolkits
		string initializationName(defaultInitializationName);
		
		IDeA::permissive_extract<std::string>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, INITIALIZATION), initializationName, defaultInitializationName);
		
		//	Retrieve initialization parameters
		GenericDataPtr initializationParamsTable;
		
		IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, INITIALIZATIONPARAMETERS), initializationParamsTable);

		//	Build initialization
		BaseInitializationPtr initialization(InitializationFactory::createInitialization(initializationName, initializationParamsTable));

		LT_LOG << "Initializing with initialization named " << initializationName << std::endl;
		initialization->initialize(m_model.get());
		
        // ----------------------------------------------------------------------------------------------------------------------
        // 7. Create the solver
        // ----------------------------------------------------------------------------------------------------------------------
        // Retrieve solver name
        const string defaultSolverName(getDefaultSolverName(modelName, masterTable, m_constructionMethod));
        string solverName(defaultSolverName);
		IDeA::permissive_extract<std::string>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, SOLVER), solverName, defaultSolverName);

		// Retrieve solver parameters
		GenericDataPtr solverParamsTable;
		IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, SOLVERPARAMETERS), solverParamsTable);

		// Build solver
        m_solver = SolverFactory::createSolver(solverName, solverParamsTable);    

        m_solver->setState(BaseSolver::FIRST_SOLVING);
		firstSolvingTimerBuildData.stop();

		LT_LOG << "First build from data for " << m_marketData->getIndexName() << " in " << firstSolvingTimerBuildData.getMilliseconds() << " ms" << endl;
		
        solveCurve();
		m_model->setCalibrated();
        calibrationInstrumentSetValues();
        // Set default refresh type
        m_refreshType = m_model->isJacobianSupported() ? FromSolver : FullRebuild;
        
        firstTotalBuildTimer.stop();
		firstSolvingTimer.stop();
		LT_LOG << "First solving for " << m_marketData->getIndexName() << " in " << firstSolvingTimer.getMilliseconds() << " ms" << endl;
		LT_LOG << "First TOTAL build for " << m_marketData->getIndexName() << " in " << firstTotalBuildTimer.getMilliseconds() << " ms" << endl;
		m_requiresRebuildFromData = false;

        //lightweight clones never hold on to their calib instruments as they exist for LT risk in cloned PS only
        if(m_lightweightClone)
        {
            	finishCalibInstruments();
                InstrumentComponent::setGlobalComponentCache(0);
        }
        else
        {
		    InstrumentComponent::setGlobalComponentCache(gcc);
        }
	}


	void FlexYCFZeroCurve::rebuildFromGenericData()
    {
		GenericDataPtr masterTable = m_marketData->getData();
		CalibrationInstrumentFactory::updateInstrumentRates(*m_fullInstruments, masterTable);
		for(size_t i=0, k=0; i < m_fullInstruments->size();++i)
		{
			double r = (*m_fullInstruments)[i]->getRate();
			if((*m_fullInstruments)[i]->wasPlaced())
			{
				(*m_partialInstruments)[k]->setRate(r);
				++k;
			}
		}
       
		m_model->prepareForSolve();
		m_solver->setState(BaseSolver::REFRESHING);
        solveCurve();
		m_model->setCalibrated();
        calibrationInstrumentSetValues();
		m_requiresRefreshFromData = false;
	}


    void FlexYCFZeroCurve::setRefreshType(const FlexYCF::ZeroCurveRefreshType refreshType)
	{
		m_refreshType = refreshType;
       

        switch (m_refreshType) {
            case FlexYCF::FromSolver:
		    case FlexYCF::FromJacobian:
		    {
                if (!m_model->isJacobianSupported()) {
					    LT_THROW_ERROR("Attempting to refresh from Solver, but model does not support it ");
                }
            }
        
            case FlexYCF::FullRebuild:
                break;
            default:
			    LT_THROW_ERROR("Invalid refresh type");
   
        }
    }

    void FlexYCFZeroCurve::refresh()
    {
		LTQuant::Timer refreshTimer;
		refreshTimer.start();

		//lazyInit();
		// Safety check: if I enter to do a refresh, but no data has been built yet, then do a full rebuild and return
		// Do not call lazyInit, because lazyInit also checks for lazyRefresh and than triggers an infinite loop
		if (m_requiresRebuildFromData) {
			rebuildCurveFromData();
			m_requiresRebuildFromData = false;
			return;
		}

        m_model->prepareForSolve();

        //only require a lazyinit if going to refresh solver or rebuild from jacobian
        //a  full rebuildfromCurveData will force a build regardless
		try
		{
            switch (m_refreshType) {
            case FlexYCF::FromSolver:
			{

				    // Check that the date hasn't changed
				    if(m_valueDate != m_marketData->getValueDate())
				    {
					    LT_THROW_ERROR("Market data has changed Date to " << m_marketData->getValueDate() << " from " << m_valueDate);
				    }
                
                    //if we don't have instruments must rebuild
                    //for this refresh from data
                    //else can refresh
                    if(!m_calibInstrumentsExist)
                    {
                    
                        const GenericDataPtr masterTable(m_marketData->getData());
						GlobalComponentCache* gcc = InstrumentComponent::getGlobalComponentCache();
						GlobalComponentCache globalComponentCache = GlobalComponentCache::createCache(masterTable, m_valueDate);
						InstrumentComponent::setGlobalComponentCache(&globalComponentCache);
                        
						FlexYCF::CalibrationInstrumentsPtr tmpFullInstruments;
                        // ----------------------------------------------------------------------------------------------------------------------
                        // 1. Load the calibration instruments
                        // ----------------------------------------------------------------------------------------------------------------------
                        tmpFullInstruments.reset(new CalibrationInstruments);
                        CalibrationInstrumentFactory::loadInstrumentList(*tmpFullInstruments, 
                            masterTable, 
                            globalComponentCache,
                            getParent());

                        FlexYCF::CalibrationInstruments::iterator curFullInstr=m_fullInstruments->begin();
                        FlexYCF::CalibrationInstruments::iterator curTmpInst=tmpFullInstruments->begin();
                        for (;curFullInstr!=m_fullInstruments->end() && curTmpInst!=tmpFullInstruments->end();++curFullInstr,++curTmpInst)
                        {
                           if( (*curFullInstr)->getName()!=(*curTmpInst)->getName() || 
                               (*curFullInstr)->getDescription()!=(*curTmpInst)->getDescription())
                           {
                               LTQC_THROW(IDeA::SystemException, "Different number/type of instruments between build/rebuild of curve!");
                           }

                           (*curFullInstr)->reloadInternalState(*curTmpInst);
                        }

                        CalibrationInstrumentFactory::updateInstrumentList(*m_fullInstruments, masterTable);

                        m_calibInstrumentsExist=true;
                        m_solver->setState(BaseSolver::REFRESHING);
                        //this solve curve needs to be in the same scope as the cache above hence can't factor it out of the if/else
                        solveCurve();
						finishCalibInstruments();
						InstrumentComponent::setGlobalComponentCache(gcc);
                    }
                    else
                    {
						rebuildFromGenericData();
                    }
            }
            break;
  
			case FlexYCF::FromJacobian:
			{
				//	... OR refreshFromJacobian:
				//	at this stage (i.e. at the start of the "try" clause), the full instruments
				//	will NOT be updated yet, however, the GenericIRMarketData will contain the
				//	new rates, so we CAN calculate the market rates shocks
				refreshFromJacobian();
			}
            break;
			
            case FlexYCF::FullRebuild:
            {
                    rebuildCurveFromData();
            }
            break;

            default:
			    {
				LTQC_THROW( LTQC::ModelQCException, "FlexYCFZeroCurve::refreshType not recognized" );
			    }
           
            }
        } 
		catch(exception& exc)
        {
            LT_LOG << exc.what() << endl;
			LT_THROW_ERROR("Curve refresh failed due to: " << exc.what());
        }
		
		refreshTimer.stop();
		LT_LOG << "Refresh solving for " << m_marketData->getIndexName() << " in " << refreshTimer.getMilliseconds() << " ms" << endl;
    }


	//	Calculates the shock on the unknowns from a shock in the input rates according
	//	the first-order approximation:
	//
	//								dz  =  J[z/C]  x  J[C/R]  x  dR
	//
	//	where:
	//	dz is the shock on the unknowns to calculate
	//	J[z/C] = J[C/z]^{-1} is the NxN jacobian of the unknowns relative to the PV of the "partial" instruments
	//	J[C/R] is NxN (diagonal) jacobian of the sensitivy of the instrument PV relative to their market rate
	//	dR is the shock on the input rates
	void FlexYCFZeroCurve::refreshFromJacobian()
	{

        // integrity check
        if (!m_model->isJacobianSupported()) {
				LT_THROW_ERROR("Attempting to refresh from Solver, but model does not support it ");
        }

        //NB in refresh from jacobian we have to assume that the calibration instruments no longer exist
        //and only use the cashed (eg getRateDerivative) values
        //at least always check m_calibInstrumentsExist before calcing the isntruments
       
		//	Calculate (or retrieve) the inverse Jacobian
		LTQC::Matrix invJacobian(m_model->getInverseJacobian());
        invJacobian.negate();
        
		LTQC::VectorDouble scaledRatesShifts(createShockedRates(*m_partialInstruments, m_marketData->getData()));
		
      
		//	TODO: Check that the size of the rate differences is the same as the # of partial instruments
		
		//	Multiplication of the diagonal matrix J[C/R] with (column) vector dR
		for(size_t j(0); j < m_partialInstruments->size(); ++j)
		{
			scaledRatesShifts[j] *= (*m_partialInstruments)[j]->getRateDerivative();
		}	
       
		const LTQC::VectorDouble variablesShifts(invJacobian.dot(scaledRatesShifts));
		
		//	Shift by the unknowns of the model by the solved shifts
		m_model->updateVariablesFromShifts(variablesShifts);

	}
	
	void FlexYCFZeroCurve::refreshFromFullJacobian()
	{
		std::vector<size_t> childrenModels = getChildrenZeroCurves(getIRIndexProperties()->getCurrencyName(), getIRIndexProperties()->getIndexName());
		if(  childrenModels.empty() )
		{
			refreshFromJacobian();
			return;
		}
		string adDiscriminator;
		IRAssetDomain::buildDiscriminator(getIRIndexProperties()->getCurrencyName(),getIRIndexProperties()->getIndexName(),adDiscriminator);
		LTQC::Matrix negativeInverseJacobian(m_model->getInverseJacobian());
		negativeInverseJacobian.negate();
		size_t numberOfInstruments = negativeInverseJacobian.getNumRows();
		vector<double> dR;
		shockedRates(dR);
		refreshFromNegativeInverseJacobianAndRatesShift(dR, negativeInverseJacobian);
		
		for(size_t i = 0; i < childrenModels.size(); ++i)
		{
			ZeroCurvePtr zc = getParentRaw()->getZeroCurve(childrenModels[i]);        
			LTQuant::FlexYCFZeroCurvePtr flexYcfZeroCurveChildModel(std::tr1::dynamic_pointer_cast<LTQuant::FlexYCFZeroCurve>(zc));
			FlexYCF::BaseModelPtr childModel = flexYcfZeroCurveChildModel->getModel();
			LTQC::Matrix negativeInverseJacobian2(childModel->getInverseFullJacobian());
			negativeInverseJacobian2.negate();

			const size_t rows = negativeInverseJacobian2.getNumRows();
			const size_t k = childModel->numberOfPlacedInstruments();
			size_t offset = childModel->jacobianOffset(AssetDomain::createAssetDomain(adDiscriminator));
			
			LTQC::Matrix negativeInverseJacobianChildModel;
			negativeInverseJacobianChildModel.subMatrix(negativeInverseJacobian2, rows - k, offset, rows - 1,  offset + numberOfInstruments - 1);
			vector<double> dR2 = ratesShiftTimesRatesDerivative(dR);
			flexYcfZeroCurveChildModel->refreshFromNegativeInversePartialJacobianAndRatesShift(dR2, negativeInverseJacobianChildModel);
		}
	}

    void FlexYCFZeroCurve::shockedRates(vector<double>& result) const
    {
        LTQC::VectorDouble shocks(createShockedRates(*m_partialInstruments, m_marketData->getData()));
        for(size_t i = 0; i < shocks.size(); ++i)
        {
            result.push_back(shocks[i]);
        }
    }

    //release the the calibration instruments
        //to minimise memory usage
        //N.B. This function will try to value full calib instruments
        //if you no longer have a valid globalComponentCache (e.g. it was on the stack in rebuildCurveData and the frame is gone)
        //this function will have undefined effects
    void FlexYCFZeroCurve::finishCalibInstruments()
    {
        m_calibInstrumentsExist=false;
        if(m_model)
        {
             m_model->finishCalibration();
             if(m_fullInstruments)
             {
                 for_each(m_fullInstruments->begin(),m_fullInstruments->end(),[&](CalibrationInstrumentPtr& it){it->finishCalibration(m_model);});
             }
             if(m_partialInstruments)
             {
                 for_each(m_partialInstruments->begin(),m_partialInstruments->end(),[&](CalibrationInstrumentPtr& it){it->finishCalibration(m_model);});
             }
        }
    }
	 
	void FlexYCFZeroCurve::calibrationInstrumentSetValues()
    {
        if(m_model)
        {
             if(m_fullInstruments)
             {
                 for_each(m_fullInstruments->begin(),m_fullInstruments->end(),[&](CalibrationInstrumentPtr& it){it->setValues(m_model);});
             }
             if(m_partialInstruments)
             {
                 for_each(m_partialInstruments->begin(),m_partialInstruments->end(),[&](CalibrationInstrumentPtr& it){it->setValues(m_model);});
             }
        }
    }

	vector<double> FlexYCFZeroCurve::ratesShiftTimesRatesDerivative(const vector<double>& dR) const
	{
		vector<double> scaledRatesShifts(dR.size(), 0.0);
		for(size_t j(0); j < m_partialInstruments->size(); ++j)
		{
            if( dR[j] != 0.0 )
            {
			    scaledRatesShifts[j] = dR[j] * (*m_partialInstruments)[j]->getRateDerivative();
            }
		}
		return scaledRatesShifts;
	}

	LTQC::VectorDouble FlexYCFZeroCurve::refreshFromNegativeInverseJacobianAndRatesShift(const vector<double>& dR, const LTQC::Matrix& invJacobian)
	{

        //NB in refresh from jacobian we have to assume that the calibration instruments no longer exist
        //and only use the cashed (eg getRateDerivative) values
        //at least always check m_calibInstrumentsExist before calcing the isntruments
		LTQC::VectorDouble scaledRatesShifts(dR.size(), 0.0);
	   
		//	Multiplication of the diagonal matrix J[C/R] with (column) vector dR
		for(size_t j(0); j < m_partialInstruments->size(); ++j)
		{
            if( dR[j] != 0.0 )
            {
			    scaledRatesShifts[j] = dR[j] * (*m_partialInstruments)[j]->getRateDerivative();
            }
		}	
        
		const LTQC::VectorDouble variablesShifts(invJacobian.dot(scaledRatesShifts));
		m_model->updateVariablesFromShifts(variablesShifts);

        return variablesShifts;
	}
    
	LTQC::VectorDouble FlexYCFZeroCurve::refreshFromNegativeInversePartialJacobianAndRatesShift(const vector<double>& dR, const LTQC::Matrix& invJacobian)
	{
		LTQC::VectorDouble scaledRatesShifts(dR.size(), 0.0);
		for(size_t j(0); j < dR.size(); ++j)
		{
            if( dR[j] != 0.0 )
            {
			    scaledRatesShifts[j] = dR[j];
            }
		}
		LTQC::VectorDouble variablesShifts(invJacobian.dot(scaledRatesShifts));
		m_model->updateVariablesFromShifts(variablesShifts);
        return variablesShifts;
	}

    void FlexYCFZeroCurve::solveCurve()
    {
        GenericDataPtr masterTable(m_marketData->getData());
        GenericDataPtr convexityModelTable;
        
        // might need to use a separate tag that specifies whether to use or not a convexity model
        masterTable->permissive_get<GenericDataPtr>("Convexity Model", 0, convexityModelTable, GenericDataPtr());
        
        if(convexityModelTable && getParent())
        {
            const size_t lookupIndex(getParent()->getLookupValue(m_marketData->getCurrency(), m_marketData->getIndexName()));
            SwaptionCubePtr swaptionCube(getParent()->getSwaptionCube(lookupIndex));

            // Get the futures convexity adjustment model
            FuturesConvexityModelPtr futuresConvexityModel;
            CalibrationInstruments::const_iterator futuresIter(m_partialInstruments->find<Futures>());
            
            if(futuresIter != m_partialInstruments->end())
            {
				futuresConvexityModel = std::tr1::dynamic_pointer_cast<Futures>(*futuresIter)->getConvexityModel();
            }
            
            if(swaptionCube && futuresConvexityModel)// && convexityModelTable)
            {
                /* TwoQSwaptionCubePtr twoQSwaptionCube(boost::dynamic_pointer_cast<TwoQSwaptionCube>(swaptionCube));
                if(twoQSwaptionCube)
                { */
                const long defaultNumberOfIterations(1);
                long numberOfIterations(defaultNumberOfIterations);
                convexityModelTable->permissive_get<long>("Max Iterations", 0, numberOfIterations, defaultNumberOfIterations);

                LT_LOG << "Solving the model iteratively" << endl;

                /*
                // Solve at least once the model once
                m_solver->solve(*m_partialInstruments, m_model);

                // make sure we are the current yield curve for this index
                //getParent()->addZeroCurve(m_marketData->getCurrency(), m_marketData->getIndexName(), shared_from_this());
                // shared_from_this() creates a crash
                getParent()->addZeroCurve(m_marketData->getCurrency(), m_marketData->getIndexName(), FlexYCFZeroCurvePtr(this, FlexYCF::NullDeleter<FlexYCFZeroCurve>()));
                */
                
                for(int cnt(0); cnt < numberOfIterations; ++cnt)
                {
                    LT_LOG << "Big Iteration\t: " << cnt << endl;                

                    // need to update convexity adjustments once the swaption vol cube has changed!
                    futuresConvexityModel->calibrateToSwaptionCube();

                    m_solver->solve(*m_partialInstruments, m_model); 
                    //before messaging the parent price supplier that we are solved we need to set our own state to solved
                    //else if one of the other children of this price supplier decides to call back into as (it may need a rate)
                    //we will start an infinite recursion
                    m_requiresRebuildFromData = false;
                    if(getParent())
                    {
                        getParent()->addZeroCurve(m_marketData->getCurrency(), m_marketData->getIndexName(), FlexYCFZeroCurvePtr(this, FlexYCF::NullDeleter()));
                        getParent()->zeroCurveUpdateNotify(m_marketData);
                    }
                }

                // need to update convexity adjustments once the swaption vol cube has changed!
                futuresConvexityModel->calibrateToSwaptionCube(/*m_model*/);
         
            }
        }
        else
        {
            // ----------------------------------------------------------------------------------------------------------------------
            // Everything's ready, calibrate the model to the instruments - as before 
            // ----------------------------------------------------------------------------------------------------------------------
            LT_LOG << "Simple Model solving" << endl;
            m_solver->solve(*m_partialInstruments, m_model);
            //before messaging the parent price supplier that we are solved we need to set our own state to solved
            //else if one of the other children of this price supplier decides to call back into as (it may need a rate)
            //we will start an infinite recursion
            m_requiresRebuildFromData = false;
            if(getParent())
            {
                // getParent()->addZeroCurve(m_marketData->getCurrency(), m_marketData->getIndexName(), FlexYCFZeroCurvePtr(this, FlexYCF::NullDeleter<FlexYCFZeroCurve>()));
                getParent()->zeroCurveUpdateNotify(m_marketData);   // not factored out
            }
        }
    }

    void FlexYCFZeroCurve::zeroCurveUpdateNotify(const string& currency, const string& indexName)
    {
        // reestablish the model dependencies
        if(getParent())
        {
            if(m_model && m_model->hasDependentIRMarketData(currency, indexName))
            {
                m_requiresRebuildFromData = true;
            }
        }        
    }

    void FlexYCFZeroCurve::spotRateUpdateNotify(const string& main, const string& money)
    {
        // reestablish the model dependencies
        if(getParent())
        {
            if(m_model && m_model->hasDependentFXSpotMarketData(main, money))
            {
                m_requiresRebuildFromData = true;
            }
        }        
    }
   
    double FlexYCFZeroCurve::getDiscountFactor(LT::date flowDate) const
    {
		lazyInit();
        // need to support inflation where we request historic data
        // avoid error in getYearsBetween
        double timePeriod(getYearsBetweenAllowNegative(m_valueDate, flowDate));
        return getDiscountFactor2(timePeriod);
    }

    double FlexYCFZeroCurve::getDiscountFactor2(double flowTime) const
    {
		lazyInit();
        return m_model->getDiscountFactor(flowTime);
    }

    double FlexYCFZeroCurve::getTenorDiscountFactor(LT::date flowDate, double tenor) const
    {
		lazyInit();

        // need to support inflation where we request historic data
        // avoid error in getYearsBetween
        double timePeriod(getYearsBetweenAllowNegative(m_valueDate, flowDate));
        return getTenorDiscountFactor2(timePeriod, tenor);
    }

    double FlexYCFZeroCurve::getTenorDiscountFactor2(double flowTime, double tenor) const
    {
		lazyInit();
        return m_model->getTenorDiscountFactor(flowTime, tenor);
    }

    double FlexYCFZeroCurve::getStructureFactor(const LT::date& flowDate) const
    {
        lazyInit();
        double timePeriod(getYearsBetweenAllowNegative(m_valueDate, flowDate));
        return getStructureFactor(timePeriod);
    }

    double FlexYCFZeroCurve::getStructureFactor(double flowTime) const
    {
        lazyInit();
        return m_model->getStructureFactor(flowTime);
    }

    IRMarketDataPtr FlexYCFZeroCurve::getMarketData() const
    {
		// do not lazyInit here as the mkt data can be retreived regardless

        return m_marketData;
    }

    double FlexYCFZeroCurve::getForwardRate(LT::date flowDate1, LT::date flowDate2, DayCounterPtr const& basis) const
    {
		lazyInit();
        /*double time1(getYearsBetweenAllowNegative(m_valueDate, flowDate1));
        double time2(getYearsBetweenAllowNegative(m_valueDate, flowDate2));

        return getForwardRate2(time1, time2, basis);*/
        
        // New calculation consistent with how forward rates
        //  as instrument component are calculated
        double tenor(basis->getDaysOverBasis(flowDate1, flowDate2));
        double dcf1(getTenorDiscountFactor(flowDate1, tenor));
        double dcf2(getTenorDiscountFactor(flowDate2, tenor));

        return (dcf1 / dcf2 - 1.0) / tenor;
    }

	double FlexYCFZeroCurve::getForwardRate(LT::date flowDate1, LT::date flowDate2, DayCounterPtr const& basis, const FlexYCF::CurveTypeConstPtr& curveType) const
    {
		lazyInit();

		if(curveType == FlexYCF::CurveType::Null())
		{
			double tenor(basis->getDaysOverBasis(flowDate1, flowDate2));
			double dcf1(getTenorDiscountFactor(flowDate1, tenor));
			double dcf2(getTenorDiscountFactor(flowDate2, tenor));

			return (dcf1 / dcf2 - 1.0) / tenor;
		} 
		else if(curveType->isTenor())
		{
			double tau(basis->getDaysOverBasis(flowDate1, flowDate2));
			double tenor = curveType->getYearFraction();
			double dcf1(getTenorDiscountFactor(flowDate1,tenor));
			double dcf2(getTenorDiscountFactor(flowDate2,tenor));

			return (dcf1 / dcf2 - 1.0) / tau;
		}
		else if(curveType == FlexYCF::CurveType::Discount())
		{
			double tau(basis->getDaysOverBasis(flowDate1, flowDate2));
			double dcf1(getDiscountFactor(flowDate1));
			double dcf2(getDiscountFactor(flowDate2));

			return (dcf1 / dcf2 - 1.0) / tau;
		}
		else
		{
			LT_THROW_ERROR("FlexYCFZeroCurve::getForwardRate: curve type not recognized");
		}
    }

    double FlexYCFZeroCurve::getForwardRate2(double flowTime1, double flowTime2, DayCounterPtr const& basis) const
    {
		lazyInit();
        double tenor(getTenorFromYears(flowTime2 - flowTime1));
        double dcf1(getTenorDiscountFactor2(flowTime1, tenor));
        double dcf2(getTenorDiscountFactor2(flowTime2, tenor));

        return (dcf1/dcf2 - 1.0) / (basis->getConvFactor() * (flowTime2 - flowTime1));
    }

    void FlexYCFZeroCurve::getForwardRateDecomposition( const LT::date& flowDate1, const LT::date& flowDate2, const ModuleDate::DayCounterPtr& basis, double& numDf, double& denomDf, double& tenor ) const
    {
        lazyInit();
        tenor = basis->getDaysOverBasis( flowDate1, flowDate2 );
        numDf = getTenorDiscountFactor( flowDate1, tenor );
        denomDf = getTenorDiscountFactor( flowDate2, tenor );
    }
	 
	void FlexYCFZeroCurve::getForwardRateDecomposition( const LT::date& flowDate1, const LT::date& flowDate2, const ModuleDate::DayCounterPtr& basis, double& numDf, double& denomDf, double& tenor, const FlexYCF::CurveTypeConstPtr& curveType) const
    {
        lazyInit();
		
		if(curveType == FlexYCF::CurveType::Null())
		{
			tenor = basis->getDaysOverBasis( flowDate1, flowDate2 );
			numDf = getTenorDiscountFactor( flowDate1, tenor );
			denomDf = getTenorDiscountFactor( flowDate2, tenor );
		}
		else if(curveType->isTenor())
		{
			double curveTenor = curveType->getYearFraction();
			tenor = basis->getDaysOverBasis( flowDate1, flowDate2 );
			numDf = getTenorDiscountFactor( flowDate1, curveTenor );
			denomDf = getTenorDiscountFactor( flowDate2, curveTenor );
		}
		else if(curveType == FlexYCF::CurveType::Discount())
		{
			tenor = basis->getDaysOverBasis( flowDate1, flowDate2 );
			numDf = getDiscountFactor(flowDate1);
			denomDf = getDiscountFactor(flowDate2);
		}
		else
		{
			LT_THROW_ERROR("FlexYCFZeroCurve::getForwardRateDecomposition: curve type not recognized");
		}
    }

    ZeroCurvePtr FlexYCFZeroCurve::clone() const
    {
        // Need to ensure the curve has been built, otherwise both the original and the clone will duplicate effort
        lazyInit();

        CloneLookup lookup;
        FlexYCFZeroCurvePtr retVal(new FlexYCFZeroCurve(*this, lookup));
        return retVal;
    }

	FlexYCF::BaseModelPtr FlexYCFZeroCurve::getModel() const
	{
		lazyInit();
        //this assumes the model cannot be used in a way that will require the calib instruments to be fully constructed
        return m_model;
	}

	LTQuant::GenericDataPtr FlexYCFZeroCurve::getModelParametersData() const
	{
		lazyInit();
		return (m_marketData? m_marketData->getData()->get<LTQuant::GenericDataPtr>(IDeA_PARAM(YIELDCURVE, FLEXYC_MODELPARAMETERS), 0): LTQuant::GenericDataPtr());
	}

	LTQuant::GenericDataPtr FlexYCFZeroCurve::getSpineCurvesDetails() const
	{
		lazyInit();
		return m_model->getSpineCurvesDetails();
	}
	
	ModuleStaticData::IRIndexPropertiesPtr FlexYCFZeroCurve::getMergedIRIndexProperties() const
	{
		const GenericDataPtr masterTable(m_marketData->getData());
		const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		const IDeA::IRCurveMktConventionPtr convention = createIRCurveMktConventions(*parametersTable, m_curveIndexProperties);
		
		const long spotDays = convention->m_depo.m_spotDays.getDays();
		const std::string basis( convention->m_depo.m_dcm.asString().data());
		const std::string calendarName = convention->m_depo.m_accrualValueCalendar.string();
		const std::string fixingCalendarName = convention->m_depo.m_fixingCalendar.string();
		const long swapFloatPeriod = convention->m_swaps.m_floatFrequency.asMonths();
		const long swapFixedPeriod = convention->m_swaps.m_fixedFrequency.asMonths();
		const std::string swapFixedbasis( convention->m_swaps.m_fixedAccrualBasis.asString().data());

		ModuleStaticData::IRIndexPropertiesPtr properties
		(
			new ModuleStaticData::IRIndexProperties
			(	m_curveIndexProperties->getCurrencyName(),
				m_curveIndexProperties->getIndexName(),
				calendarName,
				spotDays,
				fixingCalendarName,
				swapFixedPeriod,
				swapFixedbasis,
				swapFloatPeriod,
				basis,
				m_curveIndexProperties->getProperties()["CapQuoteBreak"].getLongValue(),
				m_curveIndexProperties->getProperties()["ShortCapPeriod"].getLongValue(),
				m_curveIndexProperties->getProperties()["LongCapPeriod"].getLongValue(),
				m_curveIndexProperties->getProperties()["CMSBreak"].getLongValue(),
				m_curveIndexProperties->getShortCMSFixedPeriod(),
				m_curveIndexProperties->getProperties()["LongCMSFixedPeriod"].getLongValue(),
				m_curveIndexProperties->getLongCMSFloatPeriod(),
				m_curveIndexProperties->getProperties()["CMSFixedBasis"].getStringValue(),
				m_curveIndexProperties->getURRFBlip(),
				m_curveIndexProperties->useYieldCurveSettings(),
				m_curveIndexProperties->useOnly3MOnBaseCurveForSpreads(),
				m_curveIndexProperties->hasOwnVols(),
				m_curveIndexProperties->getSwapStartDelay()	
			)
		);
		return properties;
	}

	std::vector<size_t> FlexYCFZeroCurve::getChildrenZeroCurves(const string& currency, const string& index) const
	{
		PriceSupplier* ps = getParentRaw();
		std::vector<size_t> result;	
		for(size_t i = 0; i < ps->zeroCurvesSize(); ++i)
		{     
			ZeroCurvePtr zc = ps->getZeroCurve(i);        
			LTQuant::FlexYCFZeroCurvePtr flexYcfZeroCurve(std::tr1::dynamic_pointer_cast<LTQuant::FlexYCFZeroCurve>(zc));
			if( !flexYcfZeroCurve )
			{
				throw ModelException("FlexYCFZeroCurve::getChildenZeroCurves", "Can not find FlexYCF yield curve");
			}
			FlexYCF::BaseModelPtr model = flexYcfZeroCurve->getModel();

			if ( model->hasDependentModel(currency, index) )
			{
				result.push_back(i);
			}
		}
		return result;		
	}

	void FlexYCFZeroCurve::getSpineInternalData(FlexYCF::SpineDataCachePtr& sdp) {
		sdp->model_ = getModel();
		sdp->model_->getSpineInternalData(sdp);

		string ccy = getIRIndexProperties()->getCurrencyName();
		string idx = getIRIndexProperties()->getIndexName();
		PriceSupplier* ps = getParentRaw();
		for (size_t i = 0; i < ps->zeroCurvesSize(); ++i) {
			ZeroCurvePtr zc = ps->getZeroCurve(i);        
			LTQuant::FlexYCFZeroCurvePtr flexycfptr(std::tr1::dynamic_pointer_cast<LTQuant::FlexYCFZeroCurve>(zc));
			if (flexycfptr && flexycfptr->getModel()->hasDependentModel(ccy, idx)) {
				SpineDataCachePtr p(new SpineDataCache);
				flexycfptr->getSpineInternalData(p);
				sdp->childern_.push_back(p);
			}
		}
	}

	void FlexYCFZeroCurve::restoreModelSpineData(FlexYCF::SpineDataCachePtr& sdp) {
		BaseModel::restoreModelSpineData(sdp);
	}
}

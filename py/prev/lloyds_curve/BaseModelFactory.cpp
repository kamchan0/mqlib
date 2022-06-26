/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#include "stdafx.h"

// IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

// FlexYCF
#include "BaseModel.h"
#include "BaseModelFactory.h"
#include "CalibrationInstruments.h"
#include "CalibrationInstrumentFactory.h"
#include "StubBaseModel.h"
#include "MultiTenorModel.h"
#include "MultiTenorFormulationModel.h"
#include "MultiTenorOISModel.h"
#include "MultiTenorOISFundingModel.h"
#include "InflationModel.h"
#include "StripperModel.h"
#include "FundingStripperModel.h"
#include "CTDModel.h"
#include "IndexStripperModel.h"
#include "BaseKnotPointPlacement.h"
#include "KnotPointPlacementFactory.h"
#include "BaseSolver.h"
#include "SolverFactory.h"
#include "GlobalComponentCache.h"
#include "Timer.h"
#include "InstrumentComponent.h"
#include "LeastSquaresResiduals.h"
#include "InitializationFactory.h"
#include "ConstantInitialization.h"
#include "ModelSelection.h"

//	LTQuantLib
#include "Maths/LevenbergMarquardtSolver.h"
#include "Pricers\ZeroCurve.h"
#include "Pricers\PriceSupplier.h"
#include "Data\GenericData.h"

using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    using namespace LTQuant;

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
                                 const LTQuant::GenericDataPtr masterTable, 
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
        
        void printTimer(ostream& out, LTQuant::Timer& timer, const string& msg)
        {
            timer.stop();
            out << msg << "\t" << timer.getMilliseconds() << " ms" << endl;
            timer.start();
        }
    }

   
    BaseModelPtr ModelFactory::createModel(const string& modelName, const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent )
    {
        return ModelFactory::create(modelName)(masterTable, parent);
    }

    void ModelFactory::listModels(std::vector<std::string>  &modelNames)
    {
        return instance()->modelNames(modelNames);
    }
    bool ModelFactory::isValidModel(const std::string& modelName)
    {
        return instance()->isRegisteredName(modelName);
    }
    ModelFactory* const ModelFactory::instance()
    {
        static ModelFactory modelFactory;
        return &modelFactory;
    }

    ModelFactory::ModelFactory()
    {
        registerModel<StubBaseModel>();
        registerModel<MultiTenorModel>();
		registerModel<MultiTenorFormulationModel>();
		registerModel<MultiTenorOISModel>();
        registerModel<MultiTenorOISFundingModel>();
        registerModel<InflationModel>();
        registerModel<StripperModel>();
        registerModel<FundingStripperModel>();
        registerModel<FundingSpreadStripperModel>();
		registerModel<FundingSpreadModel>();
        registerModel<IndexSpreadStripperModel>();
		registerModel<IndexBaseSpreadStripperModel>();
		registerModel<FundingIndexSpreadStripperModel>();
		registerModel<CTDModel>();
    }

}   //  FlexYCF



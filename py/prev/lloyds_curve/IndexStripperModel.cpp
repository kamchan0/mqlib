#include "stdafx.h" 

//	FlexYCF
#include "IndexStripperModel.h"
#include "FlexYCFCloneLookup.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

using namespace LTQC;
using namespace LTQuant;
using namespace std;
using namespace IDeA;

namespace FlexYCF
{

    IndexSpreadStripperModel::IndexSpreadStripperModel(const LT::date valueDate, const string tenorDescription, const LTQuant::GenericDataPtr masterTable,const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	
    }

    BaseModelPtr IndexSpreadStripperModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new IndexSpreadStripperModel(valueDate, "3M", masterTable, parent));
    }
    
    void IndexSpreadStripperModel::accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
    }
	void IndexSpreadStripperModel::accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }
    void IndexSpreadStripperModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        if(!m_dependentModel)
        {
            initialize();
        }
        double tdf = m_dependentModel->getTenorDiscountFactor(flowTime,tenor);
        multiplier *= tdf;

        StripperModel::accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
    }

	void IndexSpreadStripperModel::accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			gradientBegin += m_leg2Model->numberOfPlacedInstruments();
		}
        m_dependentModel->accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier * getSpreadTenorDiscountFactor(flowTime, tenor), gradientBegin, gradientEnd);
    }
	
	void IndexSpreadStripperModel::accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			gradientBegin += m_leg2Model->numberOfPlacedInstruments();
		}
        m_dependentModel->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

    void IndexSpreadStripperModel::accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        StripperModel::accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier * getBaseTenorDiscountFactor(flowTime, tenor), gradientBegin, gradientEnd);
    }
    ICloneLookupPtr IndexSpreadStripperModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new IndexSpreadStripperModel(*this, lookup));
    }

   
    IndexSpreadStripperModel::IndexSpreadStripperModel(IndexSpreadStripperModel const& original, CloneLookup& lookup) :
    StripperModel(original, lookup),m_fullJacobian(original.m_fullJacobian),m_ccy(original.m_ccy),m_index(original.m_index),m_dependentModel(original.m_dependentModel),
        m_leg2Model(original.m_leg2Model),m_dependentModelAD(original.m_dependentModelAD),m_leg2ModelAD(original.m_leg2ModelAD)
    {
    }

	const LTQC::Matrix& IndexSpreadStripperModel::getFullJacobian() const
    {
        initialize();

		if( m_leg2Model )
			return getFullJacobian2();

        const LTQC::Matrix&	dependentJacobian = m_dependentModel->getFullJacobian();
        const LTQC::Matrix&	jacobian = getJacobian();
        const size_t rows = dependentJacobian.getNumRows() + jacobian.getNumRows();
        const size_t cols = dependentJacobian.getNumCols() + jacobian.getNumCols();
        const size_t depRows = dependentJacobian.getNumRows();
        const size_t depCols = dependentJacobian.getNumCols();

        m_fullJacobian.resize(rows, cols);

        for(size_t row = 0; row < rows; ++row)
        {
            for(size_t col = 0; col < cols; ++col)
            {
                if(row < depRows && col < depCols)
                {
					m_fullJacobian(row,col) = dependentJacobian(row,col);
                }
                else if(row >= depRows && col >= depCols)
                {
                    m_fullJacobian(row,col) = jacobian(row - depRows, col - depCols);
                } 
                else
                {
                m_fullJacobian(row,col) = 0.0;
                }
            }
        }

		// x-terms
		CalibrationInstruments instruments = getFullInstruments();
        for(size_t row = depRows; row < rows; ++row)
        {
            vector<double> gradient( depCols, 0.0);
			
            instruments[row - depRows]->accumulateGradientConstantTenorDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }

        }
		return m_fullJacobian;
	}
	
	const LTQC::Matrix& IndexSpreadStripperModel::getFullJacobian2() const
    {
		const LTQC::Matrix&	dependentJacobian = m_dependentModel->getJacobian();
		const LTQC::Matrix&	dependentFullJacobian = m_dependentModel->getFullJacobian();
		const LTQC::Matrix&	leg2Jacobian = m_leg2Model->getFullJacobian();
        const LTQC::Matrix&	jacobian = getJacobian();
        const size_t rows = dependentJacobian.getNumRows() + leg2Jacobian.getNumRows() + jacobian.getNumRows();
        const size_t cols = dependentJacobian.getNumCols() + leg2Jacobian.getNumCols() + jacobian.getNumCols();
		const size_t leg2Rows = leg2Jacobian.getNumRows();
        const size_t leg2Cols = leg2Jacobian.getNumCols();
        const size_t depRows = dependentJacobian.getNumRows() + leg2Rows;
        const size_t depCols = dependentJacobian.getNumCols() + leg2Cols;
	

        m_fullJacobian.resize(rows, cols);

        for(size_t row = 0; row < rows; ++row)
        {
            for(size_t col = 0; col < cols; ++col)
            {
				if(row < leg2Rows && col < leg2Cols)
                {
					m_fullJacobian(row,col) = leg2Jacobian(row,col);
                }
				else if(row < depRows && col < depCols && row >= leg2Rows && col >= leg2Cols)
				{
					m_fullJacobian(row,col) = dependentJacobian(row - leg2Rows, col - leg2Cols);
				}
				else if(row >= depRows && col >= depCols)
                {
                    m_fullJacobian(row,col) = jacobian(row - depRows, col - depCols);
                } 
                else
                {
					m_fullJacobian(row,col) = 0.0;
                }
            }
        }

		// x-terms
		if(dependentFullJacobian.getNumRows() > dependentJacobian.getNumRows() )
		{
			for(size_t row = leg2Rows; row < dependentFullJacobian.getNumRows() ; ++row)
			{
				for(size_t col = 0; col < leg2Cols; ++col)
				{
					m_fullJacobian(row,col) = dependentFullJacobian(row,col);
				}
			 }
		}

		CalibrationInstruments instruments = getFullInstruments();
        for(size_t row = depRows; row < rows; ++row)
        {
            vector<double> gradient( depCols, 0.0);
            instruments[row - depRows]->accumulateGradientConstantTenorDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }
        }
		return m_fullJacobian;
	}


}   //  FlexYCF

namespace FlexYCF
{

    FundingIndexSpreadStripperModel::FundingIndexSpreadStripperModel(const LT::date valueDate, const string tenorDescription, const LTQuant::GenericDataPtr masterTable,const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	
    }

    BaseModelPtr FundingIndexSpreadStripperModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new FundingIndexSpreadStripperModel(valueDate, "3M", masterTable, parent));
    }

    double FundingIndexSpreadStripperModel::getTenorDiscountFactor(double flowTime, double tenor, const LTQC::Currency& ccy, const LT::Str& index) const
	{
		if(!m_dependentModel)
        {
            initialize();
        }
        return m_dependentModel->getTenorDiscountFactor(flowTime,tenor,ccy,index);
	}

    void FundingIndexSpreadStripperModel::accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
		if(!m_dependentModel)
        {
            initialize();
        }
        double df = m_dependentModel->getDiscountFactor(flowTime);
        multiplier *= df;

        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

	void FundingIndexSpreadStripperModel::accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
		StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier * getBaseDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }

    void FundingIndexSpreadStripperModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
    }

	void FundingIndexSpreadStripperModel::accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
		if(!m_dependentModel)
        {
            initialize();
        }
	
        m_dependentModel->accumulateDiscountFactorGradient(flowTime, multiplier * getSpreadDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }
	
	void FundingIndexSpreadStripperModel::accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		if(!m_dependentModel)
        {
            initialize();
        }
		m_dependentModel->accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
    }

    void FundingIndexSpreadStripperModel::accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
		LTQC_THROW( IDeA::ModelException, "accumulateSpreadTenorDiscountFactorGradient not implemented, use SemiAnalytic risk" );
    }

    ICloneLookupPtr FundingIndexSpreadStripperModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FundingIndexSpreadStripperModel(*this, lookup));
    }

   
    FundingIndexSpreadStripperModel::FundingIndexSpreadStripperModel(FundingIndexSpreadStripperModel const& original, CloneLookup& lookup) :
        StripperModel(original, lookup), m_fullJacobian(original.m_fullJacobian),m_ccy(original.m_ccy),m_index(original.m_index),m_dependentModel(original.m_dependentModel),m_dependentModelAD(original.m_dependentModelAD)
    {
    }

	const LTQC::Matrix& FundingIndexSpreadStripperModel::getFullJacobian() const
    {
        initialize();

	
        const LTQC::Matrix&	dependentJacobian = m_dependentModel->getFullJacobian();
        const LTQC::Matrix&	jacobian = getJacobian();
        const size_t rows = dependentJacobian.getNumRows() + jacobian.getNumRows();
        const size_t cols = dependentJacobian.getNumCols() + jacobian.getNumCols();
        const size_t depRows = dependentJacobian.getNumRows();
        const size_t depCols = dependentJacobian.getNumCols();

        m_fullJacobian.resize(rows, cols);

        for(size_t row = 0; row < rows; ++row)
        {
            for(size_t col = 0; col < cols; ++col)
            {
                if(row < depRows && col < depCols)
                {
					m_fullJacobian(row,col) = dependentJacobian(row,col);
                }
                else if(row >= depRows && col >= depCols)
                {
                    m_fullJacobian(row,col) = jacobian(row - depRows, col - depCols);
                } 
                else
                {
					m_fullJacobian(row,col) = 0.0;
                }
            }
        }

		// x-terms
		CalibrationInstruments instruments = getFullInstruments();
        for(size_t row = depRows; row < rows; ++row)
        {
            vector<double> gradient( depCols, 0.0);
			
            instruments[row - depRows]->accumulateGradientConstantDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }

        }
		return m_fullJacobian;
	}
}   //  FlexYCF

namespace FlexYCF
{

    IndexBaseSpreadStripperModel::IndexBaseSpreadStripperModel(const LT::date valueDate, const string tenorDescription, const LTQuant::GenericDataPtr masterTable,const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	
    }

    BaseModelPtr IndexBaseSpreadStripperModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new IndexBaseSpreadStripperModel(valueDate, "3M", masterTable, parent));
    }
    
    void IndexBaseSpreadStripperModel::accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
    }
	void IndexBaseSpreadStripperModel::accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier , gradientBegin, gradientEnd);
    }
    void IndexBaseSpreadStripperModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        if(!m_dependentModel)
        {
            initialize();
        }
        double tdf = m_dependentModel->getTenorDiscountFactor(flowTime,m_baseRateTenor);
        multiplier *= tdf;

        StripperModel::accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
    }
	void IndexBaseSpreadStripperModel::accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			gradientBegin += m_leg2Model->numberOfPlacedInstruments();
		}
        m_dependentModel->accumulateTenorDiscountFactorGradient(flowTime, m_baseRateTenor, multiplier * getSpreadTenorDiscountFactor(flowTime,tenor), gradientBegin, gradientEnd);
    }
	void IndexBaseSpreadStripperModel::accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			gradientBegin += m_leg2Model->numberOfPlacedInstruments();
		}
        m_dependentModel->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }
    void IndexBaseSpreadStripperModel::accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    { 
        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier * getBaseTenorDiscountFactor(flowTime,m_baseRateTenor), gradientBegin, gradientEnd);
    }
    ICloneLookupPtr IndexBaseSpreadStripperModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new IndexBaseSpreadStripperModel(*this, lookup));
    }

   
    IndexBaseSpreadStripperModel::IndexBaseSpreadStripperModel(IndexBaseSpreadStripperModel const& original, CloneLookup& lookup) :
    StripperModel(original, lookup),m_fullJacobian(original.m_fullJacobian),m_ccy(original.m_ccy),m_index(original.m_index),m_dependentModel(original.m_dependentModel),
        m_baseRateTenor(original.m_baseRateTenor),
        m_leg2Model(original.m_leg2Model),m_dependentModelAD(original.m_dependentModelAD),m_leg2ModelAD(original.m_leg2ModelAD)
    {
    }

	const LTQC::Matrix& IndexBaseSpreadStripperModel::getFullJacobian() const
    {
        initialize();

		if( m_leg2Model )
			return getFullJacobian2();

        const LTQC::Matrix&	dependentJacobian = m_dependentModel->getFullJacobian();
        const LTQC::Matrix&	jacobian = getJacobian();
        const size_t rows = dependentJacobian.getNumRows() + jacobian.getNumRows();
        const size_t cols = dependentJacobian.getNumCols() + jacobian.getNumCols();
        const size_t depRows = dependentJacobian.getNumRows();
        const size_t depCols = dependentJacobian.getNumCols();

        m_fullJacobian.resize(rows, cols);

        for(size_t row = 0; row < rows; ++row)
        {
                for(size_t col = 0; col < cols; ++col)
                {
                    if(row < depRows && col < depCols)
                    {
						m_fullJacobian(row,col) = dependentJacobian(row,col);
                    }
                    else if(row >= depRows && col >= depCols)
                    {
                        m_fullJacobian(row,col) = jacobian(row - depRows, col - depCols);
                    } 
                    else
                    {
                    m_fullJacobian(row,col) = 0.0;
                    }
                }
        }

		// x-terms
		CalibrationInstruments instruments = getFullInstruments();
        for(size_t row = depRows; row < rows; ++row)
        {
            vector<double> gradient( depCols, 0.0);
			
            instruments[row - depRows]->accumulateGradientConstantTenorDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }

        }
		return m_fullJacobian;
	}

	const LTQC::Matrix& IndexBaseSpreadStripperModel::getFullJacobian2() const
    {
		const LTQC::Matrix&	dependentJacobian = m_dependentModel->getJacobian();
		const LTQC::Matrix&	dependentFullJacobian = m_dependentModel->getFullJacobian();
		const LTQC::Matrix&	leg2Jacobian = m_leg2Model->getFullJacobian();
        const LTQC::Matrix&	jacobian = getJacobian();
        const size_t rows = dependentJacobian.getNumRows() + leg2Jacobian.getNumRows() + jacobian.getNumRows();
        const size_t cols = dependentJacobian.getNumCols() + leg2Jacobian.getNumCols() + jacobian.getNumCols();
		const size_t leg2Rows = leg2Jacobian.getNumRows();
        const size_t leg2Cols = leg2Jacobian.getNumCols();
        const size_t depRows = dependentJacobian.getNumRows() + leg2Rows;
        const size_t depCols = dependentJacobian.getNumCols() + leg2Cols;
	

        m_fullJacobian.resize(rows, cols);

        for(size_t row = 0; row < rows; ++row)
        {
            for(size_t col = 0; col < cols; ++col)
            {
				if(row < leg2Rows && col < leg2Cols)
                {
					m_fullJacobian(row,col) = leg2Jacobian(row,col);
                }
				else if(row < depRows && col < depCols && row >= leg2Rows && col >= leg2Cols)
				{
					m_fullJacobian(row,col) = dependentJacobian(row - leg2Rows, col - leg2Cols);
				}
				else if(row >= depRows && col >= depCols)
                {
                    m_fullJacobian(row,col) = jacobian(row - depRows, col - depCols);
                } 
                else
                {
					m_fullJacobian(row,col) = 0.0;
                }
            }
        }

		// x-terms
		if(dependentFullJacobian.getNumRows() > dependentJacobian.getNumRows() )
		{
			for(size_t row = leg2Rows; row < dependentFullJacobian.getNumRows() ; ++row)
			{
				for(size_t col = 0; col < leg2Cols; ++col)
				{
					m_fullJacobian(row,col) = dependentFullJacobian(row,col);
				}
			 }
		}

		CalibrationInstruments instruments = getFullInstruments();
        for(size_t row = depRows; row < rows; ++row)
        {
            vector<double> gradient( depCols, 0.0);
            instruments[row - depRows]->accumulateGradientConstantTenorDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }
        }
		return m_fullJacobian;
	}

	
}   //  FlexYCF

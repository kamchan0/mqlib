#include "stdafx.h" 

//	FlexYCF
#include "FundingStripperModel.h"
#include "FlexYCFCloneLookup.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"


using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{

    FundingStripperModel::FundingStripperModel(const LT::date valueDate,
                                 const string tenorDescription,
                                 const LTQuant::GenericDataPtr masterTable, 
                                 const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	
    }

    BaseModelPtr FundingStripperModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new FundingStripperModel(valueDate, "3M", masterTable, parent));
    }
     
    void FundingStripperModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
    }
  
	void FundingStripperModel::accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }
	
	void FundingStripperModel::accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
    }
	
	void FundingStripperModel::accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		if(!m_dependentModel)
        {
            initialize();
        }
		m_dependentModel->accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
    }
    
	ICloneLookupPtr FundingStripperModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FundingStripperModel(*this, lookup));
    }

   
    FundingStripperModel::FundingStripperModel(FundingStripperModel const& original, CloneLookup& lookup) :
    StripperModel(original, lookup),m_fullJacobian(original.m_fullJacobian),m_ccy(original.m_ccy),m_index(original.m_index),m_dependentModel(original.m_dependentModel),
        m_leg2Model(original.m_leg2Model),m_dependentModelAD(original.m_dependentModelAD),m_leg2ModelAD(original.m_leg2ModelAD)
    {
    }

	const LTQC::Matrix& FundingStripperModel::getFullJacobian() const
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
            instruments[row - depRows]->accumulateGradientConstantDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),false);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }

        }
		return m_fullJacobian;
	}

	const LTQC::Matrix& FundingStripperModel::getFullJacobian2() const
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
            instruments[row - depRows]->accumulateGradientConstantDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),false);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }

        }
		return m_fullJacobian;
	}
	
}	

namespace FlexYCF
{

    FundingSpreadStripperModel::FundingSpreadStripperModel(const LT::date valueDate, const string tenorDescription, const LTQuant::GenericDataPtr masterTable,const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	
    }

    BaseModelPtr FundingSpreadStripperModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new FundingSpreadStripperModel(valueDate, "3M", masterTable, parent));
    }
    
    void FundingSpreadStripperModel::accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        if(!m_dependentModel)
        {
            initialize();
        }
        double df = m_dependentModel->getDiscountFactor(flowTime);
        multiplier *= df;

        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }
	
	void FundingSpreadStripperModel::accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			size_t k = m_leg2Model->numberOfPlacedInstruments();
			gradientBegin += k;
		}
        m_dependentModel->accumulateDiscountFactorGradient(flowTime, multiplier * getSpreadDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }
	
	void FundingSpreadStripperModel::accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier * getBaseDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }

    void FundingSpreadStripperModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
    }
    
	void FundingSpreadStripperModel::accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			gradientBegin += m_leg2Model->numberOfPlacedInstruments();
		}
		m_dependentModel->accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
    }
    
	ICloneLookupPtr FundingSpreadStripperModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FundingSpreadStripperModel(*this, lookup));
    }

   
    FundingSpreadStripperModel::FundingSpreadStripperModel(FundingSpreadStripperModel const& original, CloneLookup& lookup) :
        StripperModel(original, lookup),m_fullJacobian(original.m_fullJacobian),m_ccy(original.m_ccy),m_index(original.m_index),m_dependentModel(original.m_dependentModel),
                                        m_leg2Model(original.m_leg2Model),m_dependentModelAD(original.m_dependentModelAD),m_leg2ModelAD(original.m_leg2ModelAD)
    {
    }

	const LTQC::Matrix& FundingSpreadStripperModel::getFullJacobian() const
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
            instruments[row - depRows]->accumulateGradientConstantDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }
        }
		return m_fullJacobian;
	}

	const LTQC::Matrix& FundingSpreadStripperModel::getFullJacobian2() const
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

    FundingSpreadModel::FundingSpreadModel(const LT::date valueDate, const string tenorDescription, const LTQuant::GenericDataPtr masterTable,const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent), m_isCalibrated(false)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	
    }

    BaseModelPtr FundingSpreadModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new FundingSpreadModel(valueDate, "3M", masterTable, parent));
    }
    
    void FundingSpreadModel::accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        if(!m_dependentModel)
        {
            initialize();
        }
        double df = m_dependentModel->getDiscountFactor(flowTime);
        multiplier *= df;

        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }
	
	void FundingSpreadModel::accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			size_t k = m_leg2Model->numberOfPlacedInstruments();
			gradientBegin += k;
		}
        m_dependentModel->accumulateDiscountFactorGradient(flowTime, multiplier * getSpreadDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }
	
	void FundingSpreadModel::accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin,  GradientIterator gradientEnd) const
    {
        StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier * getBaseDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }

    void FundingSpreadModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		if(m_isCalibrated)
			accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin,  gradientEnd);
    }
    
	void FundingSpreadModel::accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		if(!m_dependentModel)
        {
            initialize();
        }
		if( m_leg2Model )
		{
			gradientBegin += m_leg2Model->numberOfPlacedInstruments();
		}
		m_dependentModel->accumulateDiscountFactorGradient(flowTime, multiplier * getSpreadDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }

    void FundingSpreadModel::accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		 StripperModel::accumulateDiscountFactorGradient(flowTime, multiplier * getBaseDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }

	ICloneLookupPtr FundingSpreadModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FundingSpreadModel(*this, lookup));
    }

   
    FundingSpreadModel::FundingSpreadModel(FundingSpreadModel const& original, CloneLookup& lookup) :
        StripperModel(original, lookup),m_fullJacobian(original.m_fullJacobian),m_ccy(original.m_ccy),m_index(original.m_index),m_dependentModel(original.m_dependentModel),
			m_leg2Model(original.m_leg2Model),m_dependentModelAD(original.m_dependentModelAD),m_leg2ModelAD(original.m_leg2ModelAD), m_isCalibrated(original.m_isCalibrated)
    {
    }

	const LTQC::Matrix& FundingSpreadModel::getFullJacobian() const
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
            instruments[row - depRows]->accumulateGradientConstantDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }
        }
		return m_fullJacobian;
	}

	const LTQC::Matrix& FundingSpreadModel::getFullJacobian2() const
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
            instruments[row - depRows]->accumulateGradientConstantDiscountFactor(*m_dependentModel, *this, 1.0, gradient.begin(), gradient.end(),true);

            for(size_t col = 0; col < depCols; ++col)
            {
                m_fullJacobian(row,col) = gradient[col];
            }
        }
		return m_fullJacobian;
	}
}   //  FlexYCF
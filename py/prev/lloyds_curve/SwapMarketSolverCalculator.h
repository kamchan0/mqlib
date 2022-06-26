#ifndef __SwapMarketSolverCalculator_H__
#define __SwapMarketSolverCalculator_H__

#include "FunctorImpl.h"
#include "YieldCurveIF.h"
#include "Matrix.h"
#include "Tenor.h"

#include "src/Enums/IRSwapProductType.h"

namespace IDeA
{
	class SwapMarketSolverCalculator 
	{
	public:
		SwapMarketSolverCalculator(const LT::TablePtr& ycTable, const LT::TablePtr& instrumentsTable, const LT::TablePtr& basisSwapsTable, FlexYCF::BaseModelPtr model);
        
		
        LT::TablePtr generateOutputTable();
        LT::TablePtr generateInterpolatedSwapsOutputTable();
        LT::TablePtr generateBasisSwapsOutputTable();
        LT::TablePtr generateAnnuityOutputTable();
        LT::TablePtr generateSpreadLegAnnuityOutputTable();
        LT::TablePtr generateImpliedQuotesOutputTable();
		LT::TablePtr generateParRatesOutputTable();

	protected:
		
		void extractTenorsAndRates();
        void extractWeights();
        void extractSwapTenorsAndRates(LT::TablePtr instrumentTable);
        void extractBasisSwapsTenorsAndRates(LT::TablePtr instrumentTable, LTQC::VectorDouble& w, std::vector< std::pair<std::string,double> >& swaps);
        void generateTenors();
       
        void populateMarketQuotes();
    
       	LTQC::VectorDouble solve();

        static void parseDescription(const string& description, std::vector<string>& tenors, string& fwdTenor);
        static void parseDescription(const string& description, std::vector<string>& tenors);
        static IRSwapProductType toIRSwapProductType(const string& description);
        
        void initializeMatrix();
        void populateAnnuity(FlexYCF::BaseModelPtr model);
      
        size_t annuityIndex(const LTQC::Tenor& tenor) const;
        double annuity(const LTQC::Tenor& tenor) const;
        double annuity(const LTQC::Tenor& tenor1, const LTQC::Tenor& tenor2) const;
        
        size_t annuityIndexBasis(const LTQC::Tenor& tenor, const LTQC::Tenor& basisTenor) const;
        double annuityBasis(const LTQC::Tenor& tenor, const LTQC::Tenor& basisTenor) const;
        double annuityBasis(const LTQC::Tenor& tenor1, const LTQC::Tenor& tenor2, const LTQC::Tenor& basisTenor) const;
        
        
        double implyQuote(const LTQC::VectorDouble& parSwapRates, const string& productDescription) const;
        
        void generateAnnuity(LT::date valueDate, LT::date startDate, LT::date endDate, const LTQC::Tenor& frequency, const string& basis, const string& calendar, FlexYCF::BaseModelPtr model);
        size_t swapIndex(const LTQC::Tenor& tenor) const;
        size_t basisSwapIndex3m6m(const LTQC::Tenor& tenor);
        size_t basisSwapIndex1m3m(const LTQC::Tenor& tenor);

        size_t numberOfAllSwaps() const { return  m_noSwaps; }
		size_t numberOfAll3m6mBasisSwaps() const { return  m_no3m6mSwaps; }
		size_t numberOfAll1m3mBasisSwaps() const { return  m_no1m3mSwaps; }
        size_t numberOfAllBasisSwaps() const { return  m_no3m6mSwaps + m_no1m3mSwaps; }
        void calculateNumberOfVariables();

        size_t numberOfSwaps() const { return  m_swapTenors.size(); }
        size_t numberOfVariables() const { return  numberOfAllSwaps() + numberOfAllBasisSwaps(); }
        size_t numberOfConstrains() const { return  m_noConstrains; }
        size_t numberOfSwapConstrains() const { return m_swapsPortfolio.size(); }
        size_t numberOfInterpolatedSwapConstrains() const { return m_swapInterpolatedTenors.size(); }
        size_t numberOf3m6mBasisSwapConstrains() const { return m_3m6mBasisSwaps.size(); }
        size_t numberOf3m6mInterpolatedBasisSwapConstrains() const { return m_3m6mBasisSwapInterpolatedTenors.size(); }
		size_t numberOf1m3mBasisSwapConstrains() const { return m_1m3mBasisSwaps.size(); }
        size_t numberOf1m3mInterpolatedBasisSwapConstrains() const { return m_1m3mBasisSwapInterpolatedTenors.size(); }
        
		static bool findConnectedTenors(const std::vector< std::pair<string,double> >& portfolio, std::set<LTQC::Tenor>& connected);
        static bool isInputPortfolioUnderspecified(const std::vector< std::pair<string,double> >& portfolio, const std::set<LTQC::Tenor>& allTenors, std::set<LTQC::Tenor>&);
        static bool connectdTenors(const LTQC::Tenor& tenor1, const LTQC::Tenor& tenor2, const std::vector< std::pair<string,double> >& portfolio);

        LT::TablePtr                  m_yieldCurveTable;
        LT::TablePtr                  m_swapTable;
        LT::TablePtr                  m_basisSwapTable;
		LT::TablePtr                  m_3m6mBasisSwapTable;
		LT::TablePtr                  m_1m3mBasisSwapTable;
        bool                          m_1mSwapFrequency;
		bool                          m_3mSwapFrequency;
		bool                          m_6mSwapFrequency;
		
        std::vector< std::pair<LT::date, double> >						m_annuity;
        std::map< LTQC::Tenor, std::vector< std::pair<LT::date, double> > >    m_annuityMap;
        
        LTQC::Tenor                   m_3m6mSpreadFrequency;
		LTQC::Tenor                   m_1m3mSpreadFrequency;
        size_t						  m_multiplier;
        std::map<LTQC::Tenor, size_t> m_basisMultiplier;
        LTQC::Tenor                   m_swapTenor;

        // constrains
		std::vector< std::pair<std::string,double> > m_swapsPortfolio;
        std::vector< std::pair<std::string,double> > m_3m6mBasisSwaps;
		std::vector< std::pair<std::string,double> > m_1m3mBasisSwaps;
        
        
        // variables
        std::vector<LTQC::Tenor>           m_swapTenors;
        std::vector<LTQC::Tenor>           m_swapInterpolatedTenors;
        std::vector<LTQC::Tenor>           m_3m6mBasisSwapTenors;
		std::vector<LTQC::Tenor>           m_1m3mBasisSwapTenors;
        std::vector<LTQC::Tenor>           m_3m6mBasisSwapInterpolatedTenors;
		std::vector<LTQC::Tenor>           m_1m3mBasisSwapInterpolatedTenors;
        
		size_t                        m_noSwaps;
        size_t                        m_no3m6mSwaps;
		size_t                        m_no1m3mSwaps;
        size_t                        m_noConstrains;

        LTQC::Matrix                  m_matrix;
      
        LTQC::VectorDouble            m_marketQuotes;
        LTQC::VectorDouble            m_weights;
        LTQC::VectorDouble            m_3m6mWeights;
		LTQC::VectorDouble            m_1m3mWeights;

        LT::Str                       m_fixedLegTenor;
        LT::Str                       m_fixedLegBasis;
        LT::Str                       m_fixedLegCalendar;

		const LTQC::Tenor tenor1m, tenor3m, tenor6m;
	};
}

#endif
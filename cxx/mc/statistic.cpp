#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>

using namespace boost::numeric::ublas;

class mc_statistic {
public:
	mc_statistic() {};
	virtual void dump_one_result(double result) = 0;
	virtual matrix<double> get_statistic() const = 0;
	virtual ~mc_statistic() {};
};

class mean_variance_statistic : public mc_statistic {
	vector<double> res_;
	unsigned long n;
public:
	virtual void dump_one_result(double result);
	virtual matrix<double> get_statistic() const;
};

void mean_variance_statistic::dump_one_result(double result) {
	++n;
	res_(n) = result;
}

matrix<double> mean_variance_statistic::get_statistic() const {
	matrix<double> statistics(2,1);
//	statistics(0,0) = running_sum_ / path_done_;
//	statistics(1,0) = 
};

class quantile_statistic : public mc_statistic {
public:
	quantile_statistic() {};
	virtual void dump_one_result(double result);
	virtual matrix<double> get_statistic() const;	
};

class simulation_engine {
public:
	void run(const product_spec& product,
			 const init_condition& cond,
			 const model_param& params,
			 mc_statistic& statistic); // use of stratgy pattern make this method generic
};

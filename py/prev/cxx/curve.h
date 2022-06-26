class date {};

// interpolant
class biquadratic {
public:
	void setparam();
	double operator()(double x);
};

class cubic_spline {
};

class monotone_convex_spline {
};

class straight_line {
};

class hyperbolic_tension_spline {
};

template<typename strategy_t> class flat {
	// strategy: flat left or right
};

// extrapolant
class flat_extrapolant {
};

class straight_line_extrapolant {

};

enum instrument_enum {cash, futures, cross_currency_swap, cross_currency_funding_swap, cross_currency_ois_swap, fx_forward, tenor_basis_swap, fixed_rate_bond};
// instruments
class calibration_instrument {
public:
	virtual double model_price(const curve_model& model);
	virtual date maturity();
};

class cash_instrument : public calibration_instrument {
public:
	const static instrument_enum type_ = cash;
	double model_price(const curve_model& model);
	date maturity();
};

class cross_currency_funding_swap : public calibration_instrument {
public:
	const static instrument_enum type_ = cross_currency_funding_swap;
	double model_price(const curve_model& model);
	date maturity();
};

class cross_currency_swap : public calibration_instrument {
public:
	const static instrument_enum type_ = cross_currency_swap;
	double model_price(const curve_model& model);
	date maturity();
};

class cross_currency_ois_swap : public calibration_instrument {
public:
	const static instrument_enum type_ = cross_currency_ois_swap;
	double model_price(const curve_model& model);
	date maturity();
};

class futures : public calibration_instrument {
public:
	const static instrument_enum type_ = futures;
	double model_price(const curve_model& model);
	date maturity();
};

class fx_forward : public calibration_instrument {
public:
	const static instrument_enum type_ = fx_forward;
	double model_price(const curve_model& model);
	date maturity();
};

class tenor_basis_swap : public calibration_instrument {
public:
	const static instrument_enum type_ = tenor_basis_swap;
	double model_price(const curve_model& model);
	date maturity();
};

class fixed_rate_bond : public calibration_instrument {
public:
	const static instrument_enum type_ = fixed_rate_bond;
	double model_price(const curve_model& model);
	date maturity();
};

class fra : public calibration_instrument {
public:
	const static instrument_enum type_ = fra;
	double model_price(const curve_model& model);
	date maturity();
};

class ois_basis_swap : public calibration_instrument {

};

class ir_swap : public calibration_instrument {

};

// formulation

class minus_log_discount_form {

};

class spot_rate_form {

};

class instantaneous_forward_form {

};

// knot point placement
class fix_constraint_kpp {
	knotpoint_c operator()(instrument_c& instruments); // filter instruments
};

class configuration {
};

// solver
template<typename curve_model_t> class gauss_newton {
public:
	explicit gauss_newton(const configuration& config);
	bool operator()(curve_model_t& model, const instrument_c& instruments);
};

template<typename curve_model_t> class levenberg_marquardt {
public:
	explicit levenberg_marquardt(const configuration& config);
	bool operator()(curve_model_t& model, const instrument_c& instruments);
};


typedef std::vector<double> vector;
typedef std::deque<calibration_instrument> instrument_c;
typedef std::deque<knotpoint> knotpoint_c;
typedef std::deque<curve> curve_c;
typedef std::function<void(instrument_c&)> filter_t;

void filter_expired(instrument_c& instruments) {

}

void filter_particular_type(instrument_c& instruments) {

}

// instrument filter
class instrument_filter {
	std::deque<filter_t> fs_;
public:
	void operator<<(filter_t& f);
};

enum curve_index_enum {funding, on, 1w, 1m, 2m, 3m, 4m, 5m, 6m, 7m, 8m, 9m, 10m, 11m, 12m};

template<typename left_interpolant_t, typename right_interpolant_t, typename left_extrapolant_t, typename right_extrapolant_t>
class curve {
public:
	explicit curve(const knotpoint_c& knot_points, double separation_point);
};

template<typename formulation_t> class one_curve {
	curve curve_;
public:
	explicit one_curve(const formulation_t& formulation, const knotpoint_c& knotpoints);
	double df(double t, double T, curve_index_enum i);
};

template<typename formulation_t> class multi_curve {
	curve_c curves_;
public:
	explicit multi_curve(const formulation_t& formulation, const knotpoint_c& knotpoints);
	double df(double t, double T, curve_index_enum i);
};

template<typename solver_t, typename formulation_t, typename curve_model_t, typename kpp_t>
class curve_factory {
public:
//	explicit curve_constructor(const curve_model_t& cm, const kpp_t& kpp);
	curve_model operator()(const instrument_c& instruments, const instrument_filter& filter, const configuration& config);
};

template<typename solver_t, typename formulation_t, typename curve_model_t, typename kpp_t>
curve_factory<solver_t, formulation_t, curve_model_t, kpp_t>::operator()(const instrument_c& instruments, const instrument_filter& filter, const configuration& config)
{
	kpp_t kpp;
	formulation_t formulation;
	solver_t solver(configuration);
	filter(instruments);
	knotpoint_c knotpoints = kpp(instruments);
	curve_model_t model(formulation, knotpoints);
	solver(model, instruments);
	return model;
};

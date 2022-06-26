// ch6 c++ design and derivatives pricing

#include <memory>
#include <vector>
#include <boost/shared_ptr.hpp>

class random_base { // consider to templatise the representation of a path
	unsigned d_; // dimension
public:
	explicit random_base(unsigned dimension); // sizeof(unsigned) == sizeof(unsigned long) ?
	unsigned get_dimension() const;
	virtual std::auto_ptr<random_base> clone() const = 0; // which is the best return type? ref, ptr?
	virtual std::vector<double> get_uniform() = 0;
	virtual void skip(unsigned n) = 0;
	virtual void set_seed(unsigned seed) = 0;
	virtual void reset() = 0;
	virtual std::vector<double> get_gaussian() = 0;
	virtual void reset_dimension(unsigned d); // need to be virtual?
	virtual ~random_base();
};

inline unsigned random_base::get_dimension() const {
	return d_;
}

class park_miller {
	long seed_; // can seed be -ve?
public:
	park_miller(long seed = 1);
	long get_one_random_integer();
	void set_seed(long seed);
	static unsigned long max();
	static unsigned long min();
};

class random_park_miller : public random_base {
	park_miller inner_generator_;
	unsigned long initial_seed_;
	double reciprocal;
public:
	random_park_miller(unsigned dimension, unsigned seed = 1);
	virtual std::auto_ptr<random_base> clone() const;
	virtual std::vector<double> get_uniform();
	virtual void skip(unsigned n);
	virtual void set_seed(unsigned seed);
	virtual void reset();
	virtual std::vector<double> get_gaussian();
};

class antithetic_decorator : public random_base {
	boost::shared_ptr<const random_base> inner_generator_; // no tight coupling on life time
	bool antithetic_;
	std::vector<double> antithetic_variates_;
public:
	antithetic_decorator(boost::shared_ptr<const random_base> inner_generator);
	virtual std::auto_ptr<random_base> clone() const;
	virtual std::vector<double> get_uniform();
	virtual void skip(unsigned n);
	virtual void set_seed(unsigned seed);
	virtual void reset();
	virtual std::vector<double> get_gaussian();
};

antithetic_decorator::antithetic_decorator(boost::shared_ptr<const random_base> inner_generator) :
	random_base(*inner_generator), // calls compiler-generated copy constructor
	inner_generator_(inner_generator),
	antithetic_(false),
	antithetic_variates_(inner_generator->get_dimension())
{
	inner_generator_->reset();
};

// ...
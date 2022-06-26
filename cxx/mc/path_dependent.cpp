class path_dependent {
	vector<double> look_at_times;
public:
	path_dependent(const vector<double>& look_at_times);
	virtual ~path_dependent() {};
	const vector<double>& get_look_at_times() const;
	virtual unsigned long max_number_of_cash_flows() const = 0;
	virtual vector<double> possible_cash_flow_times const = 0;
	virtual unsigned long cash_flow(const vector<double>& spots, vector<cash_flow>& generated_flows) const = 0;
	virtual auto_ptr<path_dependent> clone() const = 0;
};
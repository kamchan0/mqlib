class sde {
};

class merton_sde : public sde {
};

class cir_sde : public sde {
};

class hw_sde : public sde {
};

class vasicek_sde : public sde {
};

class numerical_steps {
};

class euler_steps : public numerical_steps {
};

class milten_scheme : public numerical_steps {
};

class strategy {
};

class uniform_steps : public strategy {
};

class variable_steps : public strategy {
};

class solution_path_factory {
};

class vasicek_euler_uniform_solution : public solution_path_factory {
};

class hw_milten_uniform_solution : public solution_path_factory {
};

class floating_rate_payment {
};

class fixed_rate_payment {
};

class instrument_factory {
};

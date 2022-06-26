import scipy.optimize

class curve_factory:
	def __init__(self, instruments, curve_model): # calib_instru = list of (instru_type, t, T, price)
		self.model = curve_model
		self.instruments = instruments
	def bootstrap(self):
		"""
		1. build the error function e which is used in the solver
		2. find initial guesses y of each knot point
		3. get the instrument knot points
		:return: initial guesses (tuple), error function
		"""
		e = self.make_erf()
		t = self.instruments.knot
		y = [i.R for i in self.instruments]
		return t, y, e
	def make_erf(self):
		def e(p, y, x):
			self.model.setp(p)
			return [self.instruments[i](self.model) - z for i, z in enumerate(y)]
		return e
	def make_dfunc(self):
		def J(p, y, x):
			return self.model.J(p)
	def solve(self):
		"""
		ask the interpolator for initial guess p0; pass p0 with other data to Levenburg-Marquardt
		"""
		t, y, e = self.bootstrap()
		p0 = self.model.interpolant.p0(t, y) # TODO: y is not used
		return scipy.optimize.leastsq(e, p0, args=(y,t), full_output=True)

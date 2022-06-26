import scipy.optimize
import bisect
import numpy
import pandas as pd
from typing import Sequence as seq

class multicurve_factory(object):
	def __call__(self, curvedt, config):
		pathname = 'C:/Users/garym/Documents/gwork/gl/usd.json'
		with open(pathname) as f:
			calibration_set = pd.load(f)
		for curvename, instrument_type in calibration_set.items():
			calibration_set[curvename][instrument_type] = pd.normalize(calibration_set[curvename][instrument_type])
		#curvedt, curvenames : seq[str], knotmap: dict, interpolant_switch_dt_map: dict ):	
		curves = multicurve(curvedt, calibration_set.keys(), calibration_set)
		solver = multicurve_solver()
		solver.solve()
		return curves

class multicurve_solver:
	def __init__(self, instruments, marks, knots):
		self.instruments = instruments
		self.marks = marks # y
		self.knots = knots # x
		self.model = multicurve(knots)

	# def bootstrap(self):
	# 	"""
	# 	1. build the error function e which is used in the solver
	# 	2. find initial guesses y of each knot point
	# 	3. get the instrument knot points
	# 	:return: initial guesses (tuple), error function
	# 	"""
	# 	e = self.make_erf()
	# 	t = self.instruments.knot
	# 	y = [i.R for i in self.instruments]
	# 	return t, y, e

	def make_erf(self):
		"""
			|| y_i - f(x_i, p) ||
		"""
		def e(p, y, x):
			self.model.parameters(p)
			return [y - self.instruments[i].value(self.model) for i, y in enumerate(self.marks)]
		return e

	# def make_dfunc(self):
	# 	def jacobian(p, y, x):
	# 		return self.model.J(p)

	def solve(self):
		"""
		ask the interpolator for initial guess p0; pass p0 with other data to Levenburg-Marquardt
		"""
		# t, y, e = self.bootstrap()
		erf = self.make_erf()
		p0 = self.model.initial_guess() # TODO: y is not used
		return scipy.optimize.leastsq(erf, p0, args=(self.marks, self.knots), full_output=True)

"""
	
"""

class curve(object):
	def __init__(self, curvedt, knots: numpy.array, interpolant_switch_dt: pd.datetime64):
		self.knots = knots
		self.curvedt = curvedt
		self.interpolators = (natural_cubic(), flat_constant())
		self.interpolant_switch_dt = interpolant_switch_dt
		self.z = numpy.array([.0] * len(knots))

	def nparams(self):
		return len(self.z)

	# def initial_guess(self):
	# 	for k in self.knots:
	# 		self.z[k] = .0

	def parameters(self, p):
		assert(self.nparams() == len(p))
		for i, val in enumerate(p):
			self.z[i] = val

	def __getitem__(self, dt: pd.datetime64):
		i = numpy.searchsorted(self.knots, dt)
		if dt == self.knots[i]:
			return self.z[i]
		return self.interpolators[int(dt < self.interpolant_switch_dt)][dt]

	def rate(self, start, end):
		t0 = day_count_fraction.actual_actual(curvedt, start)
		t1 = day_count_fraction.actual_actual(curvedt, end)
		if start == self.curvdt:
			return exp(-self[end] * t1)
		return exp(-self[start] * t0)/exp(-self[end] * t1)

class multicurve(object):
	def __init__(self, curvedt, curvenames : seq[str], knotmap: dict, interpolant_switch_dt_map: dict ):
		self.curvenames = curvenames
		for name in curvenames:
			setattr(self, name, curve(curvedt, knotmap[name], interpolant_switch_dt_map[name]))

	def parameters(self, p):
		m = n = 0
		for c in self.curvenames:
			curve = getattr(self, c)
			n = curve.nparams()
			curve.parameters(p[m:m+n])
			m = n

class natural_cubic(object):
	def __init__(self):
		pass

	def __getitem__(self, dt: pd.datetime64):
		return .0

class flat_constant(object):
	def __init__(self):
		pass

	def __getitem__(self, dt: pd.datetime64):
		return .0


# class natural_cubic_spline(object):
# 	def __init__(self):
# 		pass

# 	def initial_guess(self):
# 		return 

# 	def nparams(self):
# 		return 3

# 	def parameters(self, p):


# class flat_constant_spline(object):
# 	def __init__(self):
# 		self.c = 0

# 	def nparams(self):
# 		return 1

# 	def parameters(self, p):
# 		self.c = p

# 	def value(self, x):
# 		return self.c
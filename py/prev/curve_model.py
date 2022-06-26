from abc import abstractmethod

class curve_model:
	@abstractmethod
	def setp(self, p):
		pass
	@abstractmethod
	def __call__(self, t, index):
		pass
	@abstractmethod
	def df(self, t, T, index): # index=d(efault),ois,1m,3m,
		pass

class onecurve(curve_model):
	def setp(self, p):
		self.interpolant.setp(p)
		self.extrapolant.setp(p)
	def __init__(self, interpolant, extrapolant, formulation):
		self.interpolant = interpolant
		self.extrapolant = extrapolant
		self.formulation = formulation
	def __call__(self, t, index='d'):
		return self.interpolant(t) if self.interpolant.tmin <= t < self.interpolant.tmax else self.extrapolant(t)
	def df(self, t, T, index='d'):
		return self.formulation.btransfm(self(T)-self(t))

class multicurve(curve_model):
	def __init__(self, interpolant, extrapolant, formulation):
		pass
	def __call__(self, t, index='d'):
		pass
	def df(self, t, T, index='d'):
		pass
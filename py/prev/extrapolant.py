from abc import abstractmethod

class extrapolant:
	@abstractmethod
	def setp(self, p):
		pass
	@abstractmethod
	def __call__(self, t):
		pass

class flat(extrapolant):
	def setp(self, p):
		pass
	def __init__(self, interpolant):
		self.interpolant = interpolant
	def __call__(self, t):
		return self.interpolant(max(self.interpolant.tmin,min(self.interpolant.tmax,t)))

class straightline(extrapolant):
	def __init__(self, interpolant):
		self.interpolant = interpolant
	def __call__(self, t):
		pass
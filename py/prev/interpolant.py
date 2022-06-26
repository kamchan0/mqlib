from abc import abstractmethod

class interpolant:
	@staticmethod
	def omega(t, T, tau):
		return (tau-t)/(T-t)
	@abstractmethod
	def __call__(self, t):
		pass
	@abstractmethod
	def setp(self, p):
		pass
	@abstractmethod
	def p0(self, t, y):
		pass
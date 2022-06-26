import bizdate, datetime

class fra:
	def __init__(self, td, sd, tau, R, dcc):
		"""
		:param td: value date
		:param sd: start date
		:param tau: duration
		:param R: market rate
		:param dcc: day count convention
		"""
		self.R = R
		self.dcc = dcc
		self.tau = tau
		self.t0 = bizdate.bizdate.af(td,datetime.date.today(),self.dcc)
		self.T = bizdate.bizdate.rangeinyear(td,sd,2,tau,'mf',self.dcc)
	def __call__(self, model):
		"""
		:return: model price
		"""
		return (model.df(self.t0,self.T[0],self.tau)/model.df(self.t0,self.T[1],self.tau)-1)/(self.T[1]-self.T[0])
	def maturity(self):
		return self.T[-1]
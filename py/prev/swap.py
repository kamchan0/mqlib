import interpolant
import bizdate
import datetime

class swap:
	# td: value date, sd: start date, M: nperiods in fix leg, fm: freq of fix leg, fn: freq of flt leg, R: fix leg swap rate, dcc: day count convention
	def __init__(self, td, sd, M, fm, fn, R, dcc):
		self.R = R
		self.dcc = dcc
		self.t0 = bizdate.bizdate.af(td,datetime.date.today(),dcc)
		self.Tm = bizdate.bizdate.rangeinyear(td, sd, M+1, fm, 'none', 'act360')
		N = int(M*(bizdate.interval(interval=fm)/bizdate.interval(interval=fn)))
		self.Tn = bizdate.bizdate.rangeinyear(td, sd, N+1, fn, 'none', 'act360')
		self.fm = fm
		self.fn = fn
	def __call__(self, model): # par rate
		a, f  = 0, 0
		for T in zip(self.Tm[:-1],self.Tn[1:]):
			a += (T[1]-T[0])*model.df(self.t0,T[1])
		for T in zip(self.Tn[:-1],self.Tn[1:]):
			f += model.df(self.t0,T[1])*(model.df(self.t0,T[0],self.fn)/model.df(self.t0,T[1],self.fn)-1)
		return f/a
	def maturity(self):
		return self.Tn[-1]
	@staticmethod
	def make_f(df, t, T, R, alpha): # n = (T-t)/alpha
		j = int((T-t)/alpha)
		def f(z):
			y = 0
			w = R/(1+alpha*R)*alpha
			for i in range(1, j):
				omega = interpolant.interpolant.omega(t,T,t+alpha*i)
				y += w*df[-1]**(1-omega)*z**omega
			x = (R*alpha*df.sum()-1)/(1+alpha*R)
			return z + y + x
		return f
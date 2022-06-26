import numpy
import bisect
import imvarray
import interpolant

class monotone_convex_spline(interpolant.interpolant):
	_CONST_RATE = .05
	def __init__(self, knot):
		self.n = len(knot) + 1 # add (0,0) as an extra knot
		self.knot = numpy.zeros((self.n,))
		self.knot[1:self.n] = knot
		self.tmin = self.knot[0]
		self.tmax = self.knot[-1]
		self.dt = imvarray.imvarray([(j-i)/(k-i) for i, j, k in zip(self.knot[:-2],self.knot[1:-1],self.knot[2:])],1)
		self.p = [0]*self.n
	def setp(self, p):
		self.p[1:] = p
		d = imvarray.imvarray([(u[1]-u[0])/(t[1]-t[0]) for t, u in
							   zip(zip(self.knot[:-1],self.knot[1:]), zip(self.p[:-1],self.p[1:]))],1)
		self.f = [0]*self.n
		for i in range(1,self.n-1):
			self.f[i] = self.dt[i]*d[i+1]+(1-self.dt[i])*d[i]
		self.f[0] = 1.5*d[1]-.5*self.f[1]
		self.f[-1] = 1.5*d[0]-.5*self.f[-2]
		self.g = imvarray.imvarray([self.f[i-1]-d[i] for i in range(1,self.n)],1)
		self.G = imvarray.imvarray([self.f[i]-d[i] for i in range(1,self.n)],1)
	def p0(self, t, y):
		return [k*self._CONST_RATE for k in t]
	def __call__(self, t):
		i = bisect.bisect_left(self.knot, t)
		x = self.omega(self.knot[i-1], self.knot[i], t)
		h = t-self.knot[i-1]
		return (1-x)*self.p[i-1]+x*self.p[i]+self.g[i]*h-(2*self.g[i]+self.G[i])*h*x**2+(self.g[i]+self.G[i])*h*x**3
	def J(self, y):
		pass
import numpy, scipy, scipy.optimize, bisect, imvarray, interpolant

class hyperbolic_tension_spline(interpolant.interpolant):
	def __init__(self, knot, alpha, lmbda, sigma): # knot = [t_1, ..., t_M], sites (t_i, f(t_i))
		M = len(knot)
		self.knot = imvarray.imvarray(numpy.zeros((M+5,)),-1)
		self.knot[1:M+1] = knot
		self.knot[-1] = -alpha
		self.knot[0] = 0.0
		for i in range(1,4):
			self.knot[M+i] = self.knot[M]+alpha*i
		self.lmbda = lmbda
		self.sigma = sigma
	def p0(self, t, f): # solve for b given f values; used for initial guess
		# assume t == knot. Y will then be tri-diagonal. a is the band matrix form of Y
		M = len(f)
		N = 3
		a = numpy.zeros((M, N))
		for j in range(M):
			a[j] = self.B(j+1, self.knot[j+1])[:N]
		# apply boundary conditions
		a[0][1] += (self.y(2)-self.y(0))/(self.y(2)-self.y(1))*a[0][0]
		a[0][2] -= (self.y(1)-self.y(0))/(self.y(2)-self.y(1))*a[0][0]
		a[-1][1] += (self.y(M+1)-self.y(M-1))/(self.y(M)-self.y(M-1))*a[-1][-1]
		a[-1][0] -= (self.y(M+1)-self.y(M))/(self.y(M)-self.y(M-1))*a[-1][-1]
		# construct band matrix for solver
		a[0][0] = a[-1][-1] = 0.
		# ab = numpy.vstack([a[:,2],a[:,1],a[:,0]])
		# self.b = scipy.linalg.solve_banded((1,1),ab,f)
		ab = numpy.diag(a[1:,0],-1)+numpy.diag(a[:,1])+numpy.diag(a[:-1,2],1)
		b = numpy.zeros((M+3,))
		b[1:M+1] = scipy.linalg.solve(ab, f)
		b[0] = b[1]-(self.y(1)-self.y(0))/(self.y(2)-self.y(1))*(b[2]-b[1])
		b[M+1] = b[M]+(self.y(M+1)-self.y(M))/(self.y(M)-self.y(M-1))*(b[M]-b[M-1])
		b[M+2] = 0
		return b
	def psi(self, j, t):
		i = self.sigma*(t-self.knot[j])
		h = self.sigma*(self.knot[j+1]-self.knot[j])
		return (numpy.sinh(i) - i)/self.sigma**2/numpy.sinh(self.sigma*h)
	def phi(self, j, t):
		i = self.sigma*(self.knot[j+1]-t)
		h = self.sigma*(self.knot[j+1]-self.knot[j])
		return (numpy.sinh(i)-i)/self.sigma**2/numpy.sinh(self.sigma*h)
	def dpsi(self, j, t):
		h = self.sigma*(self.knot[j+1]-self.knot[j])
		return (numpy.cosh(self.sigma*(t-self.knot[j]))-1)/self.sigma/numpy.sinh(self.sigma*h)
	def dphi(self, j, t):
		h = self.sigma*(self.knot[j+1]-self.knot[j])
		return (1-numpy.cosh(self.sigma*(self.knot[j+1]-t)))/self.sigma/numpy.sinh(self.sigma*h)
	def z(self, j):
		return self.psi(j-1,self.knot[j])-self.phi(j,self.knot[j])
	def dz(self, j):
		return self.dpsi(j-1,self.knot[j])-self.dphi(j,self.knot[j])
	def y(self, j):
		return self.knot[j]-self.z(j)/self.dz(j)
	def B(self, j, t):
		f0 = self.phi(j,t)/self.dz(j)
		f1 = self.psi(j,t)/self.dz(j+1)
		h0 = self.y(j)-self.y(j-1)
		h1 = self.y(j+1)-self.y(j)
		h2 = self.y(j+2)-self.y(j+1)
		return numpy.array([f0/h0, 1-(t-self.y(j)+f0)/h1-f0/h0+f1/h1, (t-self.y(j)+f0)/h1-f1/h2-f1/h1, f1/h2])
	def __call__(self, t, b): # t \in [t_j, t_{j+1}], b = [b_0, b_{M+1}]
		j = self.knot.invindex(bisect.bisect_left(self.knot.array(), t))
		return numpy.inner(b[j-1:j+3],self.B(j,t))
import numpy as np
import scipy, scipy.optimize

def s(t, df): # par rate
	a=f=0
	for i in range(1,len(df)):
		a += (t[i]-t[i-1])*df[i]
		f += df[i]*(df[i-1]/df[i]-1)

	# a = reduce(lambda a,T: a+(t[1]-t[0])*df[i], zip(t[:-1],t[1:]),0)
	# f = reduce(lambda f,T: f+model.df(self.t,T[1])*(model.df(self.t,T[0],self.fn)/model.df(self.t,T[1],self.fn)-1), zip(self.Tn[:-1],self.Tn[1:]),0)
	return f/a

a=np.array([.5,.5,0.,0.,0.])
b=np.array([0.,0.,0.,-.25,1.25])
A=np.matrix([a, np.roll(a,1), np.roll(a,2), np.roll(a,3),b])
f=map(lambda i: .005*i, range(5))
print "f\n", f
F = scipy.linalg.solve(A,f)
fd = F[0]-.5*(f[0]-F[0])+ F.tolist()
print "fd\n", fd
df=map(lambda x: np.exp(-x), f)
print "df\n", df
t=np.linspace(0,2,5)
print "t\n", t
p = s(t, df)
print p

import numpy

class imvarray(numpy.ndarray):
	def __new__(cls, d, n):
		nd = numpy.asarray(d)
		return super(imvarray, cls).__new__(cls,(nd.size,), dtype=nd.dtype, buffer=nd).view(imvarray)
	def __init__(self, d, n):
		self.f = lambda i: i-n
		self.g = lambda i: i+n
	def __getitem__(self, i):
		return super(imvarray, self).__getitem__(self.f(i))
	def __setitem__(self, i, y):
		return super(imvarray, self).__setitem__(self.f(i), y)
	def __getslice__(self, i, j):
		return super(imvarray, self).__getslice__(self.f(i), self.f(j))
	def __setslice__(self, i, j, y):
		return super(imvarray, self).__setslice__(self.f(i), self.f(j), y)
	def __delslice__(self, i, j):
		return super(imvarray, self).__delslice__(self.f(i), self.f(j))
	def array(self):
		return super(imvarray, self).__array__()
	def invindex(self, i):
		return self.g(i)
	def __str__(self):
		return numpy.array2string(self.array())
	def __repr__(self):
		return numpy.array_repr(self.array())
import unittest
import monotone_convex_spline as mcs
import numpy
import matplotlib.pyplot as plt

# class DebugTestCase(unittest.TestCase):
# 	@classmethod
# 	def debug(cls):
# 		unittest.defaultTestLoader.loadTestsFromTestCase(cls).debug()

# class test_monotone_convex_spline(DebugTestCase):
# 	def test_call(self):
# 		instruments = [(.5,.0275),(1.,.031),(1.5,.033),(2.,.0343),(2.5,.0353),(3.,.033),(4.,.0378),(5.,.0395),(7.,.0425,),(10.,.045),(12.,.0465),(15.,.0478),(20.,.0488),(30.,.0485)]
# 		x = [i[0] for i in instruments]
# 		y = [i[1] for i in instruments]
# 		spline = mcs.monotone_convex_spline(x)
# 		spline.setp(y)
# 		k = numpy.linspace(x[0],x[-1],num=100)
# 		z = map(spline, k)
# 		plt.plot(k, z)
# 		plt.show()
#
# if __name__ == 'main':
# 	test_monotone_convex_spline.debug()

class test_monotone_convex_spline(unittest.TestCase):
	def test_call(self):
		instruments = [(.5,.0275),(1.,.031),(1.5,.033),(2.,.0343),(2.5,.0353),(3.,.033),(4.,.0378),(5.,.0395),(7.,.0425,),(10.,.045),(12.,.0465),(15.,.0478),(20.,.0488),(30.,.0485)]
		x = [i[0] for i in instruments]
		y = [i[1] for i in instruments]
		spline = mcs.monotone_convex_spline(x)
		spline.setp(y)
		k = numpy.linspace(x[0],x[-1],num=100)
		z = map(spline, k)
		plt.plot(k, z)
		plt.show()
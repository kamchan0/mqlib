# import hyperbolic_tension_spline as hts
import monotone_convex_spline as mcs
import formulation as fm
import calib_instru as calib
import extrapolant as ex
import matplotlib.pyplot as plt
import numpy
import curve_factory
import curve_model
import unittest
import datetime
import swap
import fra

class test_curve_library(unittest.TestCase):
	# def test_fra_pricing(self):
	# 	data = numpy.array([('swap',0.,'6m',.0275,'6m','6m'),('swap',0.,'1y',.031,'6m','6m')])
	# 	ci = calib.calib_instru(data)
	# 	# print "knot", ci.knot
	# 	spline = mcs.monotone_convex_spline(ci.knot)
	# 	extrapolant = ex.flat(spline)
	# 	model = curve_model.onecurve(spline, extrapolant, form.lg)
	# 	model.p = [0.005930867, 0.013371476]
	# 	td = datetime.date.today()
	# 	f = fra.fra(td, td, '1m', 0.5, 'act360')
	# 	price = f(model)
	# 	print "price", price

	# def test_swap_pricing(self):
	# 	data = numpy.array([('swap',0.,'6m',.0275,'6m','6m'),('swap',0.,'1y',.031,'6m','6m')])
	# 	ci = calib.calib_instru(data)
	# 	# print "knot", ci.knot
	# 	spline = mcs.monotone_convex_spline(ci.knot)
	# 	extrapolant = ex.flat(spline)
	# 	model = curve_model.onecurve(spline, extrapolant, form.lg)
	# 	model.p = [0.005930867, 0.013371476]
	# 	td = datetime.date.today()
	# 	s = swap.swap(td, td, 2, '6m', '6m', 0.5, 'act360')
	# 	price = s(model)
	# 	print "price", price

	# def test_monotone_convex_spline(self):
	# 	# alpha = .5
	# 	# instru = [(.5,.0275),(1.,.031),(1.5,.033),(2.,.0343),(2.5,.0353),(3.,.033),(4.,.0378),(5.,.0395),(7.,.0425,),(10.,.045),(12.,.0465),(15.,.0478),(20.,.0488),(30.,.0485)]
	# 	# # knot = numpy.linspace(alpha,instru[-1,0],instru[-1,0]/alpha)
	# 	# knot = [i[0] for i in instru]
	# 	# mcs = monotone_convex_spline(knot)
	# 	# # f = imvarray.imvarray(instru[:,1].copy(),1)
	# 	# f = [i[1] for i in instru]
	# 	# mcs.setp(f)
	# 	# y = mcs(29.9)
	# 	# print "final:", y
	# 	instruments = [(.5,.0275),(1.,.031),(1.5,.033),(2.,.0343),(2.5,.0353),(3.,.033),(4.,.0378),(5.,.0395),(7.,.0425,),(10.,.045),(12.,.0465),(15.,.0478),(20.,.0488),(30.,.0485)]
	# 	x = [i[0] for i in instruments]
	# 	y = [i[1] for i in instruments]
	# 	spline = mcs.monotone_convex_spline(x)
	# 	spline.setp(y)
	# 	k = numpy.linspace(x[0],x[-1],num=100)
	# 	z = map(spline, k)
	# 	plt.plot(k, z)
	# 	plt.show()

	# def test_curve_factory(self):
	# 	# assume knot to be the same as all payment dates in the swap with longest maturity
	# 	# not necessarily true as user can select less points to get a coarser curve
	# 	# alpha = .5
	# 	# instru = numpy.array([('fra',0.,.019231,.00411),('swap',0.,.5,.0275),('swap',0.,1.,.031),('swap',0.,1.5,.033),('swap',0.,2.,.0343),('swap',0.,2.5,.0353),('swap',0.,3.,.033),('swap',0.,4.,.0378),('swap',0.,5.,.0395),('swap',0.,7.,.0425,),('swap',0.,10.,.045),('swap',0.,12.,.0465),('swap',0.,15.,.0478),('swap',0.,20.,.0488),('swap',0.,30.,.0485)])
	# 	# knot = numpy.linspace(alpha,instru[-1,2],instru[-1,2]/alpha)
	#
	# 	# tension spline
	# 	# hts = hyperbolic_tension_spline(knot, alpha, 50, 1)
	# 	# cstr = curve_factory(instru, log_formulation, hts)
	# 	# b0 = cstr.solve()
	#
	# 	# monotone convex
	# 	# f = imvarray.imvarray(instru[:,1].copy(),1)
	# 	# mcs = monotone_convex_spline(knot)
	# 	# model = onecurve(mcs, None, 0, 30)
	# 	# cstr = curve_factory(instru, log_formulation, model)
	# 	# numpy.seterr(all='raise')
	# 	data = [
	# 		('fra',0.,'1w',.00411),
	# 		('swap',0.,'6m',.0275,'6m','6m'),
	# 		('swap',0.,'1y',.031,'6m','6m'),
	# 		('swap',0.,'18m',.033,'6m','6m'),
	# 		('swap',0.,'2y',.0343,'6m','6m'),
	# 		('swap',0.,'30m',.0353,'6m','6m'),
	# 		('swap',0.,'3y',.0367,'6m','6m'),
	# 		('swap',0.,'4y',.0378,'6m','6m'),
	# 		('swap',0.,'5y',.0395,'6m','6m'),
	# 		('swap',0.,'7y',.0425,'6m','6m'),
	# 		('swap',0.,'10y',.045,'6m','6m'),
	# 		('swap',0.,'12y',.0465,'6m','6m'),
	# 		('swap',0.,'15y',.0478,'6m','6m'),
	# 		('swap',0.,'20y',.0488,'6m','6m'),
	# 		('swap',0.,'30y',.0485,'6m','6m')]
	# 	# data = [
	# 	# 	('swap',0.,'6m',0.0275,'6m','6m'),
	# 	# 	('swap',0.,'1y',0.031,'6m','6m'),
	# 	# 	('swap',0.,'18m',.033,'6m','6m'),
	# 	# 	('swap',0.,'2y',.0343,'6m','6m'),
	# 	# 	('swap',0.,'30m',.0353,'6m','6m'),
	# 	# 	('swap',0.,'3y',.033,'6m','6m'),
	# 	# 	('swap',0.,'4y',.0378,'6m','6m'),
	# 	# 	('swap',0.,'5y',.0395,'6m','6m'),
	# 	# 	('swap',0.,'7y',.0425,'6m','6m')
	# 	# 	]
	# 	instruments = calib.calib_instru(data)
	# 	spline = mcs.monotone_convex_spline(instruments.knot)
	# 	extrapolant = ex.flat(spline)
	# 	model = curve_model.onecurve(spline, extrapolant, fm.lg)
	# 	factory = curve_factory.curve_factory(instruments, model)
	# 	sol, cov_x, info, mesg, ier = factory.solve()
	# 	print("Result {}".format(sol))

	def test_plot(self):
		data = [
			('fra',0.,'1w',.00411),
			('swap',0.,'6m',.0275,'6m','6m'),
			('swap',0.,'1y',.031,'6m','6m'),
			('swap',0.,'18m',.033,'6m','6m'),
			('swap',0.,'2y',.0343,'6m','6m'),
			('swap',0.,'30m',.0353,'6m','6m'),
			('swap',0.,'3y',.033,'6m','6m'),
			('swap',0.,'4y',.0378,'6m','6m'),
			('swap',0.,'5y',.0395,'6m','6m'),
			('swap',0.,'7y',.0425,'6m','6m'),
			('swap',0.,'10y',.045,'6m','6m'),
			('swap',0.,'12y',.0465,'6m','6m'),
			('swap',0.,'15y',.0478,'6m','6m'),
			('swap',0.,'20y',.0488,'6m','6m'),
			('swap',0.,'30y',.0485,'6m','6m')]
		instruments = calib.calib_instru(data)
		spline = mcs.monotone_convex_spline(instruments.knot)
		k = numpy.linspace(instruments.knot[0],instruments.knot[-1]-.25,num=100)
		sol = [7.99134735e-05, 1.37316764e-02, 3.12131803e-02, 4.98064814e-02,
			   6.91994786e-02, 8.88829814e-02, 1.11160325e-01, 1.52679349e-01,
			   1.99871539e-01, 3.02843303e-01, 4.60644387e-01, 5.74292261e-01,
			   7.41279192e-01, 1.01131265e+00, 1.47901788e+00]
		spline.setp(sol) # unnecssary
		z = map(lambda t: (spline(t+.25)-spline(t))/.25, k)
		plt.plot(k, z)
		plt.show()

if __name__ == '__main__':
	unittest.main()
import numpy
from abc import abstractmethod

class formulation:
	@staticmethod
	@abstractmethod
	def ftransfm(df):
		pass
	@staticmethod
	@abstractmethod
	def btransfm(y):
		pass

class lg(formulation):
	@staticmethod # transform discount factors
	def ftransfm(df):
		return -numpy.log(df)
	@staticmethod # inverse transform
	def btransfm(y):
		return numpy.exp(-y)

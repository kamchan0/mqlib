import bizdate, fra, swap, datetime

class calib_instru:
	def __swap(self, details):
		t, maturity, R, fm, fn = details
		m = bizdate.interval(interval=maturity).years()
		f = bizdate.interval(interval=fm).years()
		return swap.swap(self.__td, self.__td, m/f, fm, fn, R, 'act360')
	def __fra(self, details):
		t, maturity, R = details
		return fra.fra(self.__td, self.__td, maturity, R, 'act360')
	def __init__(self, instrument_data):
		self.__instrud = {'fra': self.__fra, 'swap': self.__swap}
		self.__td = datetime.date.today() # todo: can get rid of this and why not using t?
		self.instruments = [self.__instrud[d[0]](d[1:]) for d in instrument_data]
		self.knot = [d.maturity() for d in self.instruments]
	def __getitem__(self, i):
		return self.instruments[i]
import datetime, dateutil.rrule, calendar, re

class baddate:
	@staticmethod
	def mf(d):
		return d + datetime.timedelta(days=calendar.SUNDAY-d.weekday()+1) # modified following
	@staticmethod
	def none(d):
		return d
		
bdm = {'mf': baddate.mf, 'none': baddate.none}

class act360:
	daysinyear = 360.0
	@classmethod
	def af(cls, sd, ed):
		return (ed-sd).days/cls.daysinyear

class act365:
	daysinyear = 365.0
	@classmethod
	def af(cls, sd, ed):
		return (ed-sd).days/cls.daysinyear

class actact:
	@classmethod
	def af(cls, sd, ed):
		if sd.year != ed.year:
			e = (ed-datetime.date(ed.year,1,1)).days
			d = e/(366.0 if calendar.isleap(ed.year) else 365.0)
			e = (datetime.date(ed.year,1,1)-sd).days
			d += e/(366.0 if calendar.isleap(sd.year) else 365.0)
		else:
			d = (ed-sd).days/(366.0 if calendar.isleap(sd.year) else 365.0)
		return d

class interval:
	__rruled = {
		'y': dateutil.rrule.YEARLY,
		'm': dateutil.rrule.MONTHLY,
		'w': dateutil.rrule.WEEKLY,
		'd': dateutil.rrule.DAILY}
	__inyear = [1.,12.,52.,365.25]
	__inday = [365.,30.,7.,1.]
	unit = freq = -1
	def __init__(self, **args):
		if 'interval' in args:
			m = re.search('(\d+)(\w)',args['interval'])
			try:
				self.unit = self.__rruled[m.group(2)]
				self.freq = int(m.group(1))
			except:
				raise ValueError('interval of incorrect format')
		elif 'unit' in args and 'freq' in args:
			self.unit = args['unit']
			self.freq = args['freq']
		elif 'timedelta' in args:
			self.unit = self.__rruled['d']
			self.freq = args['timedelta'].days
	def __uniteq(self, rhs):
		if rhs.unit != self.unit:
			raise ValueError('incompatible unit in operand')		
	def __add__(self, rhs):
		self.__uniteq(rhs)
		return interval(unit=self.unit, freq=self.freq+rhs.freq)
	def __sub__(self, rhs):
		self.__uniteq(rhs)
		return interval(unit=self.unit, freq=self.freq-rhs.freq)
	def __div__(self, rhs):
		self.__uniteq(rhs)
		return self.freq/float(rhs.freq)
	def __mul__(self, rhs):
		raise NotImplementedError
	def __str__(self):
		return '{}{}'.format(self.freq,self.__rruled.keys()[self.unit])
	def years(self):
		return self.freq/self.__inyear[self.unit]
	def days(self):
		return self.freq*self.__inday[self.unit]

class bizdate:
	@classmethod
	def range(cls, sd, nperiod, freq, bdc):
		inl = interval(interval=freq)
		d = [e.date() for e in list(dateutil.rrule.rrule(inl.unit, count=nperiod, dtstart=sd, interval=inl.freq))]
		return [bdm[bdc](e) if e.weekday()>calendar.FRIDAY else e for e in d]
	@classmethod
	def rangeinyear(cls, td, sd, nperiod, freq, bdc, dcc):
		d = cls.range(sd, nperiod, freq, bdc)
		return [cls.af(td,ed,dcc) for ed in d]
	@staticmethod
	def af(sd, ed, dcc):
		return globals()[dcc].af(sd,ed)
		
if __name__ == '__main__':
	print bizdate.range(datetime.date(2013,4,13),10,'6m','mf')
	print bizdate.af(datetime.date(2015,4,13), datetime.date(2016,4,15), 'act365')
	# t = interval(interval='2.5y')
	# print t
	# t = interval(interval='5y')
	# print t

	# td = datetime.timedelta(days=90)
	# t = interval(timedelta=td)
	# print t.years()

	print bizdate.rangeinyear(datetime.date(2013,4,13),datetime.date(2013,4,13),10,'6m','mf','act365')
	
	# t2 = interval(interval='3m')
	# print t/t2
	# pass
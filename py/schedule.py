import pandas as pd
import datetime as dt
from pandas.tseries.holiday import \
	AbstractHolidayCalendar, \
	Holiday, \
	USMartinLutherKingJr, \
	USPresidentsDay, \
	GoodFriday, \
	USMemorialDay, \
	USLaborDay, \
	USThanksgivingDay, \
	nearest_workday, \
	MO, \
	next_monday, \
	next_monday_or_tuesday

class NYCCalendar(AbstractHolidayCalendar):
	rules = [
		Holiday('NewYearsDay', month=1, day=1, observance=nearest_workday),
		USMartinLutherKingJr,
		USPresidentsDay,
		GoodFriday,
		USMemorialDay,
		Holiday('USIndependenceDay', month=7, day=4, observance=nearest_workday),
		USLaborDay,
		USThanksgivingDay,
		Holiday('Christmas', month=12, day=25, observance=nearest_workday)]

class LDNCalendar(AbstractHolidayCalendar):
	rules = [
		Holiday('New Years Day', month=1, day=1, observance=next_monday),
		GoodFriday,
		Holiday('Early May Bank Holiday', month=5, day=1, offset=pd.tseries.offsets.DateOffset(weekday=MO(1))),
		Holiday('Spring Bank Holiday', month=5, day=31, offset=pd.tseries.offsets.DateOffset(weekday=MO(-1))),
		Holiday('Summer Bank Holiday', month=8, day=31, offset=pd.tseries.offsets.DateOffset(weekday=MO(-1))),
		Holiday('Christmas Day', month=12, day=25, observance=next_monday),
		Holiday('Boxing Day', month=12, day=26, observance=next_monday_or_tuesday)]

class calendar_factory(object):
	def __call__(self, args):
		self.rules = []
		for name in args:
			klass = globals()['{}Calendar'.format(name)]
			self.rules.extend(klass.rules)
		return type('Calendar', (AbstractHolidayCalendar,), {'rules': self.rules})()

class schedule(object):
	convention_map = {'1B': {'days': 1}, 'Q': {'months': 3}, 'SA': {'months': 6}, 'A': {'months': 12}}

	def __call__(self, start_date, end_date, frequency, bad_day_convention, pay_lag, calendars):
		if frequency == '1B':
			sched = pd.date_range(start=start_date, end=end_date, freq=pd.tseries.offsets.CustomBusinessDay(1, calendar=calendar_factory()(calendars)))
			return sched
		sched = pd.date_range(start=start_date, end=end_date, freq=pd.tseries.offsets.DateOffset(**self.convention_map[frequency]))
		bdc = pd.tseries.offsets.CustomBusinessDay(0, calendar=calendar_factory()(calendars))
		return sched + bdc

import itertools

class gitertools(object):
	@staticmethod
	def pairwise(iterable):
		a, b = itertools.tee(iterable)
		next(b, None)
		return zip(a, b)

class instrument(object):
	pass

class irswap(instrument):
	def __init__(self, currency):
		pass



'''
	market: contains market date, curve, fixings, ...
	index_convention: contains conventions, ...
'''
class day_count_fraction:
	@staticmethod
	def days_in_year(date):
		return 366 if start.is_leap_year() else 365

	@staticmethod
	def actual_actual(start, end):
		if start.year == end.year:
			return (end - start) / days_in_year(start)
		year_ends = pd.date_range(start=start, end=end, freq=pd.tseries.offsets.YearEnd())
		return (end - year_ends[-1]) / days_in_year(end) + (year_ends[0] - start) / days_in_year(start) + len(year_ends) - 1

	@staticmethod
	def actual_360(start, end):
		return (end - start) / 360

	@staticmethod
	def actual_365(start, end):
		return (end - start) / 365

	@staticmethod
	def _30_360(start, end):
		if start > 30: start = 30
		if start >= 30 and end == 31: end = 30
		dY = (end - start) // np.timedelta64(1, 'Y')
		dM = (end - start) // np.timedelta64(1, 'M') - dY * 12
		dD = end - end.replace(day=1)
		return (360*dY + 30*dM + dD) / 360

	func_map = {'AA': actual_actual, 'A360': actual_360, 'A365': actual_365, '30360': _30_360}

class index_convention(object):
	def __init__(self):
		pathname = 'C:/Users/garym/Documents/gwork/gl/index_convention.json' # relative path
		self.data = pd.read_json(pathname, orient='index')

	def __getitem__(self, index):
		return self.data.loc[index]

class future_info(object):
	def __init__(self):
		pathname = 'C:/Users/garym/Documents/gwork/gl/future_info.json' # relative path
		self.data = pd.read_json(pathname, orient='index')

	def __getitem__(self, contract_code):
		return self.data.loc[contract_code]

import re

class future(instrument):
	contract_month_code = {'F': 1, 'G': 2, 'H': 3, 'J': 4, 'K': 5, 'M': 6, 'N': 7, 'Q': 8, 'U': 9, 'V': 10, 'X': 11, 'Z': 12}
	regex = r'(\w{2})(\w)(\d{,2})'

	def __init__(self, ticker, index_convention):
		m = re.match(self.regex, ticker)
		self.contract_code = m.group(1)
		month = self.contract_month_code[m.group(2)]
		year = int(m.group(3))
		mod = 10 if year < 10 else 100
		year += pd.Timestamp.today().year // mod * mod 
		self.delivery_month = pd.Timestamp(year=year, month=month, day=1)

class daily_averaging_future(future):
	def __init__(self, ticker, index_convention, future_info):
		super().__init__(ticker, index_convention)
		self.future_info = future_info[self.contract_code]
		self.index_convention = index_convention[self.future_info.Index]
		self.day_count_fraction = day_count_fraction.func_map[self.index_convention.IndexDCC]
		self.start = self.delivery_month
		self.end = self.delivery_month + pd.tseries.offsets.MonthEnd()
		self.schedule = schedule()(self.start, self.end, self.index_convention.Tenors, self.index_convention.IndexBDC, self.index_convention.PayLag, self.index_convention.ResetHolidays)

	def value(self, curves):
		for start, end in gitertools.pairwise(self.schedule):
			rate += self.day_count_fraction(start, end) * curves.ois.rate(start, end)
		return 1 - rate * self.day_count_fraction(self.start, self.end)

# if __name__ == '__main__':
# 	nyc = NYCCalendar()
# 	start_dt = pd.to_datetime('today').normalize() + pd.tseries.offsets.CustomBusinessDay(2, calendar=nyc)
# 	end_dt = start_dt + pd.tseries.offsets.DateOffset(years=10)
# 	schul = schedule()
# 	print(schul(start_dt, end_dt, 'A', 'MF', 2, ('LDN', 'NYC')))
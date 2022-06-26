import pytest
from schedule import *

def test_fedfundfuture():
	fi = future_info()
	# fi['FF']
	ic = index_convention()
	ff = daily_averaging_future('FFM1', ic, fi)
	print(ff.start)
	print(ff.end)
	print(ff.schedule)


test_fedfundfuture()
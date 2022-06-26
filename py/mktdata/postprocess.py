# import glob

# basepath = 'C:/Users/garym/Documents/gwork/gl/mktdata/refinitiv/raw_bars/'
# for path in glob.iglob(basepath, recursive=True):
# 	print(path)

import os
import bz2
import re

basepath = 'C:/Users/garym/Documents/gwork/gl/mktdata/refinitiv/raw_bars/20201028/20201028'
for folder, _, files in os.walk(basepath):
	for name in files:
		filepath = os.path.join(folder, name)
		if name.endswith('.txt'):
			os.remove(filepath)
			continue
		if name.endswith('.bz2'):
			m = re.match(r'.*\(Ric_(.+)\)(\..+).bz2', name)
			ticker = m.group(1)
			ext = m.group(2)
			hndl = bz2.BZ2File(filepath)
			data = hndl.read()
			open('{}.{}'.format(ticker, ext), 'wb').write(data)
			hndl.close()
			os.remove(filepath)

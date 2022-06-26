hols: ("SSDS"; enlist csv) 0: `$":gl/mktdata/lch-calendar.csv"; hols: `centre`code`date`occasion xcol hols
bds: {[s; e] s + til (1+e-s)} [2020.01.01; 2020.12.31] except exec date from hols where code = `EUTA
paths: paths where count each paths: { key ` sv (`:gl;`mktdata;`refinitiv;`raw_bars;`$"-" sv "." vs string x;`$"FUTP=B_JAN21=Price(Ric_LCOF1).csv") } each bds
irfut: raze {("S Z  FFFFFFF   F   F   F    "; enlist csv) 0: x} each paths
irfut: `sym`datetime`open`high`low`last`volume`txn`bid`ask`bidsize`asksize xcol irfut
irfut: `datetime`sym xkey irfut
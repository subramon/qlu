import numpy as np
import sys
#np.random.seed(1)
vals = np.random.geometric(0.5,1000000)
idx = []
for i in xrange(1, 21):
   idx.append(i*5)
quantiles = np.percentile(vals, idx)
for i,q in zip(idx, quantiles):
   print(i,int(q))
f = open("data3.txt", 'w')
for i in vals:
   f.write(str(i) + "\n")
f.close()

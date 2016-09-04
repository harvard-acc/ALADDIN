#!/usr/bin/env python

import sys
import numpy as np

points = np.array( [(float(x),float(y),float(z)) for (x,y,z) in map(lambda L: L.strip().split(), sys.stdin.readlines())] )

N = points.shape[0]

# Johan Philip. "The Probability Distribution of the Distance between two Random Points in a Box", numerically estimated with Mathematica
# an underestimate, actually, because we set a lower bound at the vdW threshold
expected_dist = 12.69

dists = np.zeros((N,N))
for (i,A) in enumerate(points):
  for (j,B) in enumerate(points):
    if i!=j:
      dists[i,j] = np.linalg.norm(A-B)
    else:
      dists[i,j] = expected_dist;

print 'Distance mean:',np.mean(dists)
print 'Distance variance:',np.var(dists)
minpair = np.unravel_index(np.argmin(dists), (N,N))
print 'Closest pair:',minpair, np.min(dists)
print '  p0',points[minpair[0]]
print '  p1',points[minpair[1]]
maxpair = np.unravel_index(np.argmax(dists), (N,N))
print 'Furthest pair:',maxpair, np.max(dists),'( max',20.0*np.sqrt(3.),')'
print '  p0',points[maxpair[0]]
print '  p1',points[maxpair[1]]

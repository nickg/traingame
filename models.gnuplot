# Use this file to tweak the model parameters for engines
# All force values are in kN

# The x-value where the two curves intersect should match
# the maximum speed for the real engine

# Stationary tractive effort
ps(x) = 34.7

# Knee point where tractive effort tails off (m/s)
knee = 10.0 

# Continuous tractive effort
pc(x) = (ps(0) * knee)/x

p(x) = x < knee ? ps(x) : pc(x)

# Resistance
# There should be at least an order of magnitude difference a << b << c
a = 4
b = 0.05
c = 0.006
q(x) = a + b*x + c*x*x

set xrange [0:100]
set yrange [0:50]
set xlabel "Velocity (m/s)"
set ylabel "Force (kN)"

plot p(x) title 'Tractive effort', \
     q(x) title 'Resistance'
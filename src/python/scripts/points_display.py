from re import X
from tkinter import Y
from matplotlib import projections
import matplotlib.pyplot as plt
import csv

X = []
Y = []
Z = []

with open('generated.txt', newline='') as file:
    r = csv.reader(file, delimiter=',')
    for row in r:
        X.append(float(row[0]))
        Y.append(float(row[2]))
        Z.append(float(row[1]))
        
fig = plt.figure()
ax = fig.add_subplot(projection='3d')

ax.scatter(X, Y, Z, marker='o')

X.clear()
Y.clear()
Z.clear()

with open('parentMesh.txt', newline='') as file:
    r = csv.reader(file, delimiter=',')
    for row in r:
        X.append(float(row[0]))
        Y.append(float(row[2]))
        Z.append(float(row[1]))
        
ax.scatter(X, Y, Z, marker='o')

X.clear()
Y.clear()
Z.clear()

with open('secondMesh.txt', newline='') as file:
    r = csv.reader(file, delimiter=',')
    for row in r:
        X.append(float(row[0]))
        Y.append(float(row[2]))
        Z.append(float(row[1]))
        
ax.scatter(X, Y, Z, marker='o')

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')

ax.legend(['generated', 'parent', 'second'])

plt.show()

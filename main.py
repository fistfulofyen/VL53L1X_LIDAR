
import serial
import copy
import math
import numpy as np
import open3d as o3d

number = 169  # the number of coordinates that the open3d will process before creating the 3d imagine.
round = 4 # was 4

if __name__ == "__main__":

    s = serial.Serial("COM4", 115200) #the COM port is changing all the time, so we need to change it by checking COM in device manager.
    lines = []
    print("Opening: " + s.name)
    f = open("400208135.xyz", "w")
    for i in range(number * round):
        x = s.readline()  # read one line each time
        c = x.decode()  # convert byte type to str in order to use it for open3d.
        print(c)
        f.write(c)
        lines.append(c)

    print("Closing: " + s.name)
    s.close()
    f.close()

    pcd = o3d.io.read_point_cloud("400208135.xyz")
    print(pcd)
    print(np.asarray(pcd.points))

    pt1 = 0  # set variable point in order to connect each X Y Z coordinate below
    pt2 = 1
    pt3 = 2
    pt4 = 3
    po = 0

    lines = []

    for x in range(number * round):  # here, we create the planes that holds the each coordinates
        lines.append([pt1 + po, pt2 + po])
        lines.append([pt2 + po, pt3 + po])
        lines.append([pt3 + po, pt4 + po])
        lines.append([pt4 + po, pt1 + po])
        po += 4;


    pt1 = 0   # after the above process, we need to reset each variable
    pt2 = 1
    pt3 = 2
    pt4 = 3
    po = 0
    do = 4

    for x in range(number * round - 1):  # here, we connect lines between each coordinates
        lines.append([pt1 + po, pt1 + do + po])
        lines.append([pt2 + po, pt2 + do + po])
        lines.append([pt3 + po, pt3 + do + po])
        lines.append([pt4 + po, pt4 + do + po])
        po += 4;

    line_set = o3d.geometry.LineSet(points=o3d.utility.Vector3dVector(np.asarray(pcd.points)),
                                    lines=o3d.utility.Vector2iVector(lines))

    # Show the open3d results

    o3d.visualization.draw_geometries([line_set])

# See PyCharm help at https://www.jetbrains.com/help/pycharm/

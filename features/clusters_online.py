from sklearn.cluster import KMeans
import numpy as np

n_clusters = 8
with open("features.txt", "r") as f:
    max_its = 50000 
    fts = []
    for v in f.readlines():
        fts.append(list(map(lambda x: float(x), v.strip().split(' '))))
        max_its-=1
        if max_its <= 0:
            break
    a = np.array(fts)
    kmeans = KMeans(n_clusters=n_clusters).fit(a)
    counts = {x:0 for x in range(n_clusters)}
    for label in kmeans.labels_:
        counts[label]+=1
    print(counts)
    out = open("cluster_centers.txt", "w")
    for c in kmeans.cluster_centers_:
        for ft in c:
            out.write(str(ft) + " ")
        out.write('\n')
    out.close()



        

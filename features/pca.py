from sklearn.decomposition import PCA
import numpy as np

n_comp = 3
with open("features.txt", "r") as f:
    max_its = 50000 
    fts = []
    for v in f.readlines():
        fts.append(list(map(lambda x: float(x), v.strip().split(' '))))
        max_its-=1
        if max_its <= 0:
            break
    a = np.array(fts)
    pca = PCA(n_components=n_comp).fit(a)
    out = open("pca.txt", "w")
    pca.components_
    for c in pca.components_:
        for ft in c:
            out.write(str(ft) + " " )
        out.write('\n')
    out.close()
    #tr = pca.transform(a[:50]) / 2 + 0.5 
    #print(tr)
    
    


        

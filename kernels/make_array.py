f = open("kernels.txt", "r")
cols = 5
rows = 0
row_nums = []
for l in f.readlines():
    if l[0] != '\n':
        rows+=1
        row_nums.append(l.replace('\t', ' ').strip().split(' '))

print("int rows = " + str(rows) + ";")
row_str = '\n'.join(["{" + ', '.join(x) + "}," for x in row_nums])
print("double kernels[][" + str(cols) + "] = {\n" + row_str + "\n};")

f.close()

import csv,glob,json
results=[]
for path in sorted(glob.glob('captures/test20/*-raw/digital.csv')):
    prev=None;clocks=[0,0];violations=[0,0];different=0;rows=0
    with open(path) as f:
        reader=csv.reader(f);next(reader)
        for row in reader:
            # channels are in order, confirmed by reader header
            v=[int(x) for x in row[1:]]
            rows+=1;different+=v[4]!=v[7]
            if prev is not None:
                for i,clock in enumerate((6,3)):
                    if prev[clock]==0 and v[clock]==1 and v[7 if i==0 else 4]==0:
                        clocks[i]+=1
                        if v[7]!=0:violations[i]+=1
            prev=v
    results.append({'capture':path,'rising_clocks':clocks,'clocks_outside_CS1':violations,'rows_with_different_selects':different})
    print(results[-1],flush=True)
open('analysis/shared-select-evidence.json','w').write(json.dumps(results,indent=2)+'\n')

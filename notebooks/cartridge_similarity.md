# Impact of the threshold on similarity search times

A
[ChEMBL blog post](http://chembl.blogspot.ch/2015/08/lsh-based-similarity-search-in-mongodb.html)
included the (to me) somewhat surprising result that similarity search
times while using the RDKit cartridge did not show much of a
dependency on the similarity threshold being used. I wanted to
investigate this a bit more closely.

I'm using a local install of ChEMBL20 for these tests. Since I don't
have the patience to wait for searches to complete with 1000
molecules, I will randomly select just 10:

    chembl_20=# select * into temporary table foo from rdk.fps order by random() limit 10;
    SELECT 10
    chembl_20=# select molregno,m from rdk.mols join foo using (molregno);
     molregno |                               m                                
    ----------+----------------------------------------------------------------
      1067143 | C[C@@H](O)[C@@H](NC(=O)N1CCN(c2ccc(C#Cc3ccccc3)cc2)CC1)C(=O)NO
       552129 | Cl.Cl.NCCNCCSc1ccncc1
      1034319 | CC(C)c1ccc(CN2CC(C(=O)Nc3ccccc3)CC2=O)cc1
       244180 | NS(=O)(=O)c1ccc(-n2nc(C(F)(F)F)cc2-c2cccc(Cl)c2)cc1CO
       357411 | CCCC1=C2c3ccc4[nH]nnc4c3CC2(CCC)CCC1=O
      1116827 | CCc1ccc(C2=NN(CCC(=O)NC3CC3)C(=O)CC2)cc1
       783512 | CCc1ccc(-c2cc(C)no2)cc1S(=O)(=O)NCc1ccc(OC)cc1
       623138 | Nc1ccc(-c2cn([C@@H]3O[C@H](CO)[C@H](O)[C@H](O)[C@H]3O)nn2)cc1
      1109930 | CCc1ccc(OCC(=O)OCC(=O)NCCNC(=O)COC(=O)COc2ccc(CC)cc2)cc1
       766599 | CC(=O)c1ccc(NC(=O)C(C)OC(=O)COc2ccc(C#N)cc2)cc1
    (10 rows)

And now look at timing results for a number of threshold values:

    chembl_20=# set rdkit.tanimoto_threshold=0.4;
    SET
    Time: 0.104 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
      3142
    (1 row)

    Time: 7022.514 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.5;
    SET
    Time: 0.103 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
       593
    (1 row)

    Time: 6966.116 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.6;
    SET
    Time: 0.112 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
       201
    (1 row)

    Time: 6914.092 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.7;
    SET
    Time: 0.116 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        93
    (1 row)

    Time: 6961.017 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.8;
    SET
    Time: 0.127 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        32
    (1 row)

    Time: 6952.292 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.9;
    SET
    Time: 0.108 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        12
    (1 row)

    Time: 6700.883 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.95;
    SET
    Time: 0.106 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        10
    (1 row)

    Time: 5512.117 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.99;
    SET
    Time: 0.102 ms
    chembl_20=# select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        10
    (1 row)

    Time: 2315.399 ms

It strikes me as somewhat odd that things are more or less flat until
very high threshold values are reached; 0.95 is the first one where a 
real difference is observable.

Let's look at the results of `explain analyze` to see if that helps understand:

    chembl_20=# set rdkit.tanimoto_threshold=0.4;
    SET
    Time: 0.138 ms
    chembl_20=# explain (analyze on, buffers on) select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;                                                                  QUERY PLAN                                                                   
    -----------------------------------------------------------------------------------------------------------------------------------------------
     Aggregate  (cost=291843.41..291843.42 rows=1 width=0) (actual time=6975.350..6975.351 rows=1 loops=1)
       Buffers: shared hit=253423, local hit=1
       ->  Nested Loop  (cost=0.43..289586.70 rows=902685 width=0) (actual time=43.666..6975.065 rows=3142 loops=1)
             Buffers: shared hit=253423, local hit=1
             ->  Seq Scan on foo  (cost=0.00..16.20 rows=620 width=32) (actual time=0.003..0.015 rows=10 loops=1)
                   Buffers: local hit=1
             ->  Index Scan using fps_mfp2_idx on fps fps1  (cost=0.43..452.49 rows=1456 width=65) (actual time=72.735..697.462 rows=314 loops=10)
                   Index Cond: (mfp2 % foo.mfp2)
                   Buffers: shared hit=253423
     Planning time: 0.063 ms
     Execution time: 6975.381 ms
    (11 rows)

    Time: 6975.672 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.7;
    SET
    Time: 0.118 ms
    chembl_20=# explain (analyze on, buffers on) select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
                                                                      QUERY PLAN                                                                  
    ----------------------------------------------------------------------------------------------------------------------------------------------
     Aggregate  (cost=291843.41..291843.42 rows=1 width=0) (actual time=7032.295..7032.296 rows=1 loops=1)
       Buffers: shared hit=250500, local hit=1
       ->  Nested Loop  (cost=0.43..289586.70 rows=902685 width=0) (actual time=240.418..7032.260 rows=93 loops=1)
             Buffers: shared hit=250500, local hit=1
             ->  Seq Scan on foo  (cost=0.00..16.20 rows=620 width=32) (actual time=0.004..0.015 rows=10 loops=1)
                   Buffers: local hit=1
             ->  Index Scan using fps_mfp2_idx on fps fps1  (cost=0.43..452.49 rows=1456 width=65) (actual time=238.074..703.216 rows=9 loops=10)
                   Index Cond: (mfp2 % foo.mfp2)
                   Buffers: shared hit=250500
     Planning time: 0.063 ms
     Execution time: 7032.327 ms
    (11 rows)

    Time: 7032.634 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.9;
    SET
    Time: 0.139 ms
    chembl_20=# explain (analyze on, buffers on) select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
                                                                      QUERY PLAN                                                                  
    ----------------------------------------------------------------------------------------------------------------------------------------------
     Aggregate  (cost=291843.41..291843.42 rows=1 width=0) (actual time=6768.003..6768.004 rows=1 loops=1)
       Buffers: shared hit=238999, local hit=1
       ->  Nested Loop  (cost=0.43..289586.70 rows=902685 width=0) (actual time=243.949..6767.985 rows=12 loops=1)
             Buffers: shared hit=238999, local hit=1
             ->  Seq Scan on foo  (cost=0.00..16.20 rows=620 width=32) (actual time=0.004..0.018 rows=10 loops=1)
                   Buffers: local hit=1
             ->  Index Scan using fps_mfp2_idx on fps fps1  (cost=0.43..452.49 rows=1456 width=65) (actual time=345.900..676.791 rows=1 loops=10)
                   Index Cond: (mfp2 % foo.mfp2)
                   Buffers: shared hit=238999
     Planning time: 0.062 ms
     Execution time: 6768.034 ms
    (11 rows)

    Time: 6768.345 ms
    chembl_20=# set rdkit.tanimoto_threshold=0.99;
    SET
    Time: 0.100 ms
    chembl_20=# explain (analyze on, buffers on) select count(*) from rdk.fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
                                                                      QUERY PLAN                                                                  
    ----------------------------------------------------------------------------------------------------------------------------------------------
     Aggregate  (cost=291843.41..291843.42 rows=1 width=0) (actual time=2287.224..2287.224 rows=1 loops=1)
       Buffers: shared hit=76686, local hit=1
       ->  Nested Loop  (cost=0.43..289586.70 rows=902685 width=0) (actual time=87.785..2287.213 rows=10 loops=1)
             Buffers: shared hit=76686, local hit=1
             ->  Seq Scan on foo  (cost=0.00..16.20 rows=620 width=32) (actual time=0.003..0.013 rows=10 loops=1)
                   Buffers: local hit=1
             ->  Index Scan using fps_mfp2_idx on fps fps1  (cost=0.43..452.49 rows=1456 width=65) (actual time=127.327..228.715 rows=1 loops=10)
                   Index Cond: (mfp2 % foo.mfp2)
                   Buffers: shared hit=76686
     Planning time: 0.063 ms
     Execution time: 2287.256 ms
    (11 rows)

    Time: 2287.559 ms

And that's it, the number of buffers that need to be accessed ends up
being essentially the same until getting to the very high
thresholds. As to why this is true? That I don't really know; I
suspect figuring it out would require digging more deeply into the
innards of how the GIST indexes work in PostgreSQL, and that's not
something I'm going to do right now.

## A larger test: ZINC

To see if the same trends are there for a larger dataset, I grabbed a
copy of the
[ZINC All Clean set](http://zinc.docking.org/subsets/all-clean).
After removing the molecules that the RDKit doesn't like (there are
actually only 14 molecules in the full set that the RDKit rejected),
this leaves 16.4 million molecules.

For the record, here's how I built the database:

    glandrum@Otter:/scratch/RDKit_git/Data/Zinc$ createdb zinc
    glandrum@Otter:/scratch/RDKit_git/Data/Zinc$ psql -c 'create extension rdkit' zinc
    CREATE EXTENSION
    glandrum@Otter:/scratch/RDKit_git/Data/Zinc$ psql -c 'create table raw_data (id SERIAL, smiles text, zinc_id char(12))' zinc
    CREATE TABLE
    glandrum@Otter:/scratch/RDKit_git/Data/Zinc$ zcat zinc_all_clean.smi.gz | sed '1d; s/\\/\\\\/g' |psql -c "copy raw_data (smiles,zinc_id) from stdin with delimiter ' '" zinc
    COPY 16403864
    glandrum@Otter:/scratch/RDKit_git/Data/Zinc$ psql zinc
    psql (9.4.2)
    Type "help" for help.

    zinc=# \timing
    Timing is on.
    zinc=# select * into mols from (select id,mol_from_smiles(smiles::cstring) m from raw_data) tmp where m is not null;
    SELECT 16403848
    Time: 8591329.609 ms
    zinc=# select id,morganbv_fp(m) as mfp2 into fps from mols;
    SELECT 16403848
    Time: 1308215.512 ms
    zinc=# create index fps_mfp2_idx on fps using gist(mfp2);
    CREATE INDEX
    Time: 1230138.847 ms
    zinc=# select * into temporary table foo from fps order by random() limit 10;
    SELECT 10
    Time: 3640.389 ms

Pick 10 random molecules:

    zinc=# select id,m from mols join foo using (id);
        id    |                                   m                                    
    ----------+------------------------------------------------------------------------
     11373174 | COc1cccc([C@@H](C)CC(=O)NCCCOC2CCCC2)c1
      6068083 | COc1ccc(CC[C@@H](C)[NH+]2CCC[C@H](O)C2)cc1
     10184316 | COc1cccc(O[C@@H](C)CNc2nccn(C)c2=O)c1
       869387 | COc1ccc(NC(=O)CN(Cc2ccc(F)cc2)C(=O)Cn2nnc3ccccc32)cc1
      2996517 | CN1C(=O)[C@H](NC(=O)Nc2ccc(Cl)cc2)N=C(c2ccccc2)c2ccccc21
      2749280 | CC(C)NS(=O)(=O)Cc1ccc(CNC(=O)c2ccc(CSc3nc4ccccc4s3)cc2)cc1
      5014876 | CCCCOC(=O)[C@@H](O)CC
     10653280 | Cc1cc(NC(=O)NC[C@](C)(O)c2ccc(C)o2)n(Cc2ccccn2)n1
      1253806 | CCCC[C@@H](CC)CNC(=O)c1cccc(OC)c1
      4846692 | O=C(c1ccc(N2CC[NH2+]CC2)c(NS(=O)(=O)c2ccccc2)c1)N1CC[NH+](C2CCCCC2)CC1
    (10 rows)

    Time: 161.055 ms

Now the searches:

    zinc=# set rdkit.tanimoto_threshold=0.5;
    SET
    Time: 0.141 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2; count 
    -------
      7443
    (1 row)

    Time: 80237.225 ms
    zinc=# set rdkit.tanimoto_threshold=0.6;
    SET
    Time: 0.089 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
       875
    (1 row)

    Time: 79778.422 ms
    zinc=# set rdkit.tanimoto_threshold=0.7;
    SET
    Time: 0.089 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
       144
    (1 row)

    Time: 79741.891 ms
    zinc=# set rdkit.tanimoto_threshold=0.8;
    SET
    Time: 0.092 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        44
    (1 row)

    Time: 79128.903 ms
    zinc=# set rdkit.tanimoto_threshold=0.9;
    SET
    Time: 0.087 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        21
    (1 row)

    Time: 73175.759 ms
    zinc=# set rdkit.tanimoto_threshold=0.95;
    SET
    Time: 0.108 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        20
    (1 row)

    Time: 49995.441 ms
    zinc=# set rdkit.tanimoto_threshold=0.99;
    SET
    Time: 0.105 ms
    zinc=# select count(*) from fps fps1 cross join (select * from foo) tmp where fps1.mfp2%tmp.mfp2;
     count 
    -------
        18
    (1 row)

    Time: 17011.942 ms

This time we start to see a difference at a threshold of 0.9, but
things remain more or less constant until then.


## An aside: chemfp performance

For raw search speed, I'm not aware of anything that does better than
Andrew Dalke's [chemfp](http://chemfp.com/), so I figured I'd go ahead
and see what exceptional performance looks like with the ChEMBL20 dataset.

I start by creating two `FPB` files with Morgan2 fingerprints:

  (py27)glandrum@Otter:~/RDKit_blog/data$ rdkit2fps -o chembl_20.mfp2.fpb --morgan ~/Downloads/chembl_20.sdf.gz 
  (py27)glandrum@Otter:~/RDKit_blog/data$ rkit2fps -o chembl_random_10.fbp --morgan chembl_random_10.smi
  
Note that I am using the licensed version of chemfp for these
tests. The open-source version is also super fast, but the idea here
is to see what the best I can do is:

    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.4 -q chembl_random_10.fpb --times -k all -c chembl_20.mfp2.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.4
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=chembl_20.mfp2.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/home/glandrum/Downloads/chembl_20.sdf.gz
    30	552129
    429	1109930
    232	623138
    433	1034319
    125	766599
    66	1067143
    99	1116827
    201	783512
    49	357411
    179	244180
    open 0.00 search 0.12 total 0.12
    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.9 -q chembl_random_10.fpb --times -k all -c chembl_20.mfp2.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.9
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=chembl_20.mfp2.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/home/glandrum/Downloads/chembl_20.sdf.gz
    2	552129
    1	1109930
    1	623138
    1	1034319
    1	766599
    1	1067143
    2	1116827
    1	783512
    1	357411
    1	244180
    open 0.00 search 0.05 total 0.05
    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.95 -q chembl_random_10.fpb --times -k all -c chembl_20.mfp2.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.95
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=chembl_20.mfp2.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/home/glandrum/Downloads/chembl_20.sdf.gz
    1	552129
    1	1109930
    1	623138
    1	1034319
    1	766599
    1	1067143
    1	1116827
    1	783512
    1	357411
    1	244180
    open 0.00 search 0.03 total 0.03
    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.99 -q chembl_random_10.fpb --times -k all -c chembl_20.mfp2.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.99
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=chembl_20.mfp2.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/home/glandrum/Downloads/chembl_20.sdf.gz
    1	552129
    1	1109930
    1	623138
    1	1034319
    1	766599
    1	1067143
    1	1116827
    1	783512
    1	357411
    1	244180
    open 0.00 search 0.02 total 0.02

Pulling the times out: doing the 10 similarity searches with tanimoto cutoffs
of 0.4, 0.9, 0.95, and 0.99 took 120ms, 50ms, 30ms, and 20ms (the
numbers are only reported with a precision of 10ms). As usual, chemfp
is just stupid fast.

*Update:* Here's equivalent chemfp output for the zinc_clean set
 of 16.4 million fingerprints:

    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.4 -q chembl_random_10.fpb --times -k all -c zinc_all_clean.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.4
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=zinc_all_clean.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/scratch/RDKit_git/Data/Zinc/zinc_all_clean.smi.gz
    92	552129
    6675	1109930
    116	623138
    19890	1034319
    5142	766599
    679	1067143
    131	1116827
    1463	783512
    0	357411
    14	244180
    open 0.00 search 1.31 total 1.31
    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.9 -q chembl_random_10.fpb --times -k all -c zinc_all_clean.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.9
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=zinc_all_clean.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/scratch/RDKit_git/Data/Zinc/zinc_all_clean.smi.gz
    0	552129
    1	1109930
    0	623138
    4	1034319
    2	766599
    0	1067143
    0	1116827
    1	783512
    0	357411
    0	244180
    open 0.00 search 0.58 total 0.58
    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.95 -q chembl_random_10.fpb --times -k all -c zinc_all_clean.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.95
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=zinc_all_clean.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/scratch/RDKit_git/Data/Zinc/zinc_all_clean.smi.gz
    0	552129
    1	1109930
    0	623138
    2	1034319
    2	766599
    0	1067143
    0	1116827
    1	783512
    0	357411
    0	244180
    open 0.00 search 0.41 total 0.41
    (py27)glandrum@Otter:~/RDKit_blog/data$ simsearch -t 0.99 -q chembl_random_10.fpb --times -k all -c zinc_all_clean.fpb 
    #Count/1
    #num_bits=2048
    #type=Count threshold=0.99
    #software=chemfp/2.1
    #queries=chembl_random_10.fpb
    #targets=zinc_all_clean.fpb
    #query_sources=chembl_random_10.smi
    #target_sources=/scratch/RDKit_git/Data/Zinc/zinc_all_clean.smi.gz
    0	552129
    1	1109930
    0	623138
    2	1034319
    2	766599
    0	1067143
    0	1116827
    1	783512
    0	357411
    0	244180
    open 0.00 search 0.22 total 0.22

Great stuff!
